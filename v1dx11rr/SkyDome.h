#ifndef _skydome
#define _skydome

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx11tex.h>
#include <DxErr.h>
#include <D3Dcompiler.h>
#include <d3dx10math.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cwctype>
#include <fstream>
#include <string>
#include <vector>

struct SkyComponent
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 UV;
};

struct MatrixType
{
	D3DXMATRIX worldMatrix;
	D3DXMATRIX viewMatrix;
	D3DXMATRIX projMatrix;
	D3DXVECTOR4 valores;
};

class SkyDome
{
public:
	float gamepad;
private:
	ID3D11VertexShader* solidColorVS;
	ID3D11PixelShader* solidColorPS;

	ID3D11InputLayout* inputLayout;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	ID3D11ShaderResourceView* textura;
	ID3D11SamplerState* texSampler;
	bool texturaHDR;   // true: equirectangular .hdr, false: jpg/png con UV del domo
	float exposicion;  // multiplicador antes del tone mapping (solo HDR)

	ID3D11Buffer* matrixBufferCB;
	MatrixType* matrices;

	SkyComponent* vertices;
	int slices, stacks, cantIndex;
	short* indices;
	float radio;
	ID3D11Device** d3dDevice;
	ID3D11DeviceContext** d3dContext;

public:
	SkyDome(int slices, int stacks, float radio, ID3D11Device** d3dDevice,
		ID3D11DeviceContext** d3dContext, WCHAR* diffuseTex)
	{
		this->slices = slices;
		this->stacks = stacks;
		this->radio = radio;
		indices = NULL;
		vertices = NULL;
		solidColorVS = nullptr;
		solidColorPS = nullptr;
		inputLayout = nullptr;
		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		textura = nullptr;
		texSampler = nullptr;
		matrixBufferCB = nullptr;
		matrices = nullptr;
		texturaHDR = false;
		exposicion = 1.0f;
		this->d3dDevice = d3dDevice;
		this->d3dContext = d3dContext;
		LoadContent(diffuseTex);
	}

	~SkyDome()
	{
		UnloadContent();
	}


	bool CompileD3DShader(WCHAR* filePath, char* entry, char* shaderModel, ID3DBlob** buffer)
	{
		DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

		ID3DBlob* errorBuffer = 0;
		HRESULT result;

		result = D3DX11CompileFromFile(filePath, 0, 0, entry, shaderModel, shaderFlags,
			0, 0, buffer, &errorBuffer, 0);
		if (FAILED(result))
		{
			if (errorBuffer != 0)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}

		if (errorBuffer != 0)
			errorBuffer->Release();

		return true;
	}

