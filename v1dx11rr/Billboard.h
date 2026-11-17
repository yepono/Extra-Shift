#ifndef _BillBoard_
#define _BillBoard_

// Billboards: quads con una imagen que SIEMPRE miran a la camara.
// Sirven para productos, fuego, carteles flotantes, etc.
//
// Para imagenes fijas en el mundo (pegadas a una pared o estanteria y que
// no giran) usar PanelRR.
//
// El origen de cada billboard es el centro de su borde inferior:
//   - x, z: posicion horizontal en el mundo.
//   - y: altura del borde inferior.
//   - ancho, alto: tamano en unidades del mundo.
//
// Uso:
//   BillboardRR billboards;
//   billboards.Inicializar(d3dDevice, d3dContext);
//   billboards.AgregarConAspecto(L"Assets/Billboards/boing.png",
//                                x, z, y, 2.0f);
//   ...
//   billboards.Dibujar(camara->View, camara->Projection, camara->Position);
//
// Se dibuja con blending alfa y depth test. Al terminar, restaura los
// estados por defecto para no afectar a los dibujos siguientes.

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <d3dx10math.h>
#include <cmath>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

class BillboardRR
{
public:
	BillboardRR()
	{
	}

	~BillboardRR()
	{
		Liberar();
	}

	bool Inicializar(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		Liberar();

		if (device == nullptr || context == nullptr)
			return false;

		m_device = device;
		m_context = context;

		if (!CrearShader() || !CrearBuffers() || !CrearEstados())
		{
			Liberar();
			return false;
		}

		m_listo = true;
		return true;
	}

