#ifndef _XBOX_CONTROLLER_H_
#define _XBOX_CONTROLLER_H_

//no agrega en la compilada drivers de comunicaciones y otras cosas
#define WIN32_LEAN_AND_MEAN

// Incluimos estas librerias para avanzar
#include <windows.h>
#include <XInput.h> //libreria del Xbox solo gamepads o joysticks compatibles


#pragma comment(lib, "Xinput9_1_0.lib")

// XBOX Controller Class Definition
class GamePadRR
{

private:
    XINPUT_STATE estadoControlador;
    int numeroControlador;

public:
    GamePadRR(int jugadorNumero)
	{
		// establece el numero de jugador
		numeroControlador = jugadorNumero - 1;
	}

    bool GetState(XINPUT_STATE& state)
	{
		// limpia la estructura del estado del gamepad
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		// obtiene el estado del gamepad
		DWORD result = XInputGetState(
			numeroControlador,
			&state
		);
		return result == ERROR_SUCCESS;
	}

    XINPUT_STATE GetState()
    {
        ZeroMemory(&estadoControlador, sizeof(XINPUT_STATE));

        XInputGetState(
            numeroControlador,
            &estadoControlador
        );

        return estadoControlador;
    }

    bool IsConnected()
    {
        XINPUT_STATE state;

        return GetState(state);
    }

    void Vibrate(
        WORD leftValue = 0,
        WORD rightValue = 0)
    {
        XINPUT_VIBRATION vibration;
        ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

        vibration.wLeftMotorSpeed = leftValue;
        vibration.wRightMotorSpeed = rightValue;

        XInputSetState(
            numeroControlador,
            &vibration
        );
    }
};

#endif