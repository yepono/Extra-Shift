#ifndef _charcos_agua
#define _charcos_agua

// Charcos de agua en el piso (como si acabaran de trapear).
//
// Cada charco es un quad horizontal un poco arriba del piso. El pixel
// shader le da forma irregular con ruido, ondas suaves que se mueven,
// reflejo tipo fresnel y brillos de las lamparas del techo. Se dibuja con
// mezcla alfa, probando profundidad pero sin escribirla.
//
// Uso:
//   CharcosAgua charcos;
//   charcos.Inicializar(device, context);
//   charcos.Agregar(x, y, z, radioX, radioZ);
//   charcos.Dibujar(vista, proyeccion, posCamara, tiempo);   // matrices de CPU

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx10math.h>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

class CharcosAgua
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
    float4 centro;      // xyz = centro del charco, w = tiempo
    float4 tamano;      // x = radio X, y = radio Z, z = semilla, w = giro
    float4 camara;      // xyz = posicion de la camara
};

struct VSInput
{
    float2 esquina : POSITION;   // -1..1
};

struct PSInput
{
    float4 posicion : SV_POSITION;
    float2 local    : TEXCOORD0;  // -1..1 dentro del charco
    float3 mundo    : TEXCOORD1;
};

PSInput VSMain(VSInput entrada)
{
    PSInput salida;

    float s = sin(tamano.w);
    float c = cos(tamano.w);
    float2 p = entrada.esquina * tamano.xy;
    float2 r = float2(p.x * c - p.y * s, p.x * s + p.y * c);

    float3 mundo = centro.xyz + float3(r.x, 0.0, r.y);
    salida.posicion = mul(float4(mundo, 1.0), vistaProyeccion);
    salida.local = entrada.esquina;
    salida.mundo = mundo;
    return salida;
}

float Hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float Ruido(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);

    float a = Hash(i);
    float b = Hash(i + float2(1.0, 0.0));
    float c = Hash(i + float2(0.0, 1.0));
    float d = Hash(i + float2(1.0, 1.0));

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float Fbm(float2 p)
{
    float valor = 0.0;
    float amplitud = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        valor += Ruido(p) * amplitud;
        p *= 2.03;
        amplitud *= 0.5;
    }
    return valor;
}

