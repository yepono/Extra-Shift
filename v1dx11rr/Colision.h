#ifndef _colision
#define _colision

// Sistema de colision simple basado en cajas alineadas a los ejes (AABB).
//
// Uso basico:
//   SistemaColision colisiones;
//   colisiones.AgregarCaja(centro, mitad, true, false, "muro");
//   colisiones.AgregarPiso(x, z, alturaPiso, ancho, fondo);
//
// Convenciones del proyecto:
//   - Mano izquierda, +Y arriba, el suelo es el plano XZ.
//   - Una caja se define por su CENTRO y sus SEMIEJES (la mitad de su tamano).
//   - `solido`  = bloquea el movimiento horizontal (muros, muebles, personajes).
//   - `soporta` = se puede caminar encima (pisos, plataformas).
//
// Una caja puede ser solida y soportable a la vez (por ejemplo una caja/crate).

#include <d3dx10math.h>
#include <cmath>
#include <string>
#include <vector>

struct CajaColision
{
	D3DXVECTOR3 centro = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 mitad  = D3DXVECTOR3(0.5f, 0.5f, 0.5f);

	bool solido  = true;
	bool soporta = false;
	bool activa  = true;

	std::string nombre;

	float MinX() const { return centro.x - mitad.x; }
	float MaxX() const { return centro.x + mitad.x; }
	float MinY() const { return centro.y - mitad.y; }
	float MaxY() const { return centro.y + mitad.y; }
	float MinZ() const { return centro.z - mitad.z; }
	float MaxZ() const { return centro.z + mitad.z; }
};

class SistemaColision
{
public:
	std::vector<CajaColision> cajas;

	// Altura maxima que el jugador puede subir de golpe (escalon).
	float pasoMaximo = 1.0f;

	void Limpiar()
	{
		cajas.clear();
	}

	// Agrega una caja con centro y semiejes.
	size_t AgregarCaja(
		const D3DXVECTOR3& centro,
		const D3DXVECTOR3& mitad,
		bool solido = true,
		bool soporta = false,
		const char* nombre = nullptr)
	{
		CajaColision caja;
		caja.centro = centro;
		caja.mitad = mitad;
		caja.solido = solido;
		caja.soporta = soporta;

		if (nombre != nullptr)
			caja.nombre = nombre;

		cajas.push_back(caja);
		return cajas.size() - 1;
	}

	// Agrega una caja indicando sus esquinas minima y maxima.
	size_t AgregarCajaMinMax(
		const D3DXVECTOR3& minimo,
		const D3DXVECTOR3& maximo,
		bool solido = true,
		bool soporta = false,
		const char* nombre = nullptr)
	{
		D3DXVECTOR3 centro = (minimo + maximo) * 0.5f;
		D3DXVECTOR3 mitad = (maximo - minimo) * 0.5f;

		// Evita semiejes negativos si las esquinas vienen invertidas.
		if (mitad.x < 0.0f) mitad.x = -mitad.x;
		if (mitad.y < 0.0f) mitad.y = -mitad.y;
		if (mitad.z < 0.0f) mitad.z = -mitad.z;

		return AgregarCaja(centro, mitad, solido, soporta, nombre);
	}

	// Piso horizontal: su cara superior queda exactamente en `alturaPiso`.
	// Se crea como NO solido para que el jugador pueda entrar caminando;
	// solo sirve de apoyo vertical.
	size_t AgregarPiso(
		float centroX,
		float centroZ,
		float alturaPiso,
		float ancho,
		float fondo,
		float grosor = 0.5f,
		const char* nombre = "piso")
	{
		if (grosor <= 0.0f)
			grosor = 0.1f;

		if (ancho < 0.0f) ancho = -ancho;
		if (fondo < 0.0f) fondo = -fondo;

		D3DXVECTOR3 centro(
			centroX,
			alturaPiso - grosor * 0.5f,
			centroZ);

		D3DXVECTOR3 mitad(
			ancho * 0.5f,
			grosor * 0.5f,
			fondo * 0.5f);

		return AgregarCaja(centro, mitad, false, true, nombre);
	}

	// Caja para un personaje (huella cuadrada). Devuelve el indice para
	// poder moverla cada frame con ActualizarCaja().
	size_t AgregarCajaPersonaje(
		float x,
		float z,
		float ySuelo,
		float radio,
		float altura,
		bool solido = true,
		const char* nombre = "personaje")
	{
		D3DXVECTOR3 centro(
			x,
			ySuelo + altura * 0.5f,
			z);

		D3DXVECTOR3 mitad(
			radio,
			altura * 0.5f,
			radio);

		return AgregarCaja(centro, mitad, solido, false, nombre);
	}

