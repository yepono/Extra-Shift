#ifndef _dxrr
#define _dxrr
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <d3dx10math.h>
#include "TerrenoRR.h"
#include "Camara.h"
#include "SkyDome.h"
#include "Billboard.h"
#include "PanelRR.h"
#include "ModeloRR.h"
#include "XACT3Util.h"
#include "AnimatedModel.h"
#include "Colision.h"
#include "ColisionDebug.h"
#include "TextoPantalla.h"
#include <cstdio>

class DXRR {

private:
	int ancho;
	int alto;
public:
	HINSTANCE hInstance;
	HWND hWnd;

	D3D_DRIVER_TYPE driverType;
	D3D_FEATURE_LEVEL featureLevel;

	ID3D11Device* d3dDevice;
	ID3D11DeviceContext* d3dContext;
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* backBufferTarget;

	ID3D11Texture2D* depthTexture;
	ID3D11DepthStencilView* depthStencilView;

	ID3D11DepthStencilState* depthStencilState;
	ID3D11DepthStencilState* depthStencilDisabledState;

	ID3D11BlendState* alphaBlendState, * commonBlendState;

	int frameBillboard;

	TerrenoRR* terreno;
	SkyDome* skydome;


	// articulos en estantes
	BillboardRR* billboard = nullptr;
	PanelRR paneles;

	ModeloRR* chavo3;
	ModeloRR* alien;
	ModeloRR* chava1;
	ModeloRR* chava2;
	ModeloRR* humano;



	ModeloRR* carrito;
	ModeloRR* banio;
	ModeloRR* cartel;
	ModeloRR* canasta;
	ModeloRR* mostrador;

	ModeloRR* refri_a1;
	ModeloRR* refri_a2;
	ModeloRR* refri_a3;
	ModeloRR* refri_a4;
	ModeloRR* refri_a5;
	ModeloRR* refri_a6;
	ModeloRR* refri_a7;
	ModeloRR* refri_a8;


	ModeloRR* refri_b1;

	ModeloRR* repisa_a1;
	ModeloRR* repisa_a2;
	ModeloRR* repisa_a3;
	ModeloRR* repisa_a4;
	ModeloRR* repisa_a5;
	ModeloRR* repisa_a6;
	ModeloRR* repisa_a7;
	ModeloRR* repisa_a8;
	ModeloRR* repisa_a9;
	ModeloRR* repisa_a10;
	ModeloRR* repisa_a11;
	ModeloRR* repisa_a12;


	ModeloRR* repisa_b1;
	ModeloRR* repisa_b2;
	ModeloRR* repisa_b3;
	ModeloRR* repisa_b4;



	ModeloRR* bar;
	ModeloRR* hotel;
	ModeloRR* oxxo;

	// vehiculo
	ModeloRR* ovni;


	// FBX alien
	AnimatedModel* alien_anim;
	bool alienAnimLoaded = false;


	// Relacionado a la camara
	float moveBackForward;
	float moveLeftRight;
	float camYaw;
	float camPitch;

	bool thirdPerson;
	D3DXVECTOR3 playerPosition;

	// Colisiones del mundo y depuracion visual (F1).
	SistemaColision colisiones;

	// Cajas que solo se dibujan con F1 (edificios grandes, jugador)
	SistemaColision colisionesDepuracion;
	size_t indiceCajaJugadorDepuracion = 0;

	DepuradorColision depuradorColision;
	bool mostrarColisiones = true;

	// Coordenadas del jugador en pantalla (F2).
	TextoPantalla textoPantalla;
	bool mostrarCoordenadas = true;

	// Volumen del jugador: radio en XZ y altura en Y.
	float radioJugador = 0.2f;
	float alturaJugador = 3.0f;

	Camara* camara;


	bool breakpoint;

	XACTINDEX cueIndex;
	CXACT3Util m_XACT3;

