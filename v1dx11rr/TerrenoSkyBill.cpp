#include <windows.h>
#include <windowsx.h>

#include "DXRR.h"
#include "GamePadRR.h"
#include <dinput.h>
#include <xinput.h>
#include <cmath>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

#define SCREEN_X 1920
#define SCREEN_Y 1080

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DXRR *dxrr;
GamePadRR *gamePad;
tagPOINT initialPoint;
tagPOINT actualPoint;
LPDIRECTINPUT8 m_pDirectInput = NULL;
LPDIRECTINPUTDEVICE8 m_pKeyboardDevice = NULL;
LPDIRECTINPUTDEVICE8 m_pMouseDevice = NULL;
bool cKeyWasDown = false;

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

void ProcessInput(HWND hWnd)
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

    const float movementSpeed = 0.15f;
    const float keyboardLookSpeed = 0.025f;

    // movimiento WASD

    if (keyboardData[DIK_W] & 0x80)
        dxrr->moveBackForward += movementSpeed;

    if (keyboardData[DIK_S] & 0x80)
        dxrr->moveBackForward -= movementSpeed;

    if (keyboardData[DIK_A] & 0x80)
        dxrr->moveLeftRight -= movementSpeed;

    if (keyboardData[DIK_D] & 0x80)
        dxrr->moveLeftRight += movementSpeed;

    // Rotacion de la camara

    if (keyboardData[DIK_LEFT] & 0x80)
        dxrr->camYaw -= keyboardLookSpeed;

    if (keyboardData[DIK_RIGHT] & 0x80)
        dxrr->camYaw += keyboardLookSpeed;

    if (keyboardData[DIK_UP] & 0x80)
        dxrr->camPitch -= keyboardLookSpeed;

    if (keyboardData[DIK_DOWN] & 0x80)
        dxrr->camPitch += keyboardLookSpeed;

    // Toggle tercera persona con c

    bool cKeyIsDown =
        (keyboardData[DIK_C] & 0x80) != 0;

    if (cKeyIsDown && !cKeyWasDown)
    {
        dxrr->thirdPerson = !dxrr->thirdPerson;

        // El jugador se posiciona en la camara al cambiar a tp
        if (dxrr->thirdPerson)
        {
            dxrr->playerPosition = dxrr->camara->Position;
        }
    }

    cKeyWasDown = cKeyIsDown;

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

        dxrr->camPitch -=
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

            const float gamepadMoveSpeed = 0.15f;
            const float gamepadLookSpeed = 0.035f;

            // Left stick controls movement.
            dxrr->moveLeftRight +=
                leftX * gamepadMoveSpeed;

            dxrr->moveBackForward +=
                leftY * gamepadMoveSpeed;

            // Right stick controls camera rotation.
            dxrr->camYaw +=
                rightX * gamepadLookSpeed;

            dxrr->camPitch -=
                rightY * gamepadLookSpeed;

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

            if (bIsDown && !previousB)
            {
                dxrr->thirdPerson =
                    !dxrr->thirdPerson;

                if (dxrr->thirdPerson)
                {
                    dxrr->playerPosition =
                        dxrr->camara->Position;
                }
            }

            previousB = bIsDown;
        }
    }

    dxrr->UpdateCameraFromInput();
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
	dxrr = new DXRR(hWnd, 800, 600);
	dxrr->moveBackForward=0;
    gamePad = new GamePadRR(1);

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
    while(TRUE)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if(msg.message == WM_QUIT)
                break;
        }

        ProcessInput(hWnd);
        dxrr->Render();
    }

    /*if (m_pMouseDevice) {
        m_pMouseDevice->Unacquire();
        m_pMouseDevice->Release();
    }
    m_pMouseDevice = NULL;

    if (m_pKeyboardDevice) {
        m_pKeyboardDevice->Unacquire();
        m_pKeyboardDevice->Release();
    }
    m_pKeyboardDevice = NULL;

    if (m_pDirectInput)
        m_pDirectInput->Release();
    m_pDirectInput = NULL;

    return msg.wParam;*/
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
