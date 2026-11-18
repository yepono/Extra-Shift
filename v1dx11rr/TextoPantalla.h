#ifndef _texto_pantalla
#define _texto_pantalla

// Texto 2D en pantalla para depuracion (por ejemplo, las coordenadas del
// jugador). Usa una fuente 5x7 embebida y un shader propio, sin texturas
// ni librerias extra. Sigue el mismo estilo que ColisionDebug.h.
//
// Uso:
//   TextoPantalla texto;
//   texto.Inicializar(d3dDevice, d3dContext);
//   texto.Dibujar("JUGADOR\nX: 12.34   Z: -56.78");
//
// El texto se dibuja en coordenadas de pantalla, desde la esquina
// superior izquierda, antes de Present().

#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

class TextoPantalla
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
    float4 color    : COLOR;
};

struct PSInput
{
    float4 posicion : SV_POSITION;
    float4 color    : COLOR;
};

PSInput VSMain(VSInput entrada)
{
    PSInput salida;
    salida.posicion = float4(entrada.posicion, 0.0f, 1.0f);
    salida.color = entrada.color;
    return salida;
}

float4 PSMain(PSInput entrada) : SV_Target
{
    return entrada.color;
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
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
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
		vertexDesc.ByteWidth = sizeof(VerticeTexto) * MaxVertices;
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

	// Dibuja un texto multilinea (separa las lineas con '\n').
	// `escala` es el tamano en pixeles de cada pixel de la fuente 5x7.
	void Dibujar(
		const char* texto,
		float x = 12.0f,
		float y = 12.0f,
		float escala = 2.0f,
		bool conFondo = true,
		float rojo = 1.0f,
		float verde = 1.0f,
		float azul = 1.0f)
	{
		if (!m_listo || m_context == nullptr || texto == nullptr)
			return;

		if (escala < 1.0f)
			escala = 1.0f;

		UINT totalViewports = 1;
		D3D11_VIEWPORT viewport;
		m_context->RSGetViewports(&totalViewports, &viewport);

		if (totalViewports == 0 ||
			viewport.Width <= 0.0f ||
			viewport.Height <= 0.0f)
		{
			return;
		}

		const float anchoLetra = 6.0f * escala;
		const float altoLinea = 9.0f * escala;

		// Mide el texto para pintar un fondo que lo haga legible.
		float anchoMaximo = 0.0f;
		float anchoActual = 0.0f;
		int lineas = 1;

		for (const char* p = texto; *p != '\0'; ++p)
		{
			if (*p == '\n')
			{
				if (anchoActual > anchoMaximo)
					anchoMaximo = anchoActual;

				anchoActual = 0.0f;
				++lineas;
			}
			else
			{
				anchoActual += anchoLetra;
			}
		}

		if (anchoActual > anchoMaximo)
			anchoMaximo = anchoActual;

		const float margen = 8.0f;
		const float fondoColor[4] = { 0.0f, 0.0f, 0.0f, 0.65f };
		const float textoColor[4] = { rojo, verde, azul, 1.0f };

		std::vector<VerticeTexto> vertices;
		vertices.reserve(strlen(texto) * 210 + 6);

		if (conFondo)
		{
			AgregarRect(
				vertices,
				x - margen,
				y - margen,
				x + anchoMaximo + margen,
				y + lineas * altoLinea + margen,
				viewport.Width,
				viewport.Height,
				fondoColor);
		}

		float cursorX = x;
		float cursorY = y;

		for (const char* p = texto; *p != '\0'; ++p)
		{
			if (*p == '\n')
			{
				cursorX = x;
				cursorY += altoLinea;
				continue;
			}

			DibujarGlifo(
				vertices,
				*p,
				cursorX,
				cursorY,
				escala,
				viewport.Width,
				viewport.Height,
				textoColor);

			cursorX += anchoLetra;
		}

		if (vertices.empty())
			return;

		if (vertices.size() > MaxVertices)
			vertices.resize(MaxVertices);

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

		memcpy(
			mapped.pData,
			vertices.data(),
			vertices.size() * sizeof(VerticeTexto));

		m_context->Unmap(m_vertexBuffer, 0);

		UINT stride = sizeof(VerticeTexto);
		UINT offset = 0;

		m_context->IASetInputLayout(m_inputLayout);
		m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->VSSetShader(m_vertexShader, nullptr, 0);
		m_context->PSSetShader(m_pixelShader, nullptr, 0);

		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_context->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(m_depthState, 0);
		m_context->RSSetState(m_rasterizerState);

		m_context->Draw((UINT)vertices.size(), 0);
	}

private:
	struct VerticeTexto
	{
		float x, y;
		float r, g, b, a;
	};

	struct Glifo
	{
		char caracter;
		unsigned char filas[7];
	};

	static const size_t MaxVertices = 32768;

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;

	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11DepthStencilState* m_depthState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;

	bool m_listo = false;

	static void AgregarRect(
		std::vector<VerticeTexto>& vertices,
		float x0,
		float y0,
		float x1,
		float y1,
		float anchoPantalla,
		float altoPantalla,
		const float color[4])
	{
		const float ndcX0 = (x0 / anchoPantalla) * 2.0f - 1.0f;
		const float ndcX1 = (x1 / anchoPantalla) * 2.0f - 1.0f;
		const float ndcY0 = 1.0f - (y0 / altoPantalla) * 2.0f;
		const float ndcY1 = 1.0f - (y1 / altoPantalla) * 2.0f;

		VerticeTexto a = { ndcX0, ndcY0, color[0], color[1], color[2], color[3] };
		VerticeTexto b = { ndcX1, ndcY0, color[0], color[1], color[2], color[3] };
		VerticeTexto c = { ndcX1, ndcY1, color[0], color[1], color[2], color[3] };
		VerticeTexto d = { ndcX0, ndcY1, color[0], color[1], color[2], color[3] };

		vertices.push_back(a);
		vertices.push_back(b);
		vertices.push_back(c);

		vertices.push_back(a);
		vertices.push_back(c);
		vertices.push_back(d);
	}

	// Fuente 5x7. Cada fila usa 5 bits (16 = columna izquierda).
	static const Glifo* TablaGlifos(int& total)
	{
		static const Glifo glifos[] =
		{
			{ ' ', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 } },
			{ '-', { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 } },
			{ '.', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100 } },
			{ ':', { 0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000 } },
			{ '!', { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 } },
			{ '?', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100 } },
			{ ',', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100, 0b01000 } },
			{ '\'', { 0b00100, 0b00100, 0b01000, 0b00000, 0b00000, 0b00000, 0b00000 } },

			{ '0', { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 } },
			{ '1', { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
			{ '2', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 } },
			{ '3', { 0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110 } },
			{ '4', { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 } },
			{ '5', { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 } },
			{ '6', { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 } },
			{ '7', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 } },
			{ '8', { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 } },
			{ '9', { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 } },

			{ 'A', { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
			{ 'B', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 } },
			{ 'C', { 0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110 } },
			{ 'D', { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 } },
			{ 'E', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 } },
			{ 'F', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 } },
			{ 'G', { 0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 } },
			{ 'H', { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
			{ 'I', { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
			{ 'J', { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 } },
			{ 'K', { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 } },
			{ 'L', { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 } },
			{ 'M', { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 } },
			{ 'N', { 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001 } },
			{ 'O', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
			{ 'P', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 } },
			{ 'Q', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 } },
			{ 'R', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 } },
			{ 'S', { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 } },
			{ 'T', { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 } },
			{ 'U', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
			{ 'V', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 } },
			{ 'W', { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001 } },
			{ 'X', { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 } },
			{ 'Y', { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 } },
			{ 'Z', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 } },
		};

		total = (int)(sizeof(glifos) / sizeof(glifos[0]));
		return glifos;
	}

	static const Glifo* BuscarGlifo(char c)
	{
		int total = 0;
		const Glifo* glifos = TablaGlifos(total);

		for (int i = 0; i < total; ++i)
		{
			if (glifos[i].caracter == c)
				return &glifos[i];
		}

		return nullptr;
	}

	static void DibujarGlifo(
		std::vector<VerticeTexto>& vertices,
		char c,
		float x,
		float y,
		float escala,
		float anchoPantalla,
		float altoPantalla,
		const float color[4])
	{
		const Glifo* glifo = BuscarGlifo(c);

		if (glifo == nullptr)
			return;

		for (int fila = 0; fila < 7; ++fila)
		{
			const unsigned char bits = glifo->filas[fila];

			for (int columna = 0; columna < 5; ++columna)
			{
				if ((bits & (1 << (4 - columna))) == 0)
					continue;

				AgregarRect(
					vertices,
					x + columna * escala,
					y + fila * escala,
					x + (columna + 1) * escala,
					y + (fila + 1) * escala,
					anchoPantalla,
					altoPantalla,
					color);
			}
		}
	}
};

#endif