	bool LoadContent(WCHAR* diffuseTex)
	{
		HRESULT d3dResult;

		ID3DBlob* vsBuffer = 0;

		bool compileResult = CompileD3DShader(L"Skydome.fx", "VS_Main", "vs_4_0", &vsBuffer);

		if (compileResult == false)
		{
			return false;
		}


		d3dResult = (*d3dDevice)->CreateVertexShader(vsBuffer->GetBufferPointer(),
			vsBuffer->GetBufferSize(), 0, &solidColorVS);

		if (FAILED(d3dResult))
		{

			if (vsBuffer)
				vsBuffer->Release();

			return false;
		}

		D3D11_INPUT_ELEMENT_DESC solidColorLayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		unsigned int totalLayoutElements = ARRAYSIZE(solidColorLayout);

		d3dResult = (*d3dDevice)->CreateInputLayout(solidColorLayout, totalLayoutElements,
			vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &inputLayout);

		vsBuffer->Release();

		if (FAILED(d3dResult))
		{
			return false;
		}

		ID3DBlob* psBuffer = 0;

		compileResult = CompileD3DShader(L"Skydome.fx", "PS_Main", "ps_4_0", &psBuffer);

		if (compileResult == false)
		{
			return false;
		}

		d3dResult = (*d3dDevice)->CreatePixelShader(psBuffer->GetBufferPointer(),
			psBuffer->GetBufferSize(), 0, &solidColorPS);

		psBuffer->Release();

		if (FAILED(d3dResult))
		{
			return false;
		}

		int cantVert = slices * stacks;
		vertices = new SkyComponent[cantVert];
		float dx = slices - 1.0f;
		float dy = stacks - 1.0f;
		float maxY = 0;

		for (int x = 0; x < slices; x++)
		{
			for (int y = 0; y < stacks; y++)
			{
				float u = (float)(((double)((dx - x)*0.5f) / dx) *
					sin(D3DX_PI * y * 2.0f / dx)) + 0.5f;
				float v = (float)(((double)((dy - x)*0.5f) / dy) *
					cos(D3DX_PI * y * 2.0 / dy)) + 0.5f;

				int indiceArreglo = x * slices + y;
				vertices[indiceArreglo].pos = D3DXVECTOR3((float)(radio*cos(((double)x / dx)* D3DX_PI / 2) *
					cos(2.0 * D3DX_PI * (double)y / dx)), (float)(radio*sin(((double)x / dx) * D3DX_PI / 2)),
					(float)(radio*cos(((double)x / dy) * D3DX_PI / 2)*sin(2.0f * D3DX_PI * (double)y / dy)));
				vertices[indiceArreglo].UV = D3DXVECTOR2(u, v);
			}
		}
		D3D11_BUFFER_DESC vertexDesc;
		ZeroMemory(&vertexDesc, sizeof(vertexDesc));
		vertexDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexDesc.ByteWidth = sizeof(SkyComponent) * cantVert;

		D3D11_SUBRESOURCE_DATA resourceData;
		ZeroMemory(&resourceData, sizeof(resourceData));
		resourceData.pSysMem = vertices;

		d3dResult = (*d3dDevice)->CreateBuffer(&vertexDesc, &resourceData, &vertexBuffer);

		if (FAILED(d3dResult))
		{
			MessageBox(0, L"Error", L"Error al crear vertex buffer", MB_OK);
			return false;
		}
        delete[] vertices;
		vertices = nullptr;

		creaIndices();

		if (!CargarTextura(diffuseTex, &textura, texturaHDR))
		{
			MessageBox(0, L"No se pudo cargar la textura del skydome", L"Error", MB_OK);
			return false;
		}

		D3D11_SAMPLER_DESC colorMapDesc;
		ZeroMemory(&colorMapDesc, sizeof(colorMapDesc));
		colorMapDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		colorMapDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		colorMapDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		colorMapDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		colorMapDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		colorMapDesc.MaxAnisotropy = 8;
		colorMapDesc.MaxLOD = D3D11_FLOAT32_MAX;

		d3dResult = (*d3dDevice)->CreateSamplerState(&colorMapDesc, &texSampler);

		if (FAILED(d3dResult))
		{
			return false;
		}


		D3D11_BUFFER_DESC constDesc;
		ZeroMemory(&constDesc, sizeof(constDesc));
		constDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constDesc.ByteWidth = sizeof(MatrixType);
		constDesc.Usage = D3D11_USAGE_DEFAULT;

		d3dResult = (*d3dDevice)->CreateBuffer(&constDesc, 0, &matrixBufferCB);

		if (FAILED(d3dResult))
		{
			return false;
		}
		matrices = new MatrixType;

		return true;
	}

	bool UnloadContent()
	{
		if (texSampler)
			texSampler->Release();
		if (textura)
			textura->Release();
		if (solidColorVS)
			solidColorVS->Release();
		if (solidColorPS)
			solidColorPS->Release();
		if (inputLayout)
			inputLayout->Release();
		if (vertexBuffer)
			vertexBuffer->Release();
		if (indexBuffer)
			indexBuffer->Release();
		if (matrixBufferCB)
			matrixBufferCB->Release();
		delete matrices;
		matrices = nullptr;

		texSampler = 0;
		textura = 0;
		solidColorVS = 0;
		solidColorPS = 0;
		inputLayout = 0;
		vertexBuffer = 0;
		indexBuffer = 0;
		matrixBufferCB = 0;

		return true;
	}

	void Update(D3DXMATRIX view, D3DXMATRIX projection)
	{
		if (matrices == nullptr)
			return;
		matrices->viewMatrix = view;
		matrices->projMatrix = projection;
	}