	DXRR(HWND hWnd, int Ancho, int Alto)
	{
		breakpoint = false;
		frameBillboard = 0;
		ancho = Ancho;
		alto = Alto;
		driverType = D3D_DRIVER_TYPE_NULL;
		featureLevel = D3D_FEATURE_LEVEL_11_0;
		d3dDevice = 0;
		d3dContext = 0;
		swapChain = 0;
		backBufferTarget = 0;
		IniciaD3D(hWnd);
		depuradorColision.Inicializar(d3dDevice, d3dContext);
		textoPantalla.Inicializar(d3dDevice, d3dContext);

		moveBackForward = 0.0f;
		moveLeftRight = 0.0f;
		camYaw = 0.0f;
		camPitch = 0.0f;

		thirdPerson = false;
		playerPosition = D3DXVECTOR3(0.0f, 2.0f, 0.0f);

		// carga de billboards
		// articulos en los estantes
		//billboard = new BillboardRR(L"Assets/Billboards/cokita.png", L"Assets/Billboards/cokita.png", d3dDevice, d3dContext, 1);
		billboard = new BillboardRR();
		billboard->Inicializar(d3dDevice, d3dContext);
		paneles.Inicializar(d3dDevice, d3dContext);


		// Camera-facing billboard: texture, x, z, y, width.
		billboard->AgregarConAspecto(
			L"Assets/Billboards/cokita.png",
			-26.0f, 4.0f, 20.0f, 1.0f);

		// Fixed panel: texture, x, y, z, width, rotation in degrees.
		//  a sprite that must not turn.
		paneles.AgregarConAspecto(
			L"Assets/Billboards/cokita.png",
			-26.0f, 20.0f, 4.0f, 1.0f, 180.0f);


		camara = new Camara(
			D3DXVECTOR3(0.0f, 80.0f, 6.0f),
			D3DXVECTOR3(0.0f, 80.0f, 0.0f),
			D3DXVECTOR3(0.0f, 1.0f, 0.0f),
			Ancho,
			Alto);
		terreno = new TerrenoRR(500, 500, d3dDevice, d3dContext);
		skydome = new SkyDome(32, 32, 100.0f, &d3dDevice, &d3dContext, L"Skydome.jpg");

		// Personajes del escenario
		chavo3 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/Persona/chavo3.obj", L"Assets/Modelos/Persona/chavo3.jpeg", L"Assets/Modelos/Persona/chavo3_specular_map.png", -90, 30);
		alien = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/Persona/alien.obj", L"Assets/Modelos/Persona/alien.jpeg", L"Assets/Modelos/Persona/alien_specular_map.png", -58, 30);
		chava1 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/Persona/chava1.obj", L"Assets/Modelos/Persona/chava1.jpeg", L"Assets/Modelos/Persona/alien_specular_map.png", -15, 15);
		chava2 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/Persona/chava2.obj", L"Assets/Modelos/Persona/chava2.jpeg", L"Assets/Modelos/Persona/alien_specular_map.png", -40, 50);


		// objetos del escenario

		carrito = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/carrito/carrito.obj", L"Assets/Modelos/objetos/carrito/carrito2.png", L"Assets/Modelos/objetos/carrito/carrito1.png", 5, 5);
		cartel = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/cartel/cartel.obj", L"Assets/Modelos/objetos/cartel/Material_45_baseColor.png", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", 7, 7);
		canasta = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/canasta.obj", L"Assets/Modelos/objetos/canasta.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", 9, 9);