float4 PSMain(PSInput entrada) : SV_Target
{
    float tiempo = centro.w;
    float semilla = tamano.z;

    // Forma irregular: el borde se deforma con ruido.
    float2 q = entrada.local;
    float distancia = length(q) + (Fbm(q * 1.7 + semilla) - 0.5) * 0.7;
    float forma = smoothstep(0.95, 0.70, distancia);
    if (forma <= 0.001)
        discard;

    // Ondas suaves que mueven la normal del agua.
    float2 ondaUV = entrada.mundo.xz * 1.3 + semilla;
    float n1 = Ruido(ondaUV + float2(tiempo * 0.35, tiempo * 0.22));
    float n2 = Ruido(ondaUV * 1.9 - float2(tiempo * 0.27, -tiempo * 0.31));
    float anillo = sin(length(q) * 18.0 - tiempo * 2.2) * 0.5 + 0.5;
    float3 normal = normalize(float3(
        (n1 - 0.5) * 0.18 + (anillo - 0.5) * 0.03 * q.x,
        1.0,
        (n2 - 0.5) * 0.18 + (anillo - 0.5) * 0.03 * q.y));

    float3 alOjo = normalize(camara.xyz - entrada.mundo);

    // Fresnel: casi transparente viendo hacia abajo, espejo de lado.
    float fresnel = 0.04 + 0.96 * pow(1.0 - saturate(dot(normal, alOjo)), 5.0);

    // "Reflejo" del techo del Oxxo: gris claro azulado.
    float3 reflejado = reflect(-alOjo, normal);
    float3 colorTecho = lerp(float3(0.55, 0.60, 0.66), float3(0.92, 0.95, 1.0), saturate(reflejado.y));

    // Brillo de las lamparas (luz desde arriba).
    float3 luz = normalize(float3(0.25, 1.0, 0.15));
    float brillo = pow(saturate(dot(reflejado, luz)), 90.0) * 1.4;

    float3 colorAgua = float3(0.42, 0.52, 0.60);
    float3 color = lerp(colorAgua, colorTecho, fresnel) + brillo;

    // Orilla un poco mas oscura (piso mojado) y centro mas claro.
    float orilla = smoothstep(0.55, 0.92, distancia);
    color *= lerp(1.0, 0.80, orilla);

    float alfa = forma * (0.32 + 0.45 * fresnel) + brillo * 0.5;
    return float4(saturate(color), saturate(alfa));
}
)";

		ID3DBlob* vsBlob = nullptr;
		ID3DBlob* psBlob = nullptr;

		if (!Compilar(shaderSource, "VSMain", "vs_5_0", &vsBlob))
			return false;

		if (!Compilar(shaderSource, "PSMain", "ps_5_0", &psBlob))
		{
			vsBlob->Release();
			return false;
		}

		HRESULT result = m_device->CreateVertexShader(
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);

		if (SUCCEEDED(result))
		{
			result = m_device->CreatePixelShader(
				psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
		}

		if (SUCCEEDED(result))
		{
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			result = m_device->CreateInputLayout(
				layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
		}

		vsBlob->Release();
		psBlob->Release();

		if (SUCCEEDED(result))
		{
			const float esquinas[12] =
			{
				-1.0f, -1.0f,   1.0f, -1.0f,   1.0f, 1.0f,
				-1.0f, -1.0f,   1.0f, 1.0f,   -1.0f, 1.0f,
			};

			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.ByteWidth = sizeof(esquinas);
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA datos;
			ZeroMemory(&datos, sizeof(datos));
			datos.pSysMem = esquinas;

			result = m_device->CreateBuffer(&desc, &datos, &m_vertexBuffer);
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
			D3D11_BLEND_DESC blendDesc;
			ZeroMemory(&blendDesc, sizeof(blendDesc));
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			result = m_device->CreateBlendState(&blendDesc, &m_blendState);
		}

		if (SUCCEEDED(result))
		{
			// Prueba profundidad (los muebles tapan el charco) sin escribirla.
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
		if (m_constantes) { m_constantes->Release(); m_constantes = nullptr; }
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

	// y = altura del piso; el charco se levanta un poco para no parpadear.
	void Agregar(float x, float y, float z, float radioX, float radioZ, float giro = 0.0f)
	{
		Charco charco;
		charco.x = x;
		charco.y = y + 0.03f;
		charco.z = z;
		charco.radioX = radioX;
		charco.radioZ = radioZ;
		charco.giro = giro;
		charco.semilla = 13.7f * (float)(m_charcos.size() + 1);
		m_charcos.push_back(charco);
	}

	void Vaciar() { m_charcos.clear(); }

	// vista/proyeccion: las matrices de la camara tal cual (sin transponer).
	void Dibujar(
		const D3DXMATRIX& vista,
		const D3DXMATRIX& proyeccion,
		const D3DXVECTOR3& posCamara,
		float tiempo)
	{
		if (!m_listo || m_context == nullptr || m_charcos.empty())
			return;

		D3DXMATRIX vistaProyeccion;
		D3DXMatrixMultiply(&vistaProyeccion, &vista, &proyeccion);

		const UINT stride = sizeof(float) * 2;
		const UINT offset = 0;
		const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		m_context->IASetInputLayout(m_inputLayout);
		m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->VSSetShader(m_vertexShader, nullptr, 0);
		m_context->PSSetShader(m_pixelShader, nullptr, 0);
		m_context->VSSetConstantBuffers(0, 1, &m_constantes);
		m_context->PSSetConstantBuffers(0, 1, &m_constantes);
		m_context->OMSetBlendState(m_blendState, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(m_depthState, 0);
		m_context->RSSetState(m_rasterizerState);

		for (size_t i = 0; i < m_charcos.size(); ++i)
		{
			const Charco& c = m_charcos[i];

			Constantes cb;
			cb.vistaProyeccion = vistaProyeccion;
			cb.centro = D3DXVECTOR4(c.x, c.y, c.z, tiempo);
			cb.tamano = D3DXVECTOR4(c.radioX, c.radioZ, c.semilla, c.giro);
			cb.camara = D3DXVECTOR4(posCamara.x, posCamara.y, posCamara.z, 1.0f);

			m_context->UpdateSubresource(m_constantes, 0, nullptr, &cb, 0, 0);
			m_context->Draw(6, 0);
		}

		// Deja estados normales para lo que se dibuje despues.
		m_context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
		m_context->OMSetDepthStencilState(nullptr, 0);
		m_context->RSSetState(nullptr);
	}

private:
	struct Charco
	{
		float x, y, z;
		float radioX, radioZ;
		float giro;
		float semilla;
	};

	struct Constantes
	{
		D3DXMATRIX vistaProyeccion;   // row_major en el shader
		D3DXVECTOR4 centro;
		D3DXVECTOR4 tamano;
		D3DXVECTOR4 camara;
	};

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

	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_constantes = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11DepthStencilState* m_depthState = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;
	std::vector<Charco> m_charcos;
	bool m_listo = false;
};

#endif