	void Liberar()
	{
		for (size_t i = 0; i < m_entradas.size(); ++i)
		{
			if (m_entradas[i].textura != nullptr)
			{
				m_entradas[i].textura->Release();
				m_entradas[i].textura = nullptr;
			}
		}

		m_entradas.clear();

		if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
		if (m_indexBuffer) { m_indexBuffer->Release(); m_indexBuffer = nullptr; }
		if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
		if (m_sampler) { m_sampler->Release(); m_sampler = nullptr; }
		if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }
		if (m_depthState) { m_depthState->Release(); m_depthState = nullptr; }
		if (m_rasterizerState) { m_rasterizerState->Release(); m_rasterizerState = nullptr; }
		if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
		if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
		if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }

		m_listo = false;
		m_device = nullptr;
		m_context = nullptr;
	}

	// Registra un billboard con tamano explicito.
	bool Agregar(
		const WCHAR* textura,
		float x,
		float z,
		float y,
		float ancho,
		float alto)
	{
		if (!m_listo || textura == nullptr || ancho <= 0.0f || alto <= 0.0f)
			return false;

		ID3D11ShaderResourceView* srv = nullptr;

		const HRESULT result = D3DX11CreateShaderResourceViewFromFile(
			m_device, textura, nullptr, nullptr, &srv, nullptr);

		if (FAILED(result) || srv == nullptr)
		{
			OutputDebugStringW(L"BillboardRR: no se pudo cargar la textura\n");
			return false;
		}

		Entrada entrada;
		entrada.textura = srv;
		entrada.x = x;
		entrada.z = z;
		entrada.y = y;
		entrada.ancho = ancho;
		entrada.alto = alto;

		m_entradas.push_back(entrada);
		return true;
	}

	// Igual que Agregar, pero calcula el alto a partir de la imagen para
	// no deformarla.
	bool AgregarConAspecto(
		const WCHAR* textura,
		float x,
		float z,
		float y,
		float ancho)
	{
		D3DX11_IMAGE_INFO info;
		ZeroMemory(&info, sizeof(info));

		const HRESULT result = D3DX11GetImageInfoFromFile(
			textura, nullptr, &info, nullptr);

		if (FAILED(result) || info.Width == 0)
			return false;

		const float alto =
			ancho * (float)info.Height / (float)info.Width;

		return Agregar(textura, x, z, y, ancho, alto);
	}

	void Dibujar(
		const D3DXMATRIX& vista,
		const D3DXMATRIX& proyeccion,
		const D3DXVECTOR3& posCamara)
	{
		if (!m_listo || m_context == nullptr || m_entradas.empty())
			return;

		D3DXMATRIX vistaProyeccion;
		D3DXMatrixMultiply(&vistaProyeccion, &vista, &proyeccion);

		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_context->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(m_depthState, 0);
		m_context->RSSetState(m_rasterizerState);

		m_context->IASetInputLayout(m_inputLayout);
		m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->VSSetShader(m_vertexShader, nullptr, 0);
		m_context->PSSetShader(m_pixelShader, nullptr, 0);
		m_context->PSSetSamplers(0, 1, &m_sampler);

		const UINT stride = sizeof(VerticeBillboard);
		const UINT offset = 0;

		for (size_t i = 0; i < m_entradas.size(); ++i)
		{
			const Entrada& entrada = m_entradas[i];

			// Direccion horizontal desde el billboard hasta la camara.
			float dirX = posCamara.x - entrada.x;
			float dirZ = posCamara.z - entrada.z;
			float distancia = sqrtf(dirX * dirX + dirZ * dirZ);

			if (distancia < 0.001f)
			{
				// La camara esta justo encima: usar una direccion fija.
				dirX = 0.0f;
				dirZ = 1.0f;
				distancia = 1.0f;
			}

			dirX /= distancia;
			dirZ /= distancia;

			// El quad local esta en el plano YZ y su normal local es +X.
			// Esta rotacion lleva +X hacia la camara.
			D3DXMATRIX rotacion;
			D3DXMatrixIdentity(&rotacion);
			rotacion._11 = dirX;
			rotacion._13 = dirZ;
			rotacion._31 = -dirZ;
			rotacion._33 = dirX;

			D3DXMATRIX traslacion;
			D3DXMatrixTranslation(
				&traslacion, entrada.x, entrada.y, entrada.z);

			D3DXMATRIX mundo;
			D3DXMatrixMultiply(&mundo, &rotacion, &traslacion);

			const float mitad = entrada.ancho * 0.5f;

			// El eje local +Z apunta a la izquierda de la camara, por eso
			// la U se invierte para que la imagen no salga espejeada.
			VerticeBillboard vertices[4] =
			{
				{ 0.0f, 0.0f, -mitad, 1.0f, 1.0f },
				{ 0.0f, entrada.alto, -mitad, 1.0f, 0.0f },
				{ 0.0f, entrada.alto, mitad, 0.0f, 0.0f },
				{ 0.0f, 0.0f, mitad, 0.0f, 1.0f },
			};

			D3D11_MAPPED_SUBRESOURCE mapped;
			if (FAILED(m_context->Map(
					m_vertexBuffer,
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mapped)))
			{
				continue;
			}

			memcpy(mapped.pData, vertices, sizeof(vertices));
			m_context->Unmap(m_vertexBuffer, 0);

			Constantes constantes;
			constantes.mundo = mundo;
			constantes.vistaProyeccion = vistaProyeccion;

			m_context->UpdateSubresource(
				m_constantBuffer, 0, nullptr, &constantes, 0, 0);

			m_context->IASetVertexBuffers(
				0, 1, &m_vertexBuffer, &stride, &offset);
			m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
			m_context->PSSetShaderResources(0, 1, &entrada.textura);
			m_context->DrawIndexed(6, 0, 0);
		}

		// Restaurar los estados por defecto.
		m_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		m_context->OMSetDepthStencilState(nullptr, 0);
		m_context->RSSetState(nullptr);
	}

	size_t Cantidad() const
	{
		return m_entradas.size();
	}