	void Render(D3DXVECTOR3 trans)
	{
		if (d3dContext == 0 || matrices == nullptr)
			return;

		unsigned int stride = sizeof(SkyComponent);
		unsigned int offset = 0;

		(*d3dContext)->IASetInputLayout(inputLayout);
		(*d3dContext)->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		(*d3dContext)->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
		(*d3dContext)->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		(*d3dContext)->VSSetShader(solidColorVS, 0, 0);
		(*d3dContext)->PSSetShader(solidColorPS, 0, 0);
		(*d3dContext)->PSSetShaderResources(0, 1, &textura);
		(*d3dContext)->PSSetSamplers(0, 1, &texSampler);

		D3DXMATRIX worldMat;
		const float bajadaDomo = 50.0f; // centro del domo bajo la camara
		D3DXMatrixTranslation(&worldMat, trans.x, trans.y - bajadaDomo, trans.z);
		D3DXMatrixTranspose(&worldMat, &worldMat);
		matrices->worldMatrix = worldMat;
		matrices->valores = D3DXVECTOR4(texturaHDR ? 1.0f : 0.0f, exposicion, bajadaDomo, 0.0f);

		(*d3dContext)->UpdateSubresource(matrixBufferCB, 0, 0, matrices, sizeof(MatrixType), 0);
		(*d3dContext)->VSSetConstantBuffers(0, 1, &matrixBufferCB);
		(*d3dContext)->PSSetConstantBuffers(0, 1, &matrixBufferCB);

		(*d3dContext)->DrawIndexed(cantIndex, 0, 0);
	}

	// Cambia la imagen del cielo sin reconstruir la malla.
	// Se usa al terminar el turno para pasar de dia a noche.
	bool ReemplazarTextura(const WCHAR* ruta)
	{
		if (d3dDevice == nullptr || *d3dDevice == nullptr || ruta == nullptr)
			return false;

		ID3D11ShaderResourceView* nueva = nullptr;
		bool nuevaEsHDR = false;
		if (!CargarTextura(ruta, &nueva, nuevaEsHDR))
			return false;

		if (textura)
			textura->Release();

		textura = nueva;
		texturaHDR = nuevaEsHDR;
		return true;
	}

	// Brillo del cielo HDR antes del tone mapping (1 = sin cambio).
	void SetExposicion(float valor)
	{
		exposicion = valor;
	}

private:
	// Carga .hdr con el lector RGBE propio; cualquier otro formato con D3DX11.
	bool CargarTextura(const WCHAR* ruta, ID3D11ShaderResourceView** srv, bool& esHDR)
	{
		*srv = nullptr;
		esHDR = EsArchivoHDR(ruta);

		if (!esHDR)
		{
			HRESULT r = D3DX11CreateShaderResourceViewFromFile(*d3dDevice, ruta, 0, 0, srv, 0);
			return SUCCEEDED(r) && *srv != nullptr;
		}

		int ancho = 0, alto = 0;
		std::vector<float> rgba;
		if (!LeerHDR(ruta, ancho, alto, rgba))
			return false;

		// float16 es filtrable en cualquier feature level (float32 RGBA no siempre)
		std::vector<D3DXFLOAT16> mitad(rgba.size());
		D3DXFloat32To16Array(mitad.data(), rgba.data(), (UINT)rgba.size());

		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = ancho;
		desc.Height = alto;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA datos;
		ZeroMemory(&datos, sizeof(datos));
		datos.pSysMem = mitad.data();
		datos.SysMemPitch = ancho * 4 * sizeof(D3DXFLOAT16);

		ID3D11Texture2D* tex = nullptr;
		if (FAILED((*d3dDevice)->CreateTexture2D(&desc, &datos, &tex)))
			return false;

		HRESULT r = (*d3dDevice)->CreateShaderResourceView(tex, nullptr, srv);
		tex->Release();
		return SUCCEEDED(r);
	}

	static bool EsArchivoHDR(const WCHAR* ruta)
	{
		std::wstring s(ruta);
		size_t punto = s.find_last_of(L'.');
		if (punto == std::wstring::npos)
			return false;
		std::wstring ext = s.substr(punto + 1);
		for (size_t i = 0; i < ext.size(); ++i)
			ext[i] = (wchar_t)towlower(ext[i]);
		return ext == L"hdr";
	}

