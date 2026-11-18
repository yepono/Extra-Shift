#ifndef _sonidos
#define _sonidos

// Sonidos del juego con MCI (winmm): reproduce .mp3 y .wav, varios a la vez,
// y puede repetir un sonido en bucle (el ambiente del desierto).
//
// Uso:
//   Sonidos sonidos;
//   sonidos.Cargar("click", "Assets/Sonido/click.mp3");
//   sonidos.Tocar("click");                 // desde el inicio
//   sonidos.Bucle("desierto");              // se repite siempre
//   sonidos.Volumen("desierto", 350);       // 0..1000
//   sonidos.LiberarTodo();
//
// Las rutas son relativas a la carpeta de trabajo (la del proyecto).

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>

#pragma comment(lib, "winmm.lib")

class Sonidos
{
public:
	~Sonidos()
	{
		LiberarTodo();
	}

	// alias: nombre corto para despues tocarlo. Devuelve false si falla.
	bool Cargar(const char* alias, const char* ruta)
	{
		if (alias == nullptr || ruta == nullptr)
			return false;

		// "mpegvideo" abre mp3 y wav y permite "play ... repeat".
		const std::string comando =
			std::string("open \"") + ruta + "\" type mpegvideo alias " + alias;

		if (!Enviar(comando))
		{
			OutputDebugStringA("Sonidos: no se pudo abrir ");
			OutputDebugStringA(ruta);
			OutputDebugStringA("\n");
			return false;
		}

		m_alias.push_back(alias);
		return true;
	}

	// Lo toca desde el principio (si ya sonaba, lo reinicia).
	void Tocar(const char* alias)
	{
		if (!Existe(alias))
			return;

		Enviar(std::string("seek ") + alias + " to start");
		Enviar(std::string("play ") + alias);
	}

	void Bucle(const char* alias)
	{
		if (!Existe(alias))
			return;

		Enviar(std::string("seek ") + alias + " to start");
		Enviar(std::string("play ") + alias + " repeat");
	}

	void Detener(const char* alias)
	{
		if (Existe(alias))
			Enviar(std::string("stop ") + alias);
	}

	// 0 (silencio) a 1000 (normal).
	void Volumen(const char* alias, int volumen)
	{
		if (!Existe(alias))
			return;

		if (volumen < 0) volumen = 0;
		if (volumen > 1000) volumen = 1000;
		Enviar(std::string("setaudio ") + alias + " volume to " + std::to_string(volumen));
	}

	void LiberarTodo()
	{
		for (size_t i = 0; i < m_alias.size(); ++i)
			Enviar("close " + m_alias[i]);

		m_alias.clear();
	}

private:
	bool Existe(const char* alias) const
	{
		if (alias == nullptr)
			return false;

		for (size_t i = 0; i < m_alias.size(); ++i)
		{
			if (m_alias[i] == alias)
				return true;
		}

		return false;
	}

	static bool Enviar(const std::string& comando)
	{
		return mciSendStringA(comando.c_str(), nullptr, 0, nullptr) == 0;
	}

	std::vector<std::string> m_alias;
};

#endif