		refri_a1 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -18, 20);
		refri_a2 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -22, 20);
		refri_a3 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -26, 20);
		refri_a4 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -30, 20);
		refri_a5 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -32, 20);
		refri_a5 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -36, 20);
		refri_a6 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -40, 20);
		refri_a7 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-1.obj", L"Assets/Modelos/objetos/refri-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -44, 20);


		refri_b1 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/refri-2.obj", L"Assets/Modelos/objetos/refri-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -13, 25);


		//repisa_b1 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-1.obj", L"Assets/Modelos/objetos/repisa-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -30, 25);
		repisa_b2 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-1.obj", L"Assets/Modelos/objetos/repisa-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -17, 40);
		//repisa_b3 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-1.obj", L"Assets/Modelos/objetos/repisa-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -36, 35);
		//repisa_b4 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-1.obj", L"Assets/Modelos/objetos/repisa-1.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -39, 35);

		repisa_a1 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -20, 35);
		repisa_a2 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -24, 35);
		repisa_a3 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -28, 35);
		repisa_a4 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -32, 35);
		repisa_a5 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -36, 35);
		repisa_a6 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -40, 35);
		repisa_a7 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -32, 30);
		repisa_a8 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -36, 30);
		repisa_a9 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -40, 30);
		repisa_a10 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -30, 25);
		repisa_a11 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -34, 25);
		repisa_a12 = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/repisa-2.obj", L"Assets/Modelos/objetos/repisa-2.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -38, 25);


		mostrador = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/mostrador.obj", L"Assets/Modelos/objetos/mostrador.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -53, 30);
		banio = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/banio/banio.obj", L"Assets/Modelos/objetos/banio/banio1.png", L"Assets/Modelos/objetos/banio/banio2.png", -5.0f, 8.0f);
		bar = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/bar.obj", L"Assets/Modelos/objetos/bar3.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", 13, 28);
		hotel = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/hotel.obj", L"Assets/Modelos/objetos/hotel.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -310, -125);
		oxxo = new ModeloRR(d3dDevice, d3dContext, "Assets/Modelos/objetos/oxxo.obj", L"Assets/Modelos/objetos/oxxo.jpeg", L"Assets/Modelos/objetos/cartel/Material_45_normal.png", -35, 25);


		// prueba de fbx
		alien_anim = new AnimatedModel(d3dDevice, d3dContext, -40.0f, 51.0f);
		alienAnimLoaded =
			alien_anim->Load("Assets/Modelos/Persona/alien2.fbx");

		if (alienAnimLoaded)
		{
			alien_anim->LoadAnimationFile(
				"Assets/Modelos/Persona/alien_idle.fbx",
				"Idle");

			alien_anim->LoadAnimationFile(
				"Assets/Modelos/Persona/alien_walk.fbx",
				"Walk");

			alien_anim->LoadAnimationFile(
				"Assets/Modelos/Persona/alien_pickup.fbx",
				"Pickup");

			alien_anim->PlayAnimation("Idle");
		}

		playerPosition = D3DXVECTOR3(
			alien_anim->GetPosX(),
			terreno->Superficie(alien_anim->GetPosX(), alien_anim->GetPosZ()),
			alien_anim->GetPosZ());

		// Registra las cajas del escenario.
		ConfigurarColisiones();

		// Deduce el volumen del jugador a partir de la malla del personaje.
		float anchoJugador = 0.0f;
		float altoModeloJugador = 0.0f;
		float fondoJugador = 0.0f;

		alien_anim->GetDimensionesMundo(
			D3DXToRadian(0.0f),
			D3DXToRadian(-90.0f),
			D3DXToRadian(-90.0f),
			0.28f,
			anchoJugador,
			altoModeloJugador,
			fondoJugador);

		if (altoModeloJugador > 0.1f)
		{
			alturaJugador = altoModeloJugador;
			if (alturaJugador < 1.0f) alturaJugador = 1.0f;
			if (alturaJugador > 8.0f) alturaJugador = 8.0f;

			const float huellaMayor =
				(anchoJugador > fondoJugador) ? anchoJugador : fondoJugador;

			radioJugador = huellaMayor * 0.35f;

			if (radioJugador < 0.3f)
				radioJugador = 0.3f;
			if (radioJugador > 2.0f)
				radioJugador = 2.0f;
		}

		camara->ConfigurarColision(&colisiones, radioJugador, alturaJugador);

		// Caja del jugador solo para debug
		indiceCajaJugadorDepuracion = colisionesDepuracion.AgregarCajaPersonaje(
			playerPosition.x,
			playerPosition.z,
			playerPosition.y,
			radioJugador,
			alturaJugador,
			false,
			"jugador");

		thirdPerson = true;
	}


	~DXRR()
	{
		delete alien_anim;
		alien_anim = nullptr;
		depuradorColision.Liberar();
		textoPantalla.Liberar();
		delete billboard;
		billboard = nullptr;
		paneles.Liberar();

		LiberaD3D();
		LiberaD3D();
		m_XACT3.Terminate();
	}

	bool IniciaD3D(HWND hWnd)
	{
		this->hInstance = hInstance;
		this->hWnd = hWnd;

		//obtiene el ancho y alto de la ventana donde se dibuja
		RECT dimensions;
		GetClientRect(hWnd, &dimensions);
		unsigned int width = dimensions.right - dimensions.left;
		unsigned int heigth = dimensions.bottom - dimensions.top;

		//Las formas en como la pc puede ejecutar el DX11, la mas rapida es D3D_DRIVER_TYPE_HARDWARE pero solo se puede usar cuando lo soporte el hardware
		//otra opcion es D3D_DRIVER_TYPE_WARP que emula el DX11 en los equipos que no lo soportan
		//la opcion menos recomendada es D3D_DRIVER_TYPE_SOFTWARE, es la mas lenta y solo es util cuando se libera una version de DX que no sea soportada por hardware
		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_SOFTWARE
		};
		unsigned int totalDriverTypes = ARRAYSIZE(driverTypes);

		//La version de DX que utilizara, en este caso el DX11
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
		};
		unsigned int totalFeaturesLevels = ARRAYSIZE(featureLevels);

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = heigth;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.Windowed = true;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;

		HRESULT result;
		unsigned int driver = 0, creationFlags = 0;
		for (driver = 0; driver < totalDriverTypes; driver++)
		{
			result = D3D11CreateDeviceAndSwapChain(0, driverTypes[driver], 0,
				creationFlags, featureLevels, totalFeaturesLevels,
				D3D11_SDK_VERSION, &swapChainDesc, &swapChain,
				&d3dDevice, &featureLevel, &d3dContext);

			if (SUCCEEDED(result))
			{
				driverType = driverTypes[driver];
				break;
			}
		}

		if (FAILED(result))
		{

			//Error al crear el Direct3D device
			return false;
		}

		ID3D11Texture2D* backBufferTexture;
		result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture);
		if (FAILED(result))
		{
			//"Error al crear el swapChainBuffer
			return false;
		}

		result = d3dDevice->CreateRenderTargetView(backBufferTexture, 0, &backBufferTarget);
		if (backBufferTexture)
			backBufferTexture->Release();

		if (FAILED(result))
		{
			//Error al crear el renderTargetView
			return false;
		}


		D3D11_VIEWPORT viewport;
		viewport.Width = (FLOAT)width;
		viewport.Height = (FLOAT)heigth;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		d3dContext->RSSetViewports(1, &viewport);

		D3D11_TEXTURE2D_DESC depthTexDesc;
		ZeroMemory(&depthTexDesc, sizeof(depthTexDesc));
		depthTexDesc.Width = width;
		depthTexDesc.Height = heigth;
		depthTexDesc.MipLevels = 1;
		depthTexDesc.ArraySize = 1;
		depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthTexDesc.SampleDesc.Count = 1;
		depthTexDesc.SampleDesc.Quality = 0;
		depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
		depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthTexDesc.CPUAccessFlags = 0;
		depthTexDesc.MiscFlags = 0;

		result = d3dDevice->CreateTexture2D(&depthTexDesc, NULL, &depthTexture);
		if (FAILED(result))
		{
			MessageBox(0, L"Error", L"Error al crear la DepthTexture", MB_OK);
			return false;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = depthTexDesc.Format;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		result = d3dDevice->CreateDepthStencilView(depthTexture, &descDSV, &depthStencilView);
		if (FAILED(result))
		{
			MessageBox(0, L"Error", L"Error al crear el depth stencil target view", MB_OK);
			return false;
		}

		d3dContext->OMSetRenderTargets(1, &backBufferTarget, depthStencilView);

		return true;

	}

	void LiberaD3D(void)
	{
		if (depthTexture)
			depthTexture->Release();
		if (depthStencilView)
			depthStencilView->Release();
		if (backBufferTarget)
			backBufferTarget->Release();
		if (swapChain)
			swapChain->Release();
		if (d3dContext)
			d3dContext->Release();
		if (d3dDevice)
			d3dDevice->Release();

		depthTexture = 0;
		depthStencilView = 0;
		d3dDevice = 0;
		d3dContext = 0;
		swapChain = 0;
		backBufferTarget = 0;
	}

	void Render(void)
	{
		float sphere[3] = { 0,0,0 };
		float prevPos[3] = { camara->Position.x, camara->Position.z, camara->Position.z };
		static float angle = 0.0f;
		angle += 0.005;
		if (angle >= 360) angle = 0.0f;
		bool collide = false;
		if (d3dContext == 0)
			return;

		float clearColor[4] = { 0, 0, 0, 1.0f };
		d3dContext->ClearRenderTargetView(backBufferTarget, clearColor);
		d3dContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// La camara ya se actualizo en Update(); aqui solo se dibuja.

		// The shaders multiply row vectors by their matrices.  Transpose the
		// CPU-side view and projection matrices before uploading them.
		D3DXMATRIX gpuView;
		D3DXMATRIX gpuProjection;
		D3DXMatrixTranspose(&gpuView, &camara->View);
		D3DXMatrixTranspose(&gpuProjection, &camara->Projection);
		skydome->Update(gpuView, gpuProjection);

		float camPosXZ[2] = { camara->Position.x, camara->Position.z };

		TurnOffDepth();
		skydome->Render(camara->Position);
		TurnOnDepth();
		terreno->Draw(gpuView, gpuProjection);
		TurnOnAlphaBlending();
		//billboard->Draw(camara->View, camara->Projection, camara->Position,
			//-26, 4, 20, 1, uv1, uv2, uv3, uv4, frameBillboard);

		TurnOffAlphaBlending();
		chavo3->Draw(gpuView, gpuProjection, terreno->Superficie(-90, 30), camara->Position, 10.0f, 0, 'A', 0.5);
		alien->Draw(gpuView, gpuProjection, terreno->Superficie(-58, 30), camara->Position, 10.0f, 0, 'A', 0.5);
		chava1->Draw(gpuView, gpuProjection, terreno->Superficie(-15, 15), camara->Position, 10.0f, 0, 'A', 0.5);
		chava2->Draw(gpuView, gpuProjection, terreno->Superficie(-40, 50), camara->Position, 180.0f, 0, 'X', 0.5);


		carrito->Draw(gpuView, gpuProjection, terreno->Superficie(5, 5), camara->Position, 10.0f, 0, 'A', 1);
		cartel->Draw(gpuView, gpuProjection, terreno->Superficie(7, 7), camara->Position, 10.0f, 0, 'A', 0.5);
		canasta->Draw(gpuView, gpuProjection, terreno->Superficie(9, 9), camara->Position, 10.0f, 0, 'A', 0.5);

		refri_a1->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);
		refri_a2->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);
		refri_a3->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);
		refri_a4->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);
		refri_a5->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);
		refri_a6->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);
		refri_a7->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.5);



		refri_b1->Draw(gpuView, gpuProjection, terreno->Superficie(-26, 20) + 1.0f, camara->Position, 10.0f, -65, 'Y', 1.5);

		//repisa_b1->Draw(gpuView, gpuProjection, terreno->Superficie(-30.0f, 25.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_b2->Draw(gpuView, gpuProjection, terreno->Superficie(-17.0f, 40.0f) + 1.0f, camara->Position, 10.0f, 0, 'Y', 1.2);
		//repisa_b3->Draw(gpuView, gpuProjection, terreno->Superficie(-36.0f, 35.0f) + 1.0f, camara->Position, 10.0f, -170, 'Y', 1.2);
		//repisa_b4->Draw(gpuView, gpuProjection, terreno->Superficie(-39.0f, 35.0f) + 1.0f, camara->Position, 10.0f, -45, 'Y', 1.2);

		repisa_a1->Draw(gpuView, gpuProjection, terreno->Superficie(-36.0f, 35.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a2->Draw(gpuView, gpuProjection, terreno->Superficie(-40.0f, 35.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a3->Draw(gpuView, gpuProjection, terreno->Superficie(-44.0f, 35.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a4->Draw(gpuView, gpuProjection, terreno->Superficie(-32.0f, 30.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a5->Draw(gpuView, gpuProjection, terreno->Superficie(-36.0f, 30.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a6->Draw(gpuView, gpuProjection, terreno->Superficie(-30.0f, 25.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a7->Draw(gpuView, gpuProjection, terreno->Superficie(-34.0f, 25.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a8->Draw(gpuView, gpuProjection, terreno->Superficie(-38.0f, 25.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a9->Draw(gpuView, gpuProjection, terreno->Superficie(-20.0f, 35.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a10->Draw(gpuView, gpuProjection, terreno->Superficie(-24.0f, 35.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a11->Draw(gpuView, gpuProjection, terreno->Superficie(-40.0f, 30.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);
		repisa_a12->Draw(gpuView, gpuProjection, terreno->Superficie(-40.0f, 30.0f) + 1.0f, camara->Position, 10.0f, 0, 'A', 1.2);

		mostrador->Draw(gpuView, gpuProjection, terreno->Superficie(0, 0) + 1.0f, camara->Position, 10.0f, -165.0f, 'Y', 1.5);
		banio->Draw(gpuView, gpuProjection, terreno->Superficie(-5.0f, 8.0f), camara->Position, 10.0f, 0, 'A', 3);
		bar->Draw(gpuView, gpuProjection, terreno->Superficie(13, 28), camara->Position, 10.0f, 0.0f, 'A', 3.5);
		hotel->Draw(gpuView, gpuProjection, terreno->Superficie(-195, -50), camara->Position, 10.0f, 0, 'A', 2.5);
		oxxo->Draw(gpuView, gpuProjection, terreno->Superficie(-35, 25), camara->Position, 10.0f, 0, 'A', 2.3);


		if (billboard != nullptr)
			billboard->Dibujar(camara->View, camara->Projection, camara->Position);

		paneles.Dibujar(camara->View, camara->Projection);


		// prueba fbx
		if (alienAnimLoaded && alien_anim != nullptr)
		{
			alien_anim->Draw(
				&camara->View._11,
				&camara->Projection._11,
				playerPosition.y,
				camara->Yaw,
				D3DXToRadian(0.0f),
				D3DXToRadian(-90.0f),
				D3DXToRadian(-90.0f),
				0.28f,
				0.0f);
		}

		if (mostrarColisiones)
		{
			// Sin depth: asi las cajas se ven aunque esten dentro de un
			// edificio o tapadas por la malla del modelo.
			if (depthStencilDisabledState != nullptr)
				d3dContext->OMSetDepthStencilState(depthStencilDisabledState, 1);

			// Cajas reales: rojo = solida, verde = solo piso,
			// amarillo = solida y piso.
			depuradorColision.Dibujar(
				colisiones,
				&camara->View._11,
				&camara->Projection._11);

			// Cajas de modelos que no participan en la simulacion
			// (edificios grandes y volumen del jugador) en cian.
			const float colorDepuracion[4] = { 0.2f, 0.8f, 1.0f, 1.0f };
			depuradorColision.Dibujar(
				colisionesDepuracion,
				&camara->View._11,
				&camara->Projection._11,
				colorDepuracion);

			if (depthStencilState != nullptr)
				d3dContext->OMSetDepthStencilState(depthStencilState, 1);
		}

		// Coordenadas del jugador para colocar cajas (F2).
		if (mostrarCoordenadas)
		{
			char textoCoordenadas[160];
			sprintf_s(
				textoCoordenadas,
				"JUGADOR\nX: %.2f   Z: %.2f\nY: %.2f",
				playerPosition.x,
				playerPosition.z,
				playerPosition.y);

			textoPantalla.Dibujar(textoCoordenadas);
		}

		// Present()
		if (swapChain != nullptr)
		{
			swapChain->Present(1, 0);
		}
	}

	void Update(float deltaTime)
	{
		const bool isMoving =
			fabsf(moveBackForward) > 0.001f ||
			fabsf(moveLeftRight) > 0.001f;

		// Applies the input once and clears the per-frame input values.
		UpdateCameraFromInput();

		// Altura del jugador: terreno + pisos registrados en el sistema
		// de colision. Despues se corrige la camara para que no se quede
		// un frame abajo.
		{
			const float alturaAnterior = playerPosition.y;

			playerPosition.y = AlturaSueloJugador(
				playerPosition.x,
				playerPosition.z,
				playerPosition.y);

			camara->DesplazarVertical(
				playerPosition.y - alturaAnterior,
				thirdPerson);
		}

		// Mantiene la caja de depuracion del jugador sobre su volumen real.
		colisionesDepuracion.ActualizarCaja(
			indiceCajaJugadorDepuracion,
			D3DXVECTOR3(
				playerPosition.x,
				playerPosition.y + alturaJugador * 0.5f,
				playerPosition.z));

		// Keep the character model in sync with the player in both modes.
		if (alienAnimLoaded && alien_anim != nullptr)
		{
			alien_anim->SetPosition(
				playerPosition.x,
				playerPosition.z);

			const char* desiredAnimation = isMoving ? "Walk" : "Idle";
			alien_anim->PlayAnimation(desiredAnimation);

			alien_anim->Update(deltaTime);
		}
	}
	// Altura del suelo en una posicion: el terreno o, si hay un piso
	// registrado mas alto, la cara superior de ese piso.
	float AlturaSueloJugador(float x, float z, float alturaActual)
	{
		const float alturaTerreno = terreno->Superficie(x, z);

		return colisiones.AlturaSuelo(
			x,
			z,
			alturaActual,
			alturaTerreno);
	}

	void AgregarCajaModelo(
		ModeloRR* modelo,
		float y,
		float rot,
		char eje,
		float escala,
		const char* nombre)
	{
		if (modelo == nullptr)
			return;

		modelo->AgregarCajaColision(
			colisiones,
			y,
			rot,
			eje,
			escala,
			true,   // solido
			false,  // no es piso
			nombre);
	}

	// Registra la caja de un modelo solo en el sistema de depuracion.
	// No bloquea al jugador; solo se dibuja con F1.
	void AgregarCajaModeloDepuracion(
		ModeloRR* modelo,
		float y,
		float rot,
		char eje,
		float escala,
		const char* nombre)
	{
		if (modelo == nullptr)
			return;

		modelo->AgregarCajaColision(
			colisionesDepuracion,
			y,
			rot,
			eje,
			escala,
			false,  // no bloquea
			false,  // no soporta
			nombre);
	}

	// Registro de las cajas del escenario. Los parametros y, rotacion,
	// eje y escala deben coincidir con los que usa Render() para cada
	// modelo; asi la caja queda exactamente donde se ve el objeto.
	void ConfigurarColisiones()
	{
		colisiones.Limpiar();
		colisionesDepuracion.Limpiar();

		// Escalon maximo que el jugador puede subir sin saltar.
		colisiones.pasoMaximo = 2.0f;

		// Personajes estaticos 
		AgregarCajaModelo(chavo3, terreno->Superficie(-90, 30), 0.0f, 'A', 0.5f, "chavo3");
		AgregarCajaModelo(alien, terreno->Superficie(-58, 30), 0.0f, 'A', 0.5f, "alien_obj");
		AgregarCajaModelo(chava1, terreno->Superficie(-15, 15), 0.0f, 'A', 0.5f, "chava1");
		AgregarCajaModelo(chava2, terreno->Superficie(-26, 20), 180.0f, 'X', 0.5f, "chava2");

		// Objetos 
		AgregarCajaModelo(carrito, terreno->Superficie(5, 5), 0.0f, 'A', 1.0f, "carrito");
		AgregarCajaModelo(cartel, terreno->Superficie(7, 7), 0.0f, 'A', 0.5f, "cartel");
		AgregarCajaModelo(canasta, terreno->Superficie(9, 9), 0.0f, 'A', 0.5f, "canasta");

		AgregarCajaModelo(refri_a1, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a1");
		AgregarCajaModelo(refri_a2, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a2");
		AgregarCajaModelo(refri_a3, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a3");
		AgregarCajaModelo(refri_a4, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a4");
		AgregarCajaModelo(refri_a5, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a5");
		AgregarCajaModelo(refri_a6, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a6");
		AgregarCajaModelo(refri_a7, terreno->Superficie(-26, 20) + 1.0f, 0.0f, 'A', 1.5f, "refri_a7");


		AgregarCajaModelo(refri_b1, terreno->Superficie(-15, 25) + 1.0f, -35.0f, 'Y', 1.5f, "refri_b1");

		//AgregarCajaModelo(repisa_b1, terreno->Superficie(-30.0f, 25.0f) + 1.0f, 90.0f, 'A', 1.2f, "repisa_b1");
		AgregarCajaModelo(repisa_b2, terreno->Superficie(-17.0f, 40.0f) + 1.0f, 0.0f, 'Y', 1.2f, "repisa_b2");
		//AgregarCajaModelo(repisa_b3, terreno->Superficie(-36.0f, 35.0f) + 1.0f, -90.0f, 'Y', 1.2f, "repisa_b3");
		//AgregarCajaModelo(repisa_b4, terreno->Superficie(-39.0f, 35.0f) + 1.0f, -90.0f, 'Y', 1.2f, "repisa_b4");


		AgregarCajaModelo(repisa_a1, terreno->Superficie(-36.0f, 35.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a1");
		AgregarCajaModelo(repisa_a2, terreno->Superficie(-40.0f, 35.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a2");
		AgregarCajaModelo(repisa_a3, terreno->Superficie(-44.0f, 35.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a3");
		AgregarCajaModelo(repisa_a4, terreno->Superficie(-32.0f, 30.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a4");
		AgregarCajaModelo(repisa_a5, terreno->Superficie(-36.0f, 30.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a5");
		AgregarCajaModelo(repisa_a6, terreno->Superficie(-30.0f, 25.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a6");
		AgregarCajaModelo(repisa_a7, terreno->Superficie(-34.0f, 25.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a7");
		AgregarCajaModelo(repisa_a8, terreno->Superficie(-38.0f, 25.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a8");
		AgregarCajaModelo(repisa_a9, terreno->Superficie(-20.0f, 35.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a9");
		AgregarCajaModelo(repisa_a10, terreno->Superficie(-24.0f, 35.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a10");
		AgregarCajaModelo(repisa_a11, terreno->Superficie(-40.0f, 30.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a11");
		AgregarCajaModelo(repisa_a12, terreno->Superficie(-40.0f, 30.0f) + 1.0f, 0.0f, 'A', 1.2f, "repisa_a12");

		AgregarCajaModelo(banio, terreno->Superficie(-5.0f, 8.0f), 0.0f, 'A', 3.0f, "banio");
		//AgregarCajaModelo(ovni, terreno->Superficie(-80, 30), 0.0f, 'A', 2.5f, "ovni");

		// Edificios
		AgregarCajaModelo(bar, terreno->Superficie(13, 28), 0.0f, 'A', 3.5f, "bar");
		AgregarCajaModelo(hotel, terreno->Superficie(-195, -50), 0.0f, 'A', 2.5f, "hotel");


		colisiones.AgregarPiso(-35, 25, 12.5f, 50.0f, 45.0f, 0.5f, "piso_edificio");

		// cajas en coordenadas locales del modelo:

		// pared neveras y refris
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-18.0f, 0.0f, -8.0f),
			D3DXVECTOR3(25.0f, 10.0f, -7.0f),
			terreno->Superficie(-35, 25),
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"pared_neveras_refri");

		// pared de puerta
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-25.0f, 0.0f, -2.0f),
			D3DXVECTOR3(-15.0f, 10.0f, -3.0f),
			terreno->Superficie(-35, 25),
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"pared_puerta");

		// nevera 
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-15.0f, 0.0f, -2.0f),
			D3DXVECTOR3(-12.0f, 5.0f, -7.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"nevera");

		// mostrador
		mostrador->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(1.0f, 0.0f, -2.0f),
			D3DXVECTOR3(0.0f, 3.5f, 2.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"mostrador");

		mostrador->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(2.0f, 0.0f, 3.0f),
			D3DXVECTOR3(-1.0f, 3.5f, 2.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"mostrador");


		// Estante oxxo
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(13.0f, 0.0f, 6.0f),
			D3DXVECTOR3(7.0f, 6.0f, 4.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"estante");

		// entrada der oxxo
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-2.0f, 0.0f, 19.0f),
			D3DXVECTOR3(-25.0f, 6.0f, 20.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"entrada_der");

		// entrada izq oxxo
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(3.0f, 0.0f, 19.0f),
			D3DXVECTOR3(25.0f, 6.0f, 20.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"entrada_izq");

		// bote de basura izq
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(4.0f, 0.0f, 20.0f),
			D3DXVECTOR3(6.0f, 3.0f, 21.5f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"bote_izq");


		// bote de basura der
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-3.0f, 0.0f, 20.0f),
			D3DXVECTOR3(-5.0f, 3.0f, 21.5f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"bote_der");

		// pared izq
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(25.0f, 0.0f, -21.0f),
			D3DXVECTOR3(26.0f, 6.0f, 20.5f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"pared_izq");

		// pared der
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-25.0f, 0.0f, -21.0f),
			D3DXVECTOR3(-26.0f, 6.0f, 20.5f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"pared_der");

		// barda der
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(-29.0f, 0.0f, -30.0f),
			D3DXVECTOR3(-30.0f, 6.0f, 35.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"barda_der");


		// salida trasera
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(25.0f, 0.0f, -20.0f),
			D3DXVECTOR3(-25.0f, 6.0f, -19.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"salida_trasera");

		// barda trasera
		oxxo->AgregarCajaLocal(colisiones,
			D3DXVECTOR3(44.0f, 0.0f, -30.0f),
			D3DXVECTOR3(-30.0f, 6.0f, -29.0f),
			terreno->Superficie(-35, 25) + 1.0f,
			0.0f,
			'A',
			1.0f,
			true,
			false,
			"barda_trasera");



	}

	bool isPointInsideSphere(float* point, float* sphere) {
		bool collition = false;

		float distance = sqrt((point[0] - sphere[0]) * (point[0] - sphere[0]) +
			(point[1] - sphere[1]) * (point[1] - sphere[1]));

		if (distance < sphere[2])
			collition = true;
		return collition;
	}

	//Activa el alpha blend para dibujar con transparencias
	void TurnOnAlphaBlending()
	{
		float blendFactor[4];
		blendFactor[0] = 0.0f;
		blendFactor[1] = 0.0f;
		blendFactor[2] = 0.0f;
		blendFactor[3] = 0.0f;
		HRESULT result;

		D3D11_BLEND_DESC descABSD;
		ZeroMemory(&descABSD, sizeof(D3D11_BLEND_DESC));
		descABSD.RenderTarget[0].BlendEnable = TRUE;
		descABSD.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		descABSD.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		descABSD.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		descABSD.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		descABSD.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		descABSD.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		descABSD.RenderTarget[0].RenderTargetWriteMask = 0x0f;

		result = d3dDevice->CreateBlendState(&descABSD, &alphaBlendState);
		if (FAILED(result))
		{
			MessageBox(0, L"Error", L"Error al crear el blend state", MB_OK);
			return;
		}

		d3dContext->OMSetBlendState(alphaBlendState, blendFactor, 0xffffffff);
	}

	//Regresa al blend normal(solido)
	void TurnOffAlphaBlending()
	{
		D3D11_BLEND_DESC descCBSD;
		ZeroMemory(&descCBSD, sizeof(D3D11_BLEND_DESC));
		descCBSD.RenderTarget[0].BlendEnable = FALSE;
		descCBSD.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		descCBSD.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		descCBSD.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		descCBSD.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		descCBSD.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		descCBSD.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		descCBSD.RenderTarget[0].RenderTargetWriteMask = 0x0f;
		HRESULT result;

		result = d3dDevice->CreateBlendState(&descCBSD, &commonBlendState);
		if (FAILED(result))
		{
			MessageBox(0, L"Error", L"Error al crear el blend state", MB_OK);
			return;
		}

		d3dContext->OMSetBlendState(commonBlendState, NULL, 0xffffffff);
	}

	void TurnOnDepth()
	{
		D3D11_DEPTH_STENCIL_DESC descDSD;
		ZeroMemory(&descDSD, sizeof(descDSD));
		descDSD.DepthEnable = true;
		descDSD.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		descDSD.DepthFunc = D3D11_COMPARISON_LESS;
		descDSD.StencilEnable = true;
		descDSD.StencilReadMask = 0xFF;
		descDSD.StencilWriteMask = 0xFF;
		descDSD.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		descDSD.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		descDSD.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		descDSD.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		descDSD.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		descDSD.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		descDSD.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		descDSD.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		d3dDevice->CreateDepthStencilState(&descDSD, &depthStencilState);

		d3dContext->OMSetDepthStencilState(depthStencilState, 1);
	}

	void TurnOffDepth()
	{
		D3D11_DEPTH_STENCIL_DESC descDDSD;
		ZeroMemory(&descDDSD, sizeof(descDDSD));
		descDDSD.DepthEnable = false;
		descDDSD.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		descDDSD.DepthFunc = D3D11_COMPARISON_LESS;
		descDDSD.StencilEnable = true;
		descDDSD.StencilReadMask = 0xFF;
		descDDSD.StencilWriteMask = 0xFF;
		descDDSD.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		descDDSD.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		descDDSD.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		descDDSD.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		descDDSD.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		descDDSD.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		descDDSD.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		descDDSD.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		d3dDevice->CreateDepthStencilState(&descDDSD, &depthStencilDisabledState);
		d3dContext->OMSetDepthStencilState(depthStencilDisabledState, 1);
	}

	void UpdateCameraFromInput()
	{
		if (thirdPerson)
		{
			camara->UpdateThirdPerson(
				playerPosition,
				moveBackForward,
				moveLeftRight,
				camYaw,
				camPitch,
				10.0f,
				1.5f
			);
		}
		else
		{
			const float firstPersonEyeHeight = 4.0f;

			camara->UpdateFirstPerson(
				playerPosition,
				moveBackForward,
				moveLeftRight,
				camYaw,
				camPitch,
				firstPersonEyeHeight
			);
		}

		// input por frame
		moveBackForward = 0.0f;
		moveLeftRight = 0.0f;
		camYaw = 0.0f;
		camPitch = 0.0f;
	}

};
#endif