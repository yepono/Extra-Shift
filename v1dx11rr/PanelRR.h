#ifndef _panel_rr
#define _panel_rr

// Paneles con una imagen FIJA en el mundo: no giran hacia la camara.
// Sirven para carteles, fotos, productos en estanterias, etc.
//
// El panel es un quad de `ancho` x `alto` unidades. El origen (x, y, z)
// es el CENTRO DEL BORDE INFERIOR:
//   - x, z: posicion horizontal en el mundo.
//   - y: altura del borde inferior. Para apoyarlo en el suelo, pasar
//     terreno->Superficie(x, z).
//   - giroGrados: rotacion alrededor de Y. 0 mira hacia +Z y 180 hacia -Z.
//
// Es de doble cara: se ve desde adelante y desde atras (por detras sale
// espejeado). El giro solo sirve para orientar la imagen.
//
// Uso:
//   PanelRR paneles;
//   paneles.Inicializar(d3dDevice, d3dContext);
//   paneles.AgregarConAspecto(L"Assets/Billboards/cokita.png",
//                             x, y, z, 3.0f, 180.0f);
//   ...
//   paneles.Dibujar(camara->View, camara->Projection);
//
// Se dibuja con blending alfa y depth test. Al terminar, restaura los
// estados por defecto.

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <d3dx10math.h>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

class PanelRR
{
public:
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
		for (size_t i = 0; i < m_paneles.size(); ++i)
		{
			if (m_paneles[i].textura != nullptr)
			{
				m_paneles[i].textura->Release();
				m_paneles[i].textura = nullptr;
			}
		}

		m_paneles.clear();

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

	// Registra un panel con tamano explicito.
	bool Agregar(
		const WCHAR* textura,
		float x,
		float y,
		float z,
		float ancho,
		float alto,
		float giroGrados = 0.0f)
	{
		if (!m_listo || textura == nullptr || ancho <= 0.0f || alto <= 0.0f)
			return false;

		ID3D11ShaderResourceView* srv = nullptr;

		const HRESULT result = D3DX11CreateShaderResourceViewFromFile(
			m_device, textura, nullptr, nullptr, &srv, nullptr);

		if (FAILED(result) || srv == nullptr)
		{
			OutputDebugStringW(L"PanelRR: no se pudo cargar la textura\n");
			return false;
		}

		Panel panel;
		panel.textura = srv;
		panel.mundo = CrearMundo(x, y, z, ancho, alto, giroGrados);

		m_paneles.push_back(panel);
		return true;
	}

	// Igual que Agregar, pero calcula el alto a partir de la imagen para
	// que no se deforme. Ideal para imagenes anchas (por ejemplo, las
	// franjas de producto de Assets/Billboards).
	bool AgregarConAspecto(
		const WCHAR* textura,
		float x,
		float y,
		float z,
		float ancho,
		float giroGrados = 0.0f)
	{
		D3DX11_IMAGE_INFO info;
		ZeroMemory(&info, sizeof(info));

		const HRESULT result = D3DX11GetImageInfoFromFile(
			textura, nullptr, &info, nullptr);

		if (FAILED(result) || info.Width == 0)
			return false;

		const float alto =
			ancho * (float)info.Height / (float)info.Width;

		return Agregar(textura, x, y, z, ancho, alto, giroGrados);
	}

	void Dibujar(const D3DXMATRIX& vista, const D3DXMATRIX& proyeccion)
	{
		if (!m_listo || m_context == nullptr || m_paneles.empty())
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

		const UINT stride = sizeof(VerticePanel);
		const UINT offset = 0;

		// Quad unitario en el plano XY, centrado en el origen.
		const VerticePanel vertices[4] =
		{
			{ -0.5f, -0.5f, 0.0f, 0.0f, 1.0f },
			{ -0.5f,  0.5f, 0.0f, 0.0f, 0.0f },
			{  0.5f,  0.5f, 0.0f, 1.0f, 0.0f },
			{  0.5f, -0.5f, 0.0f, 1.0f, 1.0f },
		};

		for (size_t i = 0; i < m_paneles.size(); ++i)
		{
			const Panel& panel = m_paneles[i];

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
			constantes.mundo = panel.mundo;
			constantes.vistaProyeccion = vistaProyeccion;

			m_context->UpdateSubresource(
				m_constantBuffer, 0, nullptr, &constantes, 0, 0);

			m_context->IASetVertexBuffers(
				0, 1, &m_vertexBuffer, &stride, &offset);
			m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
			m_context->PSSetShaderResources(0, 1, &panel.textura);
			m_context->DrawIndexed(6, 0, 0);
		}

		// Restaurar los estados por defecto.
		m_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		m_context->OMSetDepthStencilState(nullptr, 0);
		m_context->RSSetState(nullptr);
	}

