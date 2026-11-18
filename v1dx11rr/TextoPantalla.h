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
private:
	struct VerticeTexto;

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

		const float anchoLetra = AnchoLetra(escala);
		const float altoLinea = AltoLinea(escala);

		// Mide el texto para pintar un fondo que lo haga legible.
		int lineas = 1;
		for (const char* p = texto; *p != '\0'; ++p)
		{
			if (*p == '\n')
				++lineas;
		}

		const float anchoMaximo = MedirAncho(texto, escala);

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

		for (const char* p = texto; *p != '\0';)
		{
			if (*p == '\n')
			{
				cursorX = x;
				cursorY += altoLinea;
				++p;
				continue;
			}

			const int codigo = SiguienteCodigo(p);

			// Las 2 primeras filas de cada renglon quedan para los acentos.
			DibujarGlifo(
				vertices,
				codigo,
				cursorX,
				cursorY + 2.0f * escala,
				escala,
				viewport.Width,
				viewport.Height,
				textoColor);

			cursorX += anchoLetra;
		}

		EnviarVertices(vertices);
	}

	// Rectangulo de color solido en pixeles de pantalla (para oscurecer
	// la pantalla de game over, por ejemplo).
	void DibujarRect(float x0, float y0, float x1, float y1, const float color[4])
	{
		if (!m_listo || m_context == nullptr)
			return;

		UINT totalViewports = 1;
		D3D11_VIEWPORT viewport;
		m_context->RSGetViewports(&totalViewports, &viewport);

		if (totalViewports == 0 || viewport.Width <= 0.0f || viewport.Height <= 0.0f)
			return;

		std::vector<VerticeTexto> vertices;
		AgregarRect(vertices, x0, y0, x1, y1, viewport.Width, viewport.Height, color);
		EnviarVertices(vertices);
	}

	static float AnchoLetra(float escala) { return 6.0f * escala; }
	static float AltoLinea(float escala) { return 11.0f * escala; }

	// Ancho en pixeles del renglon mas largo (cuenta letras, no bytes).
	static float MedirAncho(const char* texto, float escala)
	{
		if (texto == nullptr)
			return 0.0f;

		float anchoMaximo = 0.0f;
		float anchoActual = 0.0f;

		for (const char* p = texto; *p != '\0';)
		{
			if (*p == '\n')
			{
				if (anchoActual > anchoMaximo)
					anchoMaximo = anchoActual;

				anchoActual = 0.0f;
				++p;
				continue;
			}

			SiguienteCodigo(p);
			anchoActual += AnchoLetra(escala);
		}

		return anchoActual > anchoMaximo ? anchoActual : anchoMaximo;
	}

	// Parte el texto en renglones que quepan en `anchoMaximo` pixeles,
	// cortando en los espacios. Devuelve cuantos renglones quedaron.
	static int Envolver(
		const char* texto,
		float anchoMaximo,
		float escala,
		char* salida,
		size_t tamSalida)
	{
		if (salida == nullptr || tamSalida == 0)
			return 0;

		salida[0] = '\0';
		if (texto == nullptr)
			return 0;

		int maxLetras = (int)(anchoMaximo / AnchoLetra(escala));
		if (maxLetras < 4)
			maxLetras = 4;

		size_t escrito = 0;
		int lineas = 1;
		int letrasEnLinea = 0;
		const char* p = texto;

		while (*p != '\0')
		{
			if (*p == '\n')
			{
				if (escrito + 1 >= tamSalida) break;
				salida[escrito++] = '\n';
				letrasEnLinea = 0;
				++lineas;
				++p;
				continue;
			}

			// Mide la siguiente palabra (hasta espacio o salto).
			const char* fin = p;
			int letras = 0;
			while (*fin != '\0' && *fin != ' ' && *fin != '\n')
			{
				SiguienteCodigo(fin);
				++letras;
			}

			if (letrasEnLinea > 0 && letrasEnLinea + 1 + letras > maxLetras)
			{
				if (escrito + 1 >= tamSalida) break;
				salida[escrito++] = '\n';
				letrasEnLinea = 0;
				++lineas;
			}
			else if (letrasEnLinea > 0)
			{
				if (escrito + 1 >= tamSalida) break;
				salida[escrito++] = ' ';
				++letrasEnLinea;
			}

			const size_t bytes = (size_t)(fin - p);
			if (escrito + bytes >= tamSalida) break;
			memcpy(salida + escrito, p, bytes);
			escrito += bytes;
			letrasEnLinea += letras;

			p = fin;
			while (*p == ' ')
				++p;
		}

		salida[escrito] = '\0';
		return lineas;
	}

