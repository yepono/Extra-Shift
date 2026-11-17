#ifndef _camara
#define _camara

#include <d3d11.h>
#include <D3DX11.h>
#include <D3DX10math.h>

#include "Colision.h"

class Camara{
public:
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Target;
	D3DXVECTOR3 Up;

	D3DXVECTOR3 DefaultForward;
	D3DXVECTOR3 DefaultRight;
	D3DXVECTOR3 Forward;
	D3DXVECTOR3 Right;

	D3DXMATRIX View;
	D3DXMATRIX Projection;
	D3DXMATRIX RotationMatrix;

	// yaw: rotacion horizontal; pitch: rotacion vertical
	float Yaw;
	float Pitch;

	// Colision del jugador. Si no se configura, la camara se mueve como antes.
	SistemaColision* colision = nullptr;
	float colisionRadio = 1.0f;
	float colisionAltura = 4.0f;

	Camara(
		D3DXVECTOR3 position,
		D3DXVECTOR3 target,
		D3DXVECTOR3 up,
		int width,
		int height)
	{
		Position = position;
		Target = target;
		Up = up;

		D3DXMatrixLookAtLH(
			&View,
			&Position,
			&Target,
			&Up
		);

		D3DXMatrixPerspectiveFovLH(
			&Projection,
			D3DX_PI / 3.0f,
			static_cast<float>(width) / static_cast<float>(height),
			0.01f,
			1000.0f
		);

		DefaultForward = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
		DefaultRight = D3DXVECTOR3(1.0f, 0.0f, 0.0f);

		Forward = DefaultForward;
		Right = DefaultRight;

		Yaw = 0.0f;
		Pitch = 0.0f;
	}

	// Primera persona: la camara se coloca en los ojos del jugador.
	// se toma el playerPosition para que al alternar entre ambos modos no se acumule desplazamiento.
	void UpdateFirstPerson(
		D3DXVECTOR3& playerPosition,
		float moveBackForward,
		float moveLeftRight,
		float yaw,
		float pitch,
		float eyeHeight)
	{
		Yaw += yaw;
		Pitch += pitch;

		// para que la camara no de vueltas infinitas veticalmente
		const float maxPitch = D3DXToRadian(89.0f);

		if (Pitch > maxPitch)
			Pitch = maxPitch;

		if (Pitch < -maxPitch)
			Pitch = -maxPitch;

		// Ahora si, la rotacion competa de la camara
		D3DXMatrixRotationYawPitchRoll(
			&RotationMatrix,
			Yaw,
			Pitch,
			0.0f
		);

		// calcular la direccion hacia la que mira la cam
		D3DXVec3TransformCoord(
			&Target,
			&DefaultForward,
			&RotationMatrix
		);

		D3DXVec3Normalize(&Target, &Target);
		
		// calcular los vectores del movimiento usando la rotacion horizontal (yaw)
		D3DXMATRIX yawMatrix;
		D3DXMatrixRotationY(&yawMatrix, Yaw);

		D3DXVec3TransformCoord(
			&Forward,
			&DefaultForward,
			&yawMatrix
		);

		D3DXVec3TransformCoord(
			&Right,
			&DefaultRight,
			&yawMatrix
		);

		D3DXVec3Normalize(&Forward, &Forward);
		D3DXVec3Normalize(&Right, &Right);

		// Mover al jugador (no a la camara).
		MoverJugador(playerPosition, moveBackForward, moveLeftRight);

		// La camara vive en los ojos del jugador.
		Position = playerPosition;
		Position.y += eyeHeight;

		// el target es una direccion relativa a la camara
		D3DXVECTOR3 lookAt = Position + Target;

		D3DXMatrixLookAtLH(
			&View,
			&Position,
			&lookAt,
			&Up
		);
	}

// c es para togglear la primera persona y la tercera
	void UpdateThirdPerson(
		D3DXVECTOR3& playerPosition,
		float moveBackForward,
		float moveLeftRight,
		float yaw,
		float pitch,
		float distance,
		float height)
	{
		Yaw += yaw;
		Pitch += pitch;

		const float maxPitch = D3DXToRadian(75.0f);
		const float minPitch = D3DXToRadian(-30.0f);

		if (Pitch > maxPitch)
			Pitch = maxPitch;

		if (Pitch < minPitch)
			Pitch = minPitch;

		// Calcular la direccion en la que mira la camara
		D3DXMatrixRotationYawPitchRoll(
			&RotationMatrix,
			Yaw,
			Pitch,
			0.0f
		);

		D3DXVECTOR3 lookDirection;

		D3DXVec3TransformCoord(
			&lookDirection,
			&DefaultForward,
			&RotationMatrix
		);

		D3DXVec3Normalize(
			&lookDirection,
			&lookDirection
		);

		// Calculate movement vectors using yaw only.
		D3DXMATRIX yawMatrix;

		D3DXMatrixRotationY(
			&yawMatrix,
			Yaw
		);

		D3DXVec3TransformCoord(
			&Forward,
			&DefaultForward,
			&yawMatrix
		);

		D3DXVec3TransformCoord(
			&Right,
			&DefaultRight,
			&yawMatrix
		);

		D3DXVec3Normalize(
			&Forward,
			&Forward
		);

		D3DXVec3Normalize(
			&Right,
			&Right
		);

		// Move the player.
		MoverJugador(playerPosition, moveBackForward, moveLeftRight);

		// The point that the camera follows.
		D3DXVECTOR3 focusPoint = playerPosition;
		focusPoint.y += height;

		// La canara va detras del jugador
		Position = focusPoint - lookDirection * distance;

		// La camara enfoca al jugador.
		Target = focusPoint;

		D3DXMatrixLookAtLH(
			&View,
			&Position,
			&Target,
			&Up
		);
	}

	void ConfigurarColision(SistemaColision* sistema, float radio, float altura)
	{
		colision = sistema;
		colisionRadio = radio;
		colisionAltura = altura;
	}

	// Aplica el desplazamiento horizontal del frame al jugador usando el
	// sistema de colision (si existe). La altura se ajusta aparte.
	void MoverJugador(
		D3DXVECTOR3& playerPosition,
		float moveBackForward,
		float moveLeftRight)
	{
		D3DXVECTOR3 desplazamiento =
			Forward * moveBackForward +
			Right * moveLeftRight;

		if (colision != nullptr)
		{
			playerPosition = colision->Mover(
				playerPosition,
				desplazamiento,
				colisionRadio,
				colisionAltura);
		}
		else
		{
			playerPosition += desplazamiento;
		}
	}

	// Corrige la altura de la camara cuando el suelo cambia (terreno o
	// piso de un edificio). La vista se reconstruye con la nueva altura.
	void DesplazarVertical(float deltaY, bool terceraPersona)
	{
		if (deltaY == 0.0f)
			return;

		Position.y += deltaY;

		D3DXVECTOR3 lookAt;

		if (terceraPersona)
		{
			Target.y += deltaY;
			lookAt = Target;
		}
		else
		{
			// En primera persona, Target es una direccion, no un punto.
			lookAt = Position + Target;
		}

		D3DXMatrixLookAtLH(&View, &Position, &lookAt, &Up);
	}

	D3DXVECTOR2 GetXZ() const
	{
		return D3DXVECTOR2(Position.x, Position.z);
	}

};
#endif