private:
	struct Entrada
	{
		ID3D11ShaderResourceView* textura;
		float x, z, y;
		float ancho, alto;
	};

	struct VerticeBillboard
	{
		float x, y, z;
		float u, v;
	};

	struct Constantes
	{
		D3DXMATRIX mundo;
		D3DXMATRIX vistaProyeccion;
	};

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;

	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_indexBuffer = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11SamplerState* m_sampler = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11DepthStencilState* m_depthState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;

	bool m_listo = false;

	std::vector<Entrada> m_entradas;

	bool CrearShader()
	{
		ID3DBlob* vsBlob = nullptr;
		ID3DBlob* psBlob = nullptr;

		if (!CompilarShader("VS_Main", "vs_4_0", &vsBlob))
			return false;

		if (!CompilarShader("PS_Main", "ps_4_0", &psBlob))
		{
			vsBlob->Release();
			return false;
		}

		HRESULT result = m_device->CreateVertexShader(
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			nullptr,
			&m_vertexShader);

		if (SUCCEEDED(result))
		{
			result = m_device->CreatePixelShader(
				psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(),
				nullptr,
				&m_pixelShader);
		}

		if (SUCCEEDED(result))
		{
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				  D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
				  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			result = m_device->CreateInputLayout(
				layout,
				ARRAYSIZE(layout),
				vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(),
				&m_inputLayout);
		}

		vsBlob->Release();
		psBlob->Release();

		return SUCCEEDED(result);
	}

	bool CompilarShader(
		const char* entrada,
		const char* perfil,
		ID3DBlob** blob)
	{
		ID3DBlob* errores = nullptr;

		const HRESULT result = D3DX11CompileFromFile(
			L"Billboard.hlsl",
			nullptr,
			nullptr,
			entrada,
			perfil,
			D3DCOMPILE_ENABLE_STRICTNESS,
			0,
			nullptr,
			blob,
			&errores,
			nullptr);

		if (FAILED(result))
		{
			if (errores != nullptr)
			{
				OutputDebugStringA((char*)errores->GetBufferPointer());
				errores->Release();
			}

			return false;
		}

		if (errores != nullptr)
			errores->Release();

		return true;
	}

	bool CrearBuffers()
	{
		D3D11_BUFFER_DESC vertexDesc;
		ZeroMemory(&vertexDesc, sizeof(vertexDesc));
		vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexDesc.ByteWidth = sizeof(VerticeBillboard) * 4;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT result = m_device->CreateBuffer(
			&vertexDesc, nullptr, &m_vertexBuffer);

		if (FAILED(result))
			return false;

		const UINT indices[6] = { 0, 1, 2, 0, 2, 3 };

		D3D11_BUFFER_DESC indexDesc;
		ZeroMemory(&indexDesc, sizeof(indexDesc));
		indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexDesc.ByteWidth = sizeof(indices);
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA indexData;
		ZeroMemory(&indexData, sizeof(indexData));
		indexData.pSysMem = indices;

		result = m_device->CreateBuffer(
			&indexDesc, &indexData, &m_indexBuffer);

		if (FAILED(result))
			return false;

		D3D11_BUFFER_DESC constantDesc;
		ZeroMemory(&constantDesc, sizeof(constantDesc));
		constantDesc.Usage = D3D11_USAGE_DEFAULT;
		constantDesc.ByteWidth = sizeof(Constantes);
		constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		result = m_device->CreateBuffer(
			&constantDesc, nullptr, &m_constantBuffer);

		if (FAILED(result))
			return false;

		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		return SUCCEEDED(m_device->CreateSamplerState(
			&samplerDesc, &m_sampler));
	}

	bool CrearEstados()
	{
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask =
			D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT result = m_device->CreateBlendState(
			&blendDesc, &m_blendState);

		if (FAILED(result))
			return false;

		D3D11_DEPTH_STENCIL_DESC depthDesc;
		ZeroMemory(&depthDesc, sizeof(depthDesc));
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;

		result = m_device->CreateDepthStencilState(
			&depthDesc, &m_depthState);

		if (FAILED(result))
			return false;

		D3D11_RASTERIZER_DESC rasterizerDesc;
		ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		rasterizerDesc.DepthClipEnable = TRUE;

		return SUCCEEDED(m_device->CreateRasterizerState(
			&rasterizerDesc, &m_rasterizerState));
	}
};

#endif
