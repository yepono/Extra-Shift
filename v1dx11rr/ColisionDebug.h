#ifndef _colision_debug
#define _colision_debug

// Dibuja las cajas de SistemaColision como alambre (lineas).
// Es solo para depurar; no participa en la simulacion.

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx10math.h>
#include <cstring>
#include <vector>

#include "Colision.h"

#pragma comment(lib, "d3dcompiler.lib")

class DepuradorColision
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
cbuffer Matrices : register(b0)
{
    row_major float4x4 vistaProyeccion;
};

struct VSInput
{
    float3 posicion : POSITION;
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
    salida.posicion = mul(float4(entrada.posicion, 1.0f), vistaProyeccion);
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
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
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

		D3D11_BUFFER_DESC constantDesc;
		ZeroMemory(&constantDesc, sizeof(constantDesc));
		constantDesc.Usage = D3D11_USAGE_DEFAULT;
		constantDesc.ByteWidth = sizeof(D3DXMATRIX);
		constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		result = m_device->CreateBuffer(&constantDesc, nullptr, &m_constantBuffer);
		if (FAILED(result))
		{
			Liberar();
			return false;
		}

		D3D11_BUFFER_DESC vertexDesc;
		ZeroMemory(&vertexDesc, sizeof(vertexDesc));
		vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexDesc.ByteWidth = sizeof(VerticeLinea) * MaxVertices;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		result = m_device->CreateBuffer(&vertexDesc, nullptr, &m_vertexBuffer);
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
		if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
		if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
		if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
		if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }

		m_listo = false;
		m_device = nullptr;
		m_context = nullptr;
	}

	// `colorFija` (RGBA) pinta todas las cajas del sistema con ese color.
	// Si es nullptr se usa el color segun las banderas solido/soporta.
	void Dibujar(
		const SistemaColision& sistema,
		const float* view,
		const float* projection,
		const float* colorFija = nullptr)
	{
		if (!m_listo || m_context == nullptr || view == nullptr || projection == nullptr)
			return;

		if (sistema.cajas.empty())
			return;

		std::vector<VerticeLinea> vertices;
		vertices.reserve(sistema.cajas.size() * 24);

		for (size_t i = 0; i < sistema.cajas.size(); ++i)
		{
			const CajaColision& caja = sistema.cajas[i];

			if (!caja.activa)
				continue;

			float color[4] = { 1.0f, 0.2f, 0.2f, 1.0f }; // solido

			if (colorFija != nullptr)
			{
				color[0] = colorFija[0];
				color[1] = colorFija[1];
				color[2] = colorFija[2];
				color[3] = colorFija[3];
			}
			else if (caja.soporta && caja.solido)
			{
				color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.0f;
			}
			else if (caja.soporta)
			{
				color[0] = 0.2f; color[1] = 1.0f; color[2] = 0.2f;
			}

			AgregarCaja(vertices, caja, color);
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
			vertices.size() * sizeof(VerticeLinea));

		m_context->Unmap(m_vertexBuffer, 0);

		D3DXMATRIX vista;
		D3DXMATRIX proyeccion;
		memcpy(&vista, view, sizeof(D3DXMATRIX));
		memcpy(&proyeccion, projection, sizeof(D3DXMATRIX));

		D3DXMATRIX vistaProyeccion;
		D3DXMatrixMultiply(&vistaProyeccion, &vista, &proyeccion);
		m_context->UpdateSubresource(
			m_constantBuffer, 0, nullptr, &vistaProyeccion, 0, 0);

		UINT stride = sizeof(VerticeLinea);
		UINT offset = 0;

		m_context->IASetInputLayout(m_inputLayout);
		m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		m_context->VSSetShader(m_vertexShader, nullptr, 0);
		m_context->PSSetShader(m_pixelShader, nullptr, 0);
		m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
		m_context->Draw((UINT)vertices.size(), 0);
	}

private:
	struct VerticeLinea
	{
		float x, y, z;
		float r, g, b, a;
	};

	static const size_t MaxVertices = 16384;

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;

	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;

	bool m_listo = false;

	static void AgregarArista(
		std::vector<VerticeLinea>& vertices,
		const D3DXVECTOR3& a,
		const D3DXVECTOR3& b,
		const float color[4])
	{
		VerticeLinea va = { a.x, a.y, a.z, color[0], color[1], color[2], color[3] };
		VerticeLinea vb = { b.x, b.y, b.z, color[0], color[1], color[2], color[3] };

		vertices.push_back(va);
		vertices.push_back(vb);
	}

	static void AgregarCaja(
		std::vector<VerticeLinea>& vertices,
		const CajaColision& caja,
		const float color[4])
	{
		D3DXVECTOR3 esquina[8];

		for (int i = 0; i < 8; ++i)
		{
			esquina[i] = D3DXVECTOR3(
				(i & 1) ? caja.MaxX() : caja.MinX(),
				(i & 2) ? caja.MaxY() : caja.MinY(),
				(i & 4) ? caja.MaxZ() : caja.MinZ());
		}

		static const int aristas[12][2] =
		{
			{ 0, 1 }, { 0, 2 }, { 0, 4 }, { 1, 3 },
			{ 1, 5 }, { 2, 3 }, { 2, 6 }, { 3, 7 },
			{ 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 },
		};

		for (int i = 0; i < 12; ++i)
		{
			AgregarArista(
				vertices,
				esquina[aristas[i][0]],
				esquina[aristas[i][1]],
				color);
		}
	}
};

#endif
