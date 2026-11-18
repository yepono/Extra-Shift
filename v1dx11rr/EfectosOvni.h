#ifndef _efectos_ovni
#define _efectos_ovni

// Efectos de la nave del alien, con shaders propios:
//  - Aura: un resplandor suave alrededor del ovni (billboard que siempre ve
//    a la camara, con anillo que late).
//  - Rayo de abduccion: un cono de luz verde desde la panza de la nave
//    hasta el piso, con franjas que suben.
// Ambos se dibujan con mezcla aditiva y sin escribir profundidad.
//
// Uso:
//   EfectosOvni efectos;
//   efectos.Inicializar(device, context);
//   efectos.DibujarAura(vista, proyeccion, centro, radio, tiempo, intensidad);
//   efectos.DibujarRayo(vista, proyeccion, arriba, radioArriba, piso, radioPiso, tiempo, intensidad);

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx10math.h>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

class EfectosOvni
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
cbuffer Constantes : register(b0)
{
    row_major float4x4 vistaProyeccion;
    float4 centro;     // xyz: centro (aura) o punta de arriba (rayo); w: tiempo
    float4 medidas;    // aura: x = radio | rayo: x = radio arriba, y = radio abajo, z = alto
    float4 derecha;    // xyz: derecha de la camara; w: intensidad
    float4 arriba;     // xyz: arriba de la camara
    float4 color;
};

struct VSInput
{
    float2 dato : POSITION;   // aura: esquina -1..1 | rayo: (angulo 0..1, t 0..1)
};

struct PSInput
{
    float4 posicion : SV_POSITION;
    float2 dato     : TEXCOORD0;
};

PSInput VSAura(VSInput entrada)
{
    PSInput salida;
    float3 mundo = centro.xyz +
        (derecha.xyz * entrada.dato.x + arriba.xyz * entrada.dato.y) * medidas.x;
    salida.posicion = mul(float4(mundo, 1.0), vistaProyeccion);
    salida.dato = entrada.dato;
    return salida;
}

float4 PSAura(PSInput entrada) : SV_Target
{
    float tiempo = centro.w;
    float d = length(entrada.dato);
    if (d > 1.0)
        discard;

    // Resplandor que se apaga hacia afuera + anillo que late.
    float halo = pow(saturate(1.0 - d), 2.2);
    float pulso = 0.75 + 0.25 * sin(tiempo * 3.0);
    float anillo = exp(-pow((d - 0.62 - 0.05 * sin(tiempo * 2.0)) * 9.0, 2.0)) * 0.6;

    float fuerza = (halo * 0.8 + anillo) * pulso * derecha.w;
    return float4(color.rgb * fuerza, fuerza);
}

PSInput VSRayo(VSInput entrada)
{
    PSInput salida;
    float angulo = entrada.dato.x * 6.2831853;
    float t = entrada.dato.y;
    float radio = lerp(medidas.x, medidas.y, t);

    float3 mundo = centro.xyz + float3(cos(angulo) * radio, -medidas.z * t, sin(angulo) * radio);
    salida.posicion = mul(float4(mundo, 1.0), vistaProyeccion);
    salida.dato = entrada.dato;
    return salida;
}

