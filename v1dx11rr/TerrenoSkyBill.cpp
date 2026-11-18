#include <windows.h>
#include <windowsx.h>

#include "DXRR.h"
#include "GamePadRR.h"
#include <dinput.h>
#include <xinput.h>
#include <cmath>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

// La ventana ocupa la pantalla real del monitor. Antes era 1920x1080 fijo
// y en monitores mas chicos (p. ej. 1366x768) el HUD quedaba fuera.
#define SCREEN_X GetSystemMetrics(SM_CXSCREEN)
#define SCREEN_Y GetSystemMetrics(SM_CYSCREEN)

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DXRR *dxrr;
GamePadRR *gamePad;
tagPOINT initialPoint;
tagPOINT actualPoint;
LPDIRECTINPUT8 m_pDirectInput = NULL;
LPDIRECTINPUTDEVICE8 m_pKeyboardDevice = NULL;
LPDIRECTINPUTDEVICE8 m_pMouseDevice = NULL;
bool cKeyWasDown = false;

/*void TestAssimp()
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("test.obj", aiProcess_Triangulate);
}*/

void createMouseDevice(HWND hWnd)
{
    HRESULT result;

    result = m_pDirectInput->CreateDevice(
        GUID_SysMouse,
        &m_pMouseDevice,
        0
    );

    if (FAILED(result))
        return;

    result = m_pMouseDevice->SetDataFormat(
        &c_dfDIMouse
    );

    if (FAILED(result))
        return;

    result = m_pMouseDevice->SetCooperativeLevel(
        hWnd,
        DISCL_EXCLUSIVE |
        DISCL_FOREGROUND
    );

    if (FAILED(result))
        return;

    m_pMouseDevice->Acquire();
}

void createKeyboardDevice(HWND hWnd) {
    m_pDirectInput->CreateDevice(GUID_SysKeyboard, &m_pKeyboardDevice, 0);

    m_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);

    m_pKeyboardDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

    m_pKeyboardDevice->Acquire();

}