	void ActualizarCaja(size_t indice, const D3DXVECTOR3& centro)
	{
		if (indice < cajas.size())
			cajas[indice].centro = centro;
	}

	void ActivarCaja(size_t indice, bool activa)
	{
		if (indice < cajas.size())
			cajas[indice].activa = activa;
	}

	// Devuelve true si al colocar al jugador en `pos` (radio en XZ y
	// altura en Y) su volumen se cruza con alguna caja solida.
	bool Solapa(
		const D3DXVECTOR3& pos,
		float radio,
		float altura) const
	{
		const float pie = pos.y + 0.02f;
		const float cabeza = pos.y + altura;

		for (size_t i = 0; i < cajas.size(); ++i)
		{
			const CajaColision& caja = cajas[i];

			if (!caja.activa || !caja.solido)
				continue;

			// Si la caja es lo bastante baja, se puede subir en vez de
			// chocar con ella (escalones, banquetas).
			if (caja.MaxY() <= pos.y + pasoMaximo)
				continue;

			// Sin traslape vertical no hay colision.
			if (caja.MaxY() <= pie || caja.MinY() >= cabeza)
				continue;

			// Traslape horizontal: caja expandida por el radio del jugador.
			if (pos.x + radio > caja.MinX() && pos.x - radio < caja.MaxX() &&
				pos.z + radio > caja.MinZ() && pos.z - radio < caja.MaxZ())
			{
				return true;
			}
		}

		return false;
	}

	// Mueve al jugador resolviendo colision por ejes separados. Esto
	// permite "deslizarse" a lo largo de un muro en lugar de quedar
	// completamente bloqueado. El desplazamiento se divide en pasos
	// pequenos para que el jugador no atraviese cajas delgadas cuando
	// se mueve rapido (tunneling).
	D3DXVECTOR3 Mover(
		const D3DXVECTOR3& posicion,
		const D3DXVECTOR3& desplazamiento,
		float radio,
		float altura) const
	{
		D3DXVECTOR3 resultado = posicion;

		// Si el jugador ya empezo dentro de una caja (por ejemplo porque
		// se movio el modelo o cambio el escenario), se permite moverse
		// para que pueda salir en vez de quedar atrapado.
		if (Solapa(posicion, radio, altura))
		{
			resultado += desplazamiento;
			return resultado;
		}

		const float longitud =
			sqrtf(desplazamiento.x * desplazamiento.x +
			      desplazamiento.z * desplazamiento.z);

		if (longitud <= 0.0001f)
			return resultado;

		const float pasoMaximo = 0.25f;
		int pasos = (int)ceilf(longitud / pasoMaximo);

		if (pasos < 1) pasos = 1;
		if (pasos > 64) pasos = 64;

		const D3DXVECTOR3 paso =
			desplazamiento / (float)pasos;

		for (int i = 0; i < pasos; ++i)
		{
			D3DXVECTOR3 intento = resultado;
			intento.x += paso.x;
			if (!Solapa(intento, radio, altura))
				resultado.x = intento.x;

			intento = resultado;
			intento.z += paso.z;
			if (!Solapa(intento, radio, altura))
				resultado.z = intento.z;
		}

		return resultado;
	}

	// Altura de apoyo en (x, z). Toma el terreno como base y, si hay un
	// piso registrado debajo de los pies (o como maximo `pasoMaximo` por
	// encima), devuelve la cara superior de ese piso.
	float AlturaSuelo(
		float x,
		float z,
		float yActual,
		float alturaTerreno) const
	{
		float mejor = alturaTerreno;
		const float margen = 0.05f;

		for (size_t i = 0; i < cajas.size(); ++i)
		{
			const CajaColision& caja = cajas[i];

			if (!caja.activa || !caja.soporta)
				continue;

			if (x < caja.MinX() - margen || x > caja.MaxX() + margen ||
				z < caja.MinZ() - margen || z > caja.MaxZ() + margen)
			{
				continue;
			}

			const float techo = caja.MaxY();

			// No se sube a un piso que este muy por encima de los pies.
			if (techo > yActual + pasoMaximo)
				continue;

			if (techo > mejor)
				mejor = techo;
		}

		return mejor;
	}
};

#endif