float4 PSRayo(PSInput entrada) : SV_Target
{
    float tiempo = centro.w;
    float t = entrada.dato.y;

    // Franjas que suben hacia la nave + parpadeo suave.
    float franjas = 0.55 + 0.45 * sin(t * 26.0 + tiempo * 9.0);
    float brillo = 0.8 + 0.2 * sin(tiempo * 17.0);

    // Mas fuerte junto a la nave y abajo, mas suave en medio.
    float vertical = lerp(1.0, 0.55, t) + smoothstep(0.85, 1.0, t) * 0.5;

    float fuerza = franjas * brillo * vertical * 0.55 * derecha.w;
    return float4(color.rgb * fuerza, fuerza);
}
)";

		ID3DBlob* vsAura = nullptr;
		ID3DBlob* psAura = nullptr;
		ID3DBlob* vsRayo = nullptr;
		ID3DBlob* psRayo = nullptr;

		bool ok =
			Compilar(shaderSource, "VSAura", "vs_5_0", &vsAura) &&
			Compilar(shaderSource, "PSAura", "ps_5_0", &psAura) &&
			Compilar(shaderSource, "VSRayo", "vs_5_0", &vsRayo) &&
			Compilar(shaderSource, "PSRayo", "ps_5_0", &psRayo);

		HRESULT result = ok ? S_OK : E_FAIL;

		if (SUCCEEDED(result))
			result = m_device->CreateVertexShader(vsAura->GetBufferPointer(), vsAura->GetBufferSize(), nullptr, &m_vsAura);
		if (SUCCEEDED(result))
			result = m_device->CreatePixelShader(psAura->GetBufferPointer(), psAura->GetBufferSize(), nullptr, &m_psAura);
		if (SUCCEEDED(result))
			result = m_device->CreateVertexShader(vsRayo->GetBufferPointer(), vsRayo->GetBufferSize(), nullptr, &m_vsRayo);
		if (SUCCEEDED(result))
			result = m_device->CreatePixelShader(psRayo->GetBufferPointer(), psRayo->GetBufferSize(), nullptr, &m_psRayo);

		if (SUCCEEDED(result))
		{
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			result = m_device->CreateInputLayout(
				layout, 1, vsAura->GetBufferPointer(), vsAura->GetBufferSize(), &m_inputLayout);
		}

		if (vsAura) vsAura->Release();
		if (psAura) psAura->Release();
		if (vsRayo) vsRayo->Release();
		if (psRayo) psRayo->Release();

		// Quad del aura.
		if (SUCCEEDED(result))
		{
			const float quad[12] =
			{
				-1.0f, -1.0f,   1.0f, -1.0f,   1.0f, 1.0f,
				-1.0f, -1.0f,   1.0f, 1.0f,   -1.0f, 1.0f,
			};
			result = CrearVertices(quad, sizeof(quad), &m_vbAura);
		}

		// Cono del rayo: tubo de Segmentos x 1 en (angulo, t).
		if (SUCCEEDED(result))
		{
			std::vector<float> cono;
			for (int i = 0; i < Segmentos; ++i)
			{
				const float a0 = (float)i / Segmentos;
				const float a1 = (float)(i + 1) / Segmentos;

				const float tri[12] =
				{
					a0, 0.0f,   a1, 0.0f,   a1, 1.0f,
					a0, 0.0f,   a1, 1.0f,   a0, 1.0f,
				};
				cono.insert(cono.end(), tri, tri + 12);
			}
			result = CrearVertices(cono.data(), (UINT)(cono.size() * sizeof(float)), &m_vbRayo);
		}

		if (SUCCEEDED(result))
		{
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.ByteWidth = sizeof(Constantes);
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			result = m_device->CreateBuffer(&desc, nullptr, &m_constantes);
		}

		if (SUCCEEDED(result))
		{
			// Aditivo: la luz suma, nunca oscurece.
			D3D11_BLEND_DESC blendDesc;
			ZeroMemory(&blendDesc, sizeof(blendDesc));
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			result = m_device->CreateBlendState(&blendDesc, &m_blendState);
		}

		if (SUCCEEDED(result))
		{
			D3D11_DEPTH_STENCIL_DESC depthDesc;
			ZeroMemory(&depthDesc, sizeof(depthDesc));
			depthDesc.DepthEnable = TRUE;
			depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			result = m_device->CreateDepthStencilState(&depthDesc, &m_depthState);
		}

		if (SUCCEEDED(result))
		{
			D3D11_RASTERIZER_DESC rasterDesc;
			ZeroMemory(&rasterDesc, sizeof(rasterDesc));
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.CullMode = D3D11_CULL_NONE;
			rasterDesc.DepthClipEnable = TRUE;
			result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterizerState);
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
		Soltar(m_constantes);
		Soltar(m_vbAura);
		Soltar(m_vbRayo);
		Soltar(m_rasterizerState);
		Soltar(m_depthState);
		Soltar(m_blendState);
		Soltar(m_inputLayout);
		Soltar(m_vsAura);
		Soltar(m_psAura);
		Soltar(m_vsRayo);
		Soltar(m_psRayo);

		m_listo = false;
		m_device = nullptr;
		m_context = nullptr;
	}

	// vista/proyeccion: matrices de la camara sin transponer.
	void DibujarAura(
		const D3DXMATRIX& vista,
		const D3DXMATRIX& proyeccion,
		const D3DXVECTOR3& centro,
		float radio,
		float tiempo,
		float intensidad)
	{
		if (!m_listo || intensidad <= 0.0f)
			return;

		Constantes cb = ArmarConstantes(vista, proyeccion, centro, tiempo, intensidad);
		cb.medidas = D3DXVECTOR4(radio, 0.0f, 0.0f, 0.0f);
		cb.color = D3DXVECTOR4(0.35f, 1.0f, 0.55f, 1.0f);

		Preparar(m_vbAura, m_vsAura, m_psAura, cb);
		m_context->Draw(6, 0);
		Terminar();
	}

	// arriba: punta del cono bajo la nave; piso: altura donde termina.
	void DibujarRayo(
		const D3DXMATRIX& vista,
		const D3DXMATRIX& proyeccion,
		const D3DXVECTOR3& arriba,
		float radioArriba,
		float piso,
		float radioPiso,
		float tiempo,
		float intensidad)
	{
		const float alto = arriba.y - piso;
		if (!m_listo || intensidad <= 0.0f || alto <= 0.01f)
			return;

		Constantes cb = ArmarConstantes(vista, proyeccion, arriba, tiempo, intensidad);
		cb.medidas = D3DXVECTOR4(radioArriba, radioPiso, alto, 0.0f);
		cb.color = D3DXVECTOR4(0.25f, 1.0f, 0.35f, 1.0f);

		Preparar(m_vbRayo, m_vsRayo, m_psRayo, cb);
		m_context->Draw(Segmentos * 6, 0);
		Terminar();
	}