	size_t Cantidad() const
	{
		return m_paneles.size();
	}

private:
	struct Panel
	{
		ID3D11ShaderResourceView* textura;
		D3DXMATRIX mundo;
	};

	struct VerticePanel
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

	std::vector<Panel> m_paneles;

	// El origen del panel es el centro de su borde inferior.
	static D3DXMATRIX CrearMundo(
		float x,
		float y,
		float z,
		float ancho,
		float alto,
		float giroGrados)
	{
		D3DXMATRIX escala;
		D3DXMATRIX rotacion;
		D3DXMATRIX traslacion;
		D3DXMATRIX mundo;

		D3DXMatrixScaling(&escala, ancho, alto, 1.0f);
		D3DXMatrixRotationY(&rotacion, D3DXToRadian(giroGrados));
		D3DXMatrixTranslation(&traslacion, x, y + alto * 0.5f, z);

		D3DXMatrixMultiply(&mundo, &escala, &rotacion);
		D3DXMatrixMultiply(&mundo, &mundo, &traslacion);

		return mundo;
	}

	bool CrearShader()
	{
		// HLSL embebido: el panel solo necesita color y matrices.
		static const char* shaderSource = R"(
cbuffer Matrices : register(b0)
{
    row_major float4x4 mundo;
    row_major float4x4 vistaProyeccion;
};

Texture2D colorMap : register(t0);
SamplerState colorSampler : register(s0);

struct VSInput
{
    float3 posicion : POSITION;
    float2 uv       : TEXCOORD0;
};

struct PSInput
{
    float4 posicion : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

PSInput VSMain(VSInput entrada)
{
    PSInput salida;
    salida.posicion = mul(mul(float4(entrada.posicion, 1.0f), mundo), vistaProyeccion);
    salida.uv = entrada.uv;
    return salida;
}

float4 PSMain(PSInput entrada) : SV_Target
{
    float4 color = colorMap.Sample(colorSampler, entrada.uv);
    clip(color.a - 0.01f);
    return color;
}
)";

		ID3DBlob* vsBlob = nullptr;
		ID3DBlob* psBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;

		HRESULT result = D3DCompile(
			shaderSource,
			strlen(shaderSource),
			nullptr,
			nullptr,
			nullptr,
			"VSMain",
			"vs_5_0",
			0,
			0,
			&vsBlob,
			&errorBlob);

		if (FAILED(result))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}
			return false;
		}

		result = D3DCompile(
			shaderSource,
			strlen(shaderSource),
			nullptr,
			nullptr,
			nullptr,
			"PSMain",
			"ps_5_0",
			0,
			0,
			&psBlob,
			&errorBlob);

		if (FAILED(result))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}
			vsBlob->Release();
			return false;
		}

		result = m_device->CreateVertexShader(
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

	bool CrearBuffers()
	{
		D3D11_BUFFER_DESC vertexDesc;
		ZeroMemory(&vertexDesc, sizeof(vertexDesc));
		vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexDesc.ByteWidth = sizeof(VerticePanel) * 4;
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
		// Doble cara: un cartel no debe desaparecer si se mira desde atras.
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		rasterizerDesc.DepthClipEnable = TRUE;

		return SUCCEEDED(m_device->CreateRasterizerState(
			&rasterizerDesc, &m_rasterizerState));
	}
};

#endif