private:
	void EnviarVertices(std::vector<VerticeTexto>& vertices)
	{
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
		int caracter;
		unsigned char filas[7];
	};

	enum Acento { SinAcento, Agudo, Tilde, Dieresis };

	// Lee un caracter del texto y avanza el puntero. Acepta UTF-8
	// (como se guardan los .h) y tambien Latin-1 / Windows-1252 de un
	// solo byte, por si el compilador convierte los literales.
	// Devuelve el codigo Latin-1 (0..255) o '?' si no se puede mostrar.
	static int SiguienteCodigo(const char*& p)
	{
		const unsigned char b0 = (unsigned char)p[0];

		if (b0 < 0x80)
		{
			++p;
			return b0;
		}

		const unsigned char b1 = (unsigned char)p[1];

		if ((b0 & 0xE0) == 0xC0 && (b1 & 0xC0) == 0x80)
		{
			p += 2;
			const int codigo = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
			return codigo <= 0xFF ? codigo : '?';
		}

		if ((b0 & 0xF0) == 0xE0 && (b1 & 0xC0) == 0x80 && ((unsigned char)p[2] & 0xC0) == 0x80)
		{
			p += 3;
			return '?';
		}

		++p;
		return b0;
	}

	// La fuente solo tiene mayusculas: convierte minusculas (con acento
	// o sin el) a su mayuscula Latin-1.
	static int Mayuscula(int c)
	{
		if (c >= 'a' && c <= 'z')
			return c - 32;

		if (c >= 0xE0 && c <= 0xFE && c != 0xF7)
			return c - 32;

		return c;
	}

	// Letra base y acento de las vocales acentuadas, la Ñ y la Ü.
	static int SepararAcento(int c, Acento& acento)
	{
		acento = SinAcento;

		switch (c)
		{
		case 0xC1: acento = Agudo; return 'A';
		case 0xC9: acento = Agudo; return 'E';
		case 0xCD: acento = Agudo; return 'I';
		case 0xD3: acento = Agudo; return 'O';
		case 0xDA: acento = Agudo; return 'U';
		case 0xD1: acento = Tilde; return 'N';
		case 0xDC: acento = Dieresis; return 'U';
		default: return c;
		}
	}

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
			{ '"', { 0b01010, 0b01010, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 } },
			{ ';', { 0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b01000 } },
			{ '(', { 0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010 } },
			{ ')', { 0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000 } },
			{ '/', { 0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000 } },
			{ '+', { 0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000 } },
			{ '$', { 0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100 } },
			{ '%', { 0b11001, 0b11010, 0b00010, 0b00100, 0b01000, 0b01011, 0b10011 } },
			{ 0xBF, { 0b00100, 0b00000, 0b00100, 0b01000, 0b10000, 0b10001, 0b01110 } }, // ¿
			{ 0xA1, { 0b00100, 0b00000, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 } }, // ¡

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

	static const Glifo* BuscarGlifo(int c)
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
		int codigo,
		float x,
		float y,
		float escala,
		float anchoPantalla,
		float altoPantalla,
		const float color[4])
	{
		Acento acento = SinAcento;
		const int base = SepararAcento(Mayuscula(codigo), acento);
		const Glifo* glifo = BuscarGlifo(base);

		if (glifo == nullptr)
			return;

		// Acento en las 2 filas de arriba de la letra.
		if (acento != SinAcento)
		{
			static const unsigned char acentos[4][2] =
			{
				{ 0b00000, 0b00000 },
				{ 0b00010, 0b00100 }, // agudo
				{ 0b01101, 0b10010 }, // tilde
				{ 0b00000, 0b01010 }, // dieresis
			};

			for (int fila = 0; fila < 2; ++fila)
			{
				const unsigned char bits = acentos[acento][fila];

				for (int columna = 0; columna < 5; ++columna)
				{
					if ((bits & (1 << (4 - columna))) == 0)
						continue;

					AgregarRect(
						vertices,
						x + columna * escala,
						y + (fila - 2) * escala,
						x + (columna + 1) * escala,
						y + (fila - 1) * escala,
						anchoPantalla,
						altoPantalla,
						color);
				}
			}
		}

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