private:
	static const int Segmentos = 40;

	struct Constantes
	{
		D3DXMATRIX vistaProyeccion;
		D3DXVECTOR4 centro;
		D3DXVECTOR4 medidas;
		D3DXVECTOR4 derecha;
		D3DXVECTOR4 arriba;
		D3DXVECTOR4 color;
	};

	Constantes ArmarConstantes(
		const D3DXMATRIX& vista,
		const D3DXMATRIX& proyeccion,
		const D3DXVECTOR3& centro,
		float tiempo,
		float intensidad)
	{
		Constantes cb;
		D3DXMatrixMultiply(&cb.vistaProyeccion, &vista, &proyeccion);
		cb.centro = D3DXVECTOR4(centro.x, centro.y, centro.z, tiempo);

		// En la matriz de vista (vectores fila) las columnas 1 y 2 son la
		// derecha y el arriba de la camara en el mundo.
		cb.derecha = D3DXVECTOR4(vista._11, vista._21, vista._31, intensidad);
		cb.arriba = D3DXVECTOR4(vista._12, vista._22, vista._32, 0.0f);
		return cb;
	}

	void Preparar(ID3D11Buffer* vb, ID3D11VertexShader* vs, ID3D11PixelShader* ps, const Constantes& cb)
	{
		const UINT stride = sizeof(float) * 2;
		const UINT offset = 0;
		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_context->UpdateSubresource(m_constantes, 0, nullptr, &cb, 0, 0);
		m_context->IASetInputLayout(m_inputLayout);
		m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->VSSetShader(vs, nullptr, 0);
		m_context->PSSetShader(ps, nullptr, 0);
		m_context->VSSetConstantBuffers(0, 1, &m_constantes);
		m_context->PSSetConstantBuffers(0, 1, &m_constantes);
		m_context->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(m_depthState, 0);
		m_context->RSSetState(m_rasterizerState);
	}

	void Terminar()
	{
		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(nullptr, 0);
		m_context->RSSetState(nullptr);
	}

	HRESULT CrearVertices(const float* datos, UINT bytes, ID3D11Buffer** buffer)
	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = bytes;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA inicial;
		ZeroMemory(&inicial, sizeof(inicial));
		inicial.pSysMem = datos;

		return m_device->CreateBuffer(&desc, &inicial, buffer);
	}

	bool Compilar(const char* fuente, const char* entrada, const char* perfil, ID3DBlob** blob)
	{
		ID3DBlob* errores = nullptr;
		const HRESULT result = D3DCompile(
			fuente, strlen(fuente), nullptr, nullptr, nullptr,
			entrada, perfil, 0, 0, blob, &errores);

		if (errores != nullptr)
		{
			OutputDebugStringA((char*)errores->GetBufferPointer());
			errores->Release();
		}

		return SUCCEEDED(result);
	}

	template <typename T>
	static void Soltar(T*& recurso)
	{
		if (recurso != nullptr)
		{
			recurso->Release();
			recurso = nullptr;
		}
	}

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;
	ID3D11VertexShader* m_vsAura = nullptr;
	ID3D11PixelShader* m_psAura = nullptr;
	ID3D11VertexShader* m_vsRayo = nullptr;
	ID3D11PixelShader* m_psRayo = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_vbAura = nullptr;
	ID3D11Buffer* m_vbRayo = nullptr;
	ID3D11Buffer* m_constantes = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11DepthStencilState* m_depthState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	bool m_listo = false;
};

#endif