void ProcessInput(HWND hWnd, float deltaTime)
{
    if (!dxrr)
        return;

    BYTE keyboardData[256] = {};
    DIMOUSESTATE mouseData = {};

    HRESULT keyboardResult =
        m_pKeyboardDevice->GetDeviceState(
            sizeof(keyboardData),
            keyboardData
        );

    if (FAILED(keyboardResult))
    {
        m_pKeyboardDevice->Acquire();
        return;
    }

    HRESULT mouseResult =
        m_pMouseDevice->GetDeviceState(
            sizeof(mouseData),
            &mouseData
        );

    if (FAILED(mouseResult))
    {
        m_pMouseDevice->Acquire();
    }

    // limpiar valores del frame anterior.
    dxrr->moveBackForward = 0.0f;
    dxrr->moveLeftRight = 0.0f;
    dxrr->camYaw = 0.0f;
    dxrr->camPitch = 0.0f;
    dxrr->moveUpDown = 0.0f;

    float movementSpeed = 30.0f;      // units per second
    const float keyboardLookSpeed = 1.5f;   // radians per second

    // movimiento WASD

    if (keyboardData[DIK_W] & 0x80)
        dxrr->moveBackForward += movementSpeed * deltaTime;

    if (keyboardData[DIK_S] & 0x80)
        dxrr->moveBackForward -= movementSpeed * deltaTime;

    if (keyboardData[DIK_A] & 0x80)
        dxrr->moveLeftRight -= movementSpeed * deltaTime;

    if (keyboardData[DIK_D] & 0x80)
        dxrr->moveLeftRight += movementSpeed * deltaTime;

    // Nave: ESPACIO sube, SHIFT baja (solo se usa al volar)
    const float velocidadSubida = 18.0f;

    if (keyboardData[DIK_SPACE] & 0x80)
        dxrr->moveUpDown += velocidadSubida * deltaTime;

    if ((keyboardData[DIK_LSHIFT] & 0x80) || (keyboardData[DIK_RSHIFT] & 0x80))
        dxrr->moveUpDown -= velocidadSubida * deltaTime;

    // Rotacion de la camara

    if (keyboardData[DIK_LEFT] & 0x80)
        dxrr->camYaw -= keyboardLookSpeed * deltaTime;

    if (keyboardData[DIK_RIGHT] & 0x80)
        dxrr->camYaw += keyboardLookSpeed * deltaTime;

    if (keyboardData[DIK_UP] & 0x80)
        dxrr->camPitch -= keyboardLookSpeed * deltaTime;

    if (keyboardData[DIK_DOWN] & 0x80)
        dxrr->camPitch += keyboardLookSpeed * deltaTime;

    // Toggle tercera persona con c

    bool cKeyIsDown =
        (keyboardData[DIK_C] & 0x80) != 0;

    if (cKeyIsDown && !cKeyWasDown && !dxrr->CamaraBloqueada())
    {
        // playerPosition is the single source of truth for both camera modes,
        // so toggling only changes how the camera is placed around it.
        dxrr->thirdPerson = !dxrr->thirdPerson;
    }

    cKeyWasDown = cKeyIsDown;

    // Teclas del turno: solo cuentan en el frame en que se presionan.
    static bool teclaAnterior[256] = {};

    auto Presionada = [&](int dik) -> bool
        {
            const bool ahora = (keyboardData[dik] & 0x80) != 0;
            const bool flanco = ahora && !teclaAnterior[dik];
            teclaAnterior[dik] = ahora;
            return flanco;
        };

    if (Presionada(DIK_E))
    {
        dxrr->teclaInteractuar = true;
        dxrr->teclaConfirmar = true;
    }

    const bool enter = Presionada(DIK_RETURN);
    const bool enterNumerico = Presionada(DIK_NUMPADENTER);
    if (enter || enterNumerico)
        dxrr->teclaConfirmar = true;

    const bool uno = Presionada(DIK_1);
    const bool unoNumerico = Presionada(DIK_NUMPAD1);
    const bool dos = Presionada(DIK_2);
    const bool dosNumerico = Presionada(DIK_NUMPAD2);
    const bool tres = Presionada(DIK_3);
    const bool tresNumerico = Presionada(DIK_NUMPAD3);

    if (uno || unoNumerico) dxrr->teclaOpcion = 1;
    if (dos || dosNumerico) dxrr->teclaOpcion = 2;
    if (tres || tresNumerico) dxrr->teclaOpcion = 3;

    if (Presionada(DIK_R))
        dxrr->teclaReiniciar = true;

    // Mostrar u ocultar las cajas de colision (F1)

    static bool f1WasDown = false;

    bool f1IsDown =
        (keyboardData[DIK_F1] & 0x80) != 0;

    if (f1IsDown && !f1WasDown)
    {
        dxrr->mostrarColisiones = !dxrr->mostrarColisiones;
    }

    f1WasDown = f1IsDown;

    // Mostrar u ocultar las coordenadas del jugador (F2)

    static bool f2WasDown = false;

    bool f2IsDown =
        (keyboardData[DIK_F2] & 0x80) != 0;

    if (f2IsDown && !f2WasDown)
    {
        dxrr->mostrarCoordenadas = !dxrr->mostrarCoordenadas;
    }

    f2WasDown = f2IsDown;

    // Escape

    if (keyboardData[DIK_ESCAPE] & 0x80)
    {
        KillTimer(hWnd, 100);
        PostQuitMessage(0);
        return;
    }

    // Rotacion de la camara con mouse

    if (SUCCEEDED(mouseResult))
    {
        const float mouseSensitivity = 0.0025f;

        dxrr->camYaw +=
            static_cast<float>(mouseData.lX) *
            mouseSensitivity;

        dxrr->camPitch +=
            static_cast<float>(mouseData.lY) *
            mouseSensitivity;
    }

    // Gamepad input

    if (gamePad)
    {
        XINPUT_STATE gamepadState;

        if (gamePad->GetState(gamepadState))
        {
            const XINPUT_GAMEPAD& pad =
                gamepadState.Gamepad;

            auto NormalizeStick =
                [](SHORT value) -> float
                {
                    if (value >= 0)
                    {
                        return static_cast<float>(value) /
                            32767.0f;
                    }

                    return static_cast<float>(value) /
                        32768.0f;
                };

            float leftX =
                NormalizeStick(pad.sThumbLX);

            float leftY =
                NormalizeStick(pad.sThumbLY);

            float rightX =
                NormalizeStick(pad.sThumbRX);

            float rightY =
                NormalizeStick(pad.sThumbRY);

            const float deadZone = 0.19f;

            if (fabsf(leftX) < deadZone)
                leftX = 0.0f;

            if (fabsf(leftY) < deadZone)
                leftY = 0.0f;

            if (fabsf(rightX) < deadZone)
                rightX = 0.0f;

            if (fabsf(rightY) < deadZone)
                rightY = 0.0f;

            const float gamepadMoveSpeed = 30.0f;
            const float gamepadLookSpeed = 2.1f;

            // Left stick controls movement.
            dxrr->moveLeftRight += leftX * gamepadMoveSpeed * deltaTime;

            dxrr->moveBackForward += leftY * gamepadMoveSpeed * deltaTime;

            // Right stick controls camera rotation.
            dxrr->camYaw += rightX * gamepadLookSpeed * deltaTime;

            dxrr->camPitch -= rightY * gamepadLookSpeed * deltaTime;

            // Right shoulder: faster movement.
            if (pad.wButtons &
                XINPUT_GAMEPAD_RIGHT_SHOULDER)
            {
                dxrr->moveBackForward *= 3.0f;
                dxrr->moveLeftRight *= 3.0f;
            }

            // Left shoulder: slower movement.
            if (pad.wButtons &
                XINPUT_GAMEPAD_LEFT_SHOULDER)
            {
                dxrr->moveBackForward *= 0.33f;
                dxrr->moveLeftRight *= 0.33f;
            }

            // toggle camera using the gamepad B button.
            static bool previousB = false;

            bool bIsDown =
                (pad.wButtons & XINPUT_GAMEPAD_B) != 0;

            if (bIsDown && !previousB && !dxrr->CamaraBloqueada())
            {
                // Same as the C key: playerPosition stays put.
                dxrr->thirdPerson =
                    !dxrr->thirdPerson;
            }

            previousB = bIsDown;
        }
    }

    //dxrr->UpdateCameraFromInput();
    SetCursorPos(
        actualPoint.x,
        actualPoint.y
    );

    ShowCursor(FALSE);
}


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;
    DEVMODE dmScreenSettings;
    int posX, posY;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"DXRR_E1";
    wc.cbSize = sizeof(WNDCLASSEX);


    RegisterClassEx(&wc);

    // If full screen set the screen to maximum size of the users desktop and 32bit.
    memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
    dmScreenSettings.dmSize = sizeof(dmScreenSettings);
    dmScreenSettings.dmPelsWidth = (unsigned long)SCREEN_X;
    dmScreenSettings.dmPelsHeight = (unsigned long)SCREEN_Y;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    // Change the display settings to full screen.
    // Si tienes problemas con correr el codigo apaga lo sig:
    //ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

    // Set the position of the window to the top left corner.
    posX = posY = 0;

    //RECT wr = {0, 0, SCREEN_X, SCREEN_Y};
    //AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    hWnd = CreateWindowEx(WS_EX_APPWINDOW,
                          L"DXRR_E1",
                          L"PLANTILLA PROYECTO",
                          WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
        posX,
        posY,
        SCREEN_X,
        SCREEN_Y,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hWnd, nCmdShow);
	dxrr = new DXRR(hWnd, SCREEN_X, SCREEN_Y);
	dxrr->moveBackForward=0;
    gamePad = new GamePadRR(1);

    initialPoint.x = 0;
    initialPoint.y = 0;
    ClientToScreen(hWnd, &initialPoint);
    actualPoint.x = initialPoint.x + SCREEN_X / 2;
    actualPoint.y = initialPoint.y + SCREEN_Y / 2;

	SetTimer(hWnd, 100, 33, NULL);
    MSG msg;
    ::DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION,
        IID_IDirectInput8, (void**)&m_pDirectInput, 0);
    createMouseDevice(hWnd);
    createKeyboardDevice(hWnd);

    LARGE_INTEGER timerFrequency = {};
    LARGE_INTEGER previousCounter = {};
    LARGE_INTEGER currentCounter = {};

    QueryPerformanceFrequency(&timerFrequency);
    QueryPerformanceCounter(&previousCounter);

    while(TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                break;
        }

        QueryPerformanceCounter(&currentCounter);

        float deltaTime = static_cast<float>(
            static_cast<double>(currentCounter.QuadPart - previousCounter.QuadPart) /
            static_cast<double>(timerFrequency.QuadPart));

        previousCounter = currentCounter;

        // Avoid a large movement jump after a pause or debugger break.
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        ProcessInput(hWnd, deltaTime);
        dxrr->Update(deltaTime);
        dxrr->Render();
    }

}


LRESULT CALLBACK WindowProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {
        KillTimer(hWnd, 100);
        PostQuitMessage(0);
        return 0;
    }

    case WM_TIMER:
    {
        return 0;
    }
    }

    return DefWindowProc(
        hWnd,
        message,
        wParam,
        lParam
    );
}