	// Lector Radiance RGBE (.hdr): encabezado, resolucion "-Y alto +X ancho",
	// scanlines con RLE moderno o sin comprimir. Devuelve RGBA float lineal.
	static bool LeerHDR(const WCHAR* ruta, int& ancho, int& alto, std::vector<float>& rgba)
	{
		std::ifstream f(ruta, std::ios::binary);
		if (!f)
			return false;

		std::string linea;
		if (!std::getline(f, linea) || linea.compare(0, 2, "#?") != 0)
			return false;

		while (std::getline(f, linea))
		{
			if (!linea.empty() && linea.back() == '\r')
				linea.pop_back();
			if (linea.empty())
				break; // fin del encabezado
			if (linea.compare(0, 7, "FORMAT=") == 0 && linea != "FORMAT=32-bit_rle_rgbe")
				return false; // XYZE u otro formato no soportado
		}

		if (!std::getline(f, linea))
			return false;
		if (sscanf_s(linea.c_str(), "-Y %d +X %d", &alto, &ancho) != 2 || ancho <= 0 || alto <= 0)
			return false; // solo la orientacion estandar

		std::vector<uint8_t> scan((size_t)ancho * 4);
		rgba.assign((size_t)ancho * alto * 4, 0.0f);

		for (int y = 0; y < alto; ++y)
		{
			uint8_t cab[4];
			if (!f.read((char*)cab, 4))
				return false;

			const bool rleNuevo = ancho >= 8 && ancho < 0x8000 &&
				cab[0] == 2 && cab[1] == 2 && (cab[2] & 0x80) == 0;

			if (rleNuevo)
			{
				if (((cab[2] << 8) | cab[3]) != ancho)
					return false;

				// Cada canal (R, G, B, E) viene comprimido por separado
				for (int c = 0; c < 4; ++c)
				{
					int x = 0;
					while (x < ancho)
					{
						int cuenta = f.get();
						if (cuenta == EOF)
							return false;

						if (cuenta > 128)
						{
							cuenta -= 128;
							int valor = f.get();
							if (valor == EOF || cuenta > ancho - x)
								return false;
							while (cuenta-- > 0)
								scan[(size_t)(x++) * 4 + c] = (uint8_t)valor;
						}
						else
						{
							if (cuenta == 0 || cuenta > ancho - x)
								return false;
							while (cuenta-- > 0)
							{
								int valor = f.get();
								if (valor == EOF)
									return false;
								scan[(size_t)(x++) * 4 + c] = (uint8_t)valor;
							}
						}
					}
				}
			}
			else
			{
				// Scanline sin comprimir: los 4 bytes leidos son el primer pixel
				memcpy(scan.data(), cab, 4);
				if (ancho > 1 && !f.read((char*)scan.data() + 4, (std::streamsize)(ancho - 1) * 4))
					return false;
			}

			float* fila = &rgba[(size_t)y * ancho * 4];
			for (int x = 0; x < ancho; ++x)
			{
				const uint8_t* p = &scan[(size_t)x * 4];
				const float escala = p[3] ? (float)ldexp(1.0, p[3] - (128 + 8)) : 0.0f;
				fila[x * 4 + 0] = p[0] * escala;
				fila[x * 4 + 1] = p[1] * escala;
				fila[x * 4 + 2] = p[2] * escala;
				fila[x * 4 + 3] = 1.0f;
			}
		}

		return true;
	}

	void creaIndices()
	{
		cantIndex = (slices - 1)*(stacks - 1) * 6;
		indices = new short[cantIndex];
		int counter = 0;
		for (int i = 0; i < slices - 1; i++)
		{
			for (int j = 0; j < stacks - 1; j++)
			{
				int lowerLeft = j + i * stacks;
				int lowerRight = (j + 1) + i * stacks;
				int topLeft = j + (i + 1)*stacks;
				int topRight = (j + 1) + (i + 1)*stacks;

				indices[counter++] = lowerLeft;
				indices[counter++] = lowerRight;
				indices[counter++] = topLeft;

				indices[counter++] = lowerRight;
				indices[counter++] = topRight;
				indices[counter++] = topLeft;
			}
		}
		D3D11_BUFFER_DESC indexDesc;
		ZeroMemory(&indexDesc, sizeof(indexDesc));
		indexDesc.Usage = D3D11_USAGE_DEFAULT;
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexDesc.ByteWidth = sizeof(short) * cantIndex;
		indexDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA resourceData;
		ZeroMemory(&resourceData, sizeof(resourceData));
		resourceData.pSysMem = indices;

		HRESULT d3dResult = (*d3dDevice)->CreateBuffer(&indexDesc, &resourceData, &indexBuffer);

		if (FAILED(d3dResult))
		{
			return;
		}
     delete[] indices;
		indices = nullptr;
	}
};
#endif