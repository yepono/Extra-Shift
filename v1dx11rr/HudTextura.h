#ifndef _hud_textura
#define _hud_textura

// Quads 2D en pantalla para el HUD (dialogo y barra de vida).
// Las coordenadas estan en pixeles, origen arriba a la izquierda.
// El recorte UV permite dibujar solo la parte util de un PNG con
// mucho margen transparente, y encoger la barra de vida.

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

class HudTextura
{
public:
	bool Inicializar(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		Liberar();

		if (device == nullptr || context == nullptr)
			return false;

		m_device = device;
		m_context = context;

		static const char* shaderSource = R"(
struct VSInput
{
    float2 posicion : POSITION;
    float2 uv       : TEXCOORD;
};

struct PSInput
{
    float4 posicion : SV_POSITION;
    float2 uv       : TEXCOORD;
};

PSInput VSMain(VSInput entrada)
{
    PSInput salida;
    salida.posicion = float4(entrada.posicion, 0.0f, 1.0f);
    salida.uv = entrada.uv;
    return salida;
}

Texture2D colorMap : register(t0);
SamplerState samplerMap : register(s0);

float4 PSMain(PSInput entrada) : SV_Target
{
    return colorMap.Sample(samplerMap, entrada.uv);
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
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
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

		if (FAILED(result))
		{
			Liberar();
			return false;
		}

		D3D11_BUFFER_DESC vertexDesc;
		ZeroMemory(&vertexDesc, sizeof(vertexDesc));
		vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexDesc.ByteWidth = sizeof(VerticeHud) * 6;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		result = m_device->CreateBuffer(&vertexDesc, nullptr, &m_vertexBuffer);

		if (SUCCEEDED(result))
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

			result = m_device->CreateBlendState(&blendDesc, &m_blendState);
		}

		if (SUCCEEDED(result))
		{
			D3D11_DEPTH_STENCIL_DESC depthDesc;
			ZeroMemory(&depthDesc, sizeof(depthDesc));
			depthDesc.DepthEnable = FALSE;
			depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			depthDesc.StencilEnable = FALSE;

			result = m_device->CreateDepthStencilState(
				&depthDesc, &m_depthState);
		}

		if (SUCCEEDED(result))
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			ZeroMemory(&rasterDesc, sizeof(rasterDesc));
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.CullMode = D3D11_CULL_NONE;
			rasterDesc.DepthClipEnable = TRUE;

			result = m_device->CreateRasterizerState(
				&rasterDesc, &m_rasterizerState);
		}

		if (SUCCEEDED(result))
		{
			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(samplerDesc));
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samplerDesc.MinLOD = 0;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			result = m_device->CreateSamplerState(&samplerDesc, &m_sampler);
		}

		if (FAILED(result))
		{
			Liberar();
			return false;
		}

		m_listo = true;
		return true;
	}

	void Liberar()
	{
		for (int i = 0; i < MaxTexturas; ++i)
		{
			if (m_texturas[i] != nullptr)
			{
				m_texturas[i]->Release();
				m_texturas[i] = nullptr;
			}
		}

		if (m_sampler) { m_sampler->Release(); m_sampler = nullptr; }
		if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
		if (m_rasterizerState) { m_rasterizerState->Release(); m_rasterizerState = nullptr; }
		if (m_depthState) { m_depthState->Release(); m_depthState = nullptr; }
		if (m_blendState) { m_blendState->Release(); m_blendState = nullptr; }
		if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
		if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
		if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }

		m_listo = false;
		m_device = nullptr;
		m_context = nullptr;
	}

	bool Cargar(int indice, const WCHAR* ruta)
	{
		if (!m_listo || indice < 0 || indice >= MaxTexturas || ruta == nullptr)
			return false;

		if (m_texturas[indice] != nullptr)
		{
			m_texturas[indice]->Release();
			m_texturas[indice] = nullptr;
		}

		const HRESULT result = D3DX11CreateShaderResourceViewFromFile(
			m_device,
			ruta,
			nullptr,
			nullptr,
			&m_texturas[indice],
			nullptr);

		return SUCCEEDED(result) && m_texturas[indice] != nullptr;
	}

	// Dibuja un recorte de la textura en un rectangulo de pantalla.
	void Dibujar(
		int indice,
		float x,
		float y,
		float ancho,
		float alto,
		float u0,
		float v0,
		float u1,
		float v1)
	{
		if (!m_listo || m_context == nullptr)
			return;

		if (indice < 0 || indice >= MaxTexturas || m_texturas[indice] == nullptr)
			return;

		if (ancho <= 0.0f || alto <= 0.0f)
			return;

		UINT totalViewports = 1;
		D3D11_VIEWPORT viewport;
		m_context->RSGetViewports(&totalViewports, &viewport);

		if (totalViewports == 0 ||
			viewport.Width <= 0.0f ||
			viewport.Height <= 0.0f)
		{
			return;
		}

		const float x0 = (x / viewport.Width) * 2.0f - 1.0f;
		const float x1 = ((x + ancho) / viewport.Width) * 2.0f - 1.0f;
		const float y0 = 1.0f - (y / viewport.Height) * 2.0f;
		const float y1 = 1.0f - ((y + alto) / viewport.Height) * 2.0f;

		VerticeHud vertices[6] =
		{
			{ x0, y0, u0, v0 },
			{ x1, y0, u1, v0 },
			{ x1, y1, u1, v1 },
			{ x0, y0, u0, v0 },
			{ x1, y1, u1, v1 },
			{ x0, y1, u0, v1 },
		};

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (FAILED(m_context->Map(
				m_vertexBuffer,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mapped)))
		{
			return;
		}

		memcpy(mapped.pData, vertices, sizeof(vertices));
		m_context->Unmap(m_vertexBuffer, 0);

		UINT stride = sizeof(VerticeHud);
		UINT offset = 0;

		m_context->IASetInputLayout(m_inputLayout);
		m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->VSSetShader(m_vertexShader, nullptr, 0);
		m_context->PSSetShader(m_pixelShader, nullptr, 0);
		m_context->PSSetShaderResources(0, 1, &m_texturas[indice]);
		m_context->PSSetSamplers(0, 1, &m_sampler);

		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_context->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(m_depthState, 0);
		m_context->RSSetState(m_rasterizerState);

		m_context->Draw(6, 0);
	}

	void DibujarCompleta(int indice)
	{
		UINT totalViewports = 1;
		D3D11_VIEWPORT viewport;
		if (m_context == nullptr)
			return;

		m_context->RSGetViewports(&totalViewports, &viewport);
		Dibujar(indice, 0.0f, 0.0f, viewport.Width, viewport.Height, 0.0f, 0.0f, 1.0f, 1.0f);
	}

private:
	struct VerticeHud
	{
		float x, y;
		float u, v;
	};

	static const int MaxTexturas = 8;

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11DepthStencilState* m_depthState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	ID3D11SamplerState* m_sampler = nullptr;
	ID3D11ShaderResourceView* m_texturas[MaxTexturas] = {};
	bool m_listo = false;
};

#endif
