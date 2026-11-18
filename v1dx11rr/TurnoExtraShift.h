#ifndef _turno_extra_shift
#define _turno_extra_shift

// Maquina de estados del turno en el Oxxo.
//
// Explorando -> (el jugador entra) puerta bloqueada
//   -> cliente en camino -> cliente esperando en la caja
//   -> dialogo de 5 lineas en primera persona (opciones barajadas)
//   -> caen 2 o 3 productos; hay que devolverlos estante por estante
//   -> el siguiente cliente no avanza mientras quede algo en el piso
//   -> al tercer cliente: noche, la puerta se abre, abordar la nave con 600 pesos
//
// La vida baja de golpe: respuesta rara, respuesta alienigena,
// o 20 segundos sin atender (cliente en la caja o piso sin limpiar).
//
// Los textos estan en UTF-8 con acentos; TextoPantalla los decodifica.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <windows.h>

class TurnoExtraShift
{
public:
	// ---- Ajustes de juego -------------------------------------------------
	static constexpr float VidaMaxima = 100.0f;
	static constexpr float CuraHumana = 5.0f;    // opcion humana
	static constexpr float DanoRaro = 15.0f;     // opcion rara / sospechosa
	static constexpr float DanoAlien = 35.0f;    // opcion alienigena
	static constexpr float DanoEspera = 20.0f;   // cada 20 s sin atender
	static constexpr float LimiteEspera = 20.0f;

	static constexpr float PagoHumano = 45.0f;   // por linea bien contestada
	static constexpr float PagoRaro = 20.0f;     // el cliente compra a medias
	static constexpr float MetaPesos = 600.0f;   // pieza averiada de la nave

	// ---- Posiciones en el mundo (ajustar con F2) -------------------------
	static constexpr float MostradorX = -53.0f;  // centro del mostrador
	static constexpr float MostradorZ = 30.0f;
	static constexpr float PuestoX = -56.5f;     // el jugador, detras del mostrador
	static constexpr float PuestoZ = 30.0f;
	static constexpr float ClienteX = -49.0f;    // el cliente, frente al mostrador
	static constexpr float ClienteZ = 30.0f;
	static constexpr float NaveX = -35.0f;
	static constexpr float NaveZ = 62.0f;

	TurnoExtraShift()
	{
		srand((unsigned)time(nullptr));
		Reiniciar();
	}

	void Reiniciar()
	{
		fase = Fase::Explorando;
		cliente = 0;
		linea = 0;
		seleccion = -1;
		respondida = false;
		pausa = 0.0f;
		timer = 0.0f;
		espera = 0.0f;
		vida = VidaMaxima;
		pesos = 0.0f;
		ticket = 0.0f;
		clienteMolesto = false;
		llevando = -1;
		generacion++;
		pedirReposicion = false;
		pedirRecogida = false;

		for (int i = 0; i < MaxObjetos; ++i)
			objetos[i].activo = false;

		for (int i = 0; i < 3; ++i)
		{
			orden[i] = i;
			opcionVisible[i][0] = '\0';
		}

		lineaVisible[0] = '\0';
		aviso[0] = '\0';
		estado[0] = '\0';
		motivo[0] = '\0';
		RefrescarEstado();
	}

	// opcion: 0 = ninguna, 1..3 = tecla pulsada este frame.
	// confirmar: ENTER / E este frame (acepta la opcion marcada).
	void Actualizar(
		float dt,
		float jugadorX,
		float jugadorZ,
		bool interactuar,
		int opcion,
		bool confirmar)
	{
		if (dt < 0.0f)
			dt = 0.0f;

		for (int i = 0; i < MaxObjetos; ++i)
		{
			if (!objetos[i].activo)
				continue;

			if (objetos[i].enMano)
			{
				objetos[i].x = jugadorX;
				objetos[i].z = jugadorZ;
			}
			else
			{
				objetos[i].caida += dt / 0.45f;
				if (objetos[i].caida > 1.0f)
					objetos[i].caida = 1.0f;
			}
		}

		switch (fase)
		{
		case Fase::Explorando:
			sprintf_s(aviso, "Entra al Oxxo para empezar el turno. Las puertas se cerrarán.");

			if (DentroDelOxxo(jugadorX, jugadorZ))
			{
				fase = Fase::ClienteEnCamino;
				cliente = 0;
				timer = 0.0f;
				sprintf_s(aviso, "¡Empieza el turno! Las puertas quedan bloqueadas.");
			}
			break;

		case Fase::ClienteEnCamino:
			timer += dt;
			sprintf_s(aviso, "%s se acerca. %s", Guion()[cliente].nombre, Guion()[cliente].contexto);

			if (timer >= 2.2f)
			{
				fase = Fase::ClienteEsperando;
				espera = 0.0f;
			}
			break;

		case Fase::ClienteEsperando:
			if (!ContarEspera(dt))
				break;

			if (interactuar && Distancia(jugadorX, jugadorZ, MostradorX, MostradorZ) <= RadioCaja)
			{
				EntrarDialogo();
			}
			else
			{
				sprintf_s(
					aviso,
					"%s está en la caja. Ve al mostrador y pulsa E. Quedan %d s.",
					Guion()[cliente].nombre,
					SegundosRestantes());
			}
			break;

		case Fase::Dialogo:
			if (respondida)
			{
				pausa -= dt;
				if (pausa <= 0.0f)
					AvanzarLinea();
			}
			else
			{
				if (opcion >= 1 && opcion <= 3)
					seleccion = opcion - 1;

				if (confirmar && seleccion >= 0)
					Responder(seleccion);
			}
			break;

		case Fase::Limpieza:
		{
			if (!ContarEspera(dt))
				break;

			bool evento = false;
			if (interactuar)
				evento = IntentarLimpieza(jugadorX, jugadorZ);

			if (ObjetosActivos() == 0)
			{
				cliente++;
				fase = Fase::ClienteEnCamino;
				timer = 0.0f;
				espera = 0.0f;
				sprintf_s(aviso, "El piso está limpio. Llega el siguiente cliente.");
				break;
			}

			if (!evento)
			{
				if (llevando >= 0)
				{
					const Estante& estante = Estantes()[objetos[llevando].estante];
					sprintf_s(
						aviso,
						"Llevas %s. Devuélvelo a su estante (X %.0f, Z %.0f) y pulsa E. Quedan %d s.",
						estante.nombre,
						estante.x,
						estante.z,
						SegundosRestantes());
				}
				else
				{
					sprintf_s(
						aviso,
						"Hay %d productos en el piso. Recógelos con E; el cliente espera. Quedan %d s.",
						ObjetosActivos(),
						SegundosRestantes());
				}
			}
			break;
		}

		case Fase::Noche:
			if (Distancia(jugadorX, jugadorZ, NaveX, NaveZ) <= RadioNave)
			{
				if (interactuar)
				{
					if (pesos >= MetaPesos)
					{
						fase = Fase::Victoria;
						sprintf_s(aviso, "Abordaste la nave y compraste la pieza.");
					}
					else
					{
						Perder("No juntaste los 600 pesos para la pieza averiada. Te quedaste varado en la Tierra.");
					}
				}
				else
				{
					sprintf_s(aviso, "Estás junto a tu nave. Pulsa E para abordarla.");
				}
			}
			else
			{
				sprintf_s(aviso, "Ya es de noche. Sal del Oxxo y aborda tu nave, frente a la puerta.");
			}
			break;

		case Fase::Victoria:
		case Fase::Derrota:
			break;
		}

		RefrescarEstado();
	}

	// ---- Consultas para DXRR ---------------------------------------------
	bool PuertaBloqueada() const
	{
		return fase != Fase::Explorando &&
			fase != Fase::Noche &&
			fase != Fase::Victoria &&
			fase != Fase::Derrota;
	}

	bool ForzarPrimeraPersona() const { return fase == Fase::Dialogo; }
	bool BloquearMovimiento() const { return fase == Fase::Dialogo || JuegoTerminado(); }
	bool EsNoche() const { return fase == Fase::Noche || fase == Fase::Victoria; }
	bool EnDialogo() const { return fase == Fase::Dialogo; }
	bool JuegoTerminado() const { return fase == Fase::Victoria || fase == Fase::Derrota; }
	bool Gano() const { return fase == Fase::Victoria; }

	bool ClienteVisibleEnCaja() const
	{
		return fase == Fase::ClienteEnCamino ||
			fase == Fase::ClienteEsperando ||
			fase == Fase::Dialogo;
	}

	int IndiceCliente() const
	{
		if (cliente < 0) return 0;
		if (cliente > 2) return 2;
		return cliente;
	}

	float Vida() const { return vida; }
	float Pesos() const { return pesos; }
	float Ticket() const { return ticket; }

	// Mira desde el puesto hacia el cliente (yaw de la camara).
	static float YawHaciaCliente()
	{
		return atan2f(ClienteX - PuestoX, ClienteZ - PuestoZ);
	}

	// Mira desde el cliente hacia el puesto (yaw del modelo del cliente).
	static float YawHaciaPuesto()
	{
		return atan2f(PuestoX - ClienteX, PuestoZ - ClienteZ);
	}

	bool TomarReposicion(float& x, float& z)
	{
		if (!pedirReposicion)
			return false;

		pedirReposicion = false;
		x = PuestoX;
		z = PuestoZ;
		return true;
	}

	bool ConsumirRecogida()
	{
		if (!pedirRecogida)
			return false;

		pedirRecogida = false;
		return true;
	}

	// Cambia cada vez que aparecen o desaparecen productos del piso.
	int GeneracionObjetos() const { return generacion; }
	static int MaximoObjetos() { return MaxObjetos; }

	// Devuelve false si ese hueco no tiene un producto activo.
	bool ObjetoActivo(
		int indice,
		float& x,
		float& z,
		float& caida,
		bool& enMano,
		int& estante) const
	{
		if (indice < 0 || indice >= MaxObjetos || !objetos[indice].activo)
			return false;

		x = objetos[indice].x;
		z = objetos[indice].z;
		caida = objetos[indice].caida;
		enMano = objetos[indice].enMano;
		estante = objetos[indice].estante;
		return true;
	}

	const WCHAR* TexturaEstante(int estante) const
	{
		if (estante < 0 || estante >= TotalEstantes)
			estante = 0;

		return Estantes()[estante].textura;
	}

	const char* Aviso() const { return aviso; }
	const char* Estado() const { return estado; }
	const char* Motivo() const { return motivo; }
	const char* LineaCliente() const { return lineaVisible; }
	const char* NombreCliente() const { return Guion()[IndiceCliente()].nombre; }
	int NumeroLinea() const { return linea; }

	const char* Opcion(int slot) const
	{
		if (slot < 0 || slot > 2)
			return "";

		return opcionVisible[slot];
	}

	// Slot (0..2) donde esta el cursor; -1 si aun no se elige.
	int Seleccion() const { return seleccion; }

	// true mientras se muestra la reaccion a la respuesta elegida.
	bool Respondida() const { return respondida; }

	int SegundosRestantes() const
	{
		int s = (int)ceilf(LimiteEspera - espera);
		return s < 0 ? 0 : s;
	}

	bool CuentaRegresivaActiva() const
	{
		return fase == Fase::ClienteEsperando || fase == Fase::Limpieza;
	}

private:
	enum class Fase
	{
		Explorando,
		ClienteEnCamino,
		ClienteEsperando,
		Dialogo,
		Limpieza,
		Noche,
		Victoria,
		Derrota
	};

	// Orden original del guion: 0 = humana, 1 = rara, 2 = alienigena.
	enum Tipo { Humana = 0, Rara = 1, Alien = 2 };

	struct LineaGuion
	{
		const char* cliente;
		const char* opciones[3];
	};

	struct ClienteGuion
	{
		const char* nombre;
		const char* contexto;
		LineaGuion lineas[5];
	};

	struct Estante
	{
		float x;
		float z;
		const WCHAR* textura;
		const char* nombre;
	};

	struct ObjetoSuelo
	{
		bool activo = false;
		bool enMano = false;
		int estante = 0;
		float x = 0.0f;
		float z = 0.0f;
		float caida = 0.0f;
	};

	// Interior del Oxxo (modelo en -35, 25). Solo la parte de adentro,
	// lejos del marco de la puerta, para no encerrar al jugador en ella.
	static bool DentroDelOxxo(float x, float z)
	{
		return x > -58.0f && x < -12.0f && z > 8.0f && z < 40.0f;
	}

	static float Distancia(float x0, float z0, float x1, float z1)
	{
		const float dx = x0 - x1;
		const float dz = z0 - z1;
		return sqrtf(dx * dx + dz * dz);
	}

	static const ClienteGuion* Guion()
	{
		static const ClienteGuion guion[3] =
		{
			{
				"El Vecino Apurado",
				"Llega de prisa golpeando el mostrador. Trae una coca y unas papas.",
				{
					{
						"¡Qué transa, bro! Me urge pagar esto que voy tarde al jale. ¿Me cobras rápido?",
						{
							"Claro que sí, jefe. Son 45 pesos en total, ¿va a ser en efectivo o tarjeta?",
							"Afirmativo, terrícola. Intercambio este cilindro de azúcar por tus créditos terrestres.",
							"Mis antenas captan que tu pulso cardíaco es acelerado. ¿Veneras al gran dios Zorgon?"
						}
					},
					{
						"¿Tienes cambio de un billete de 500? Es que no traigo más chico.",
						{
							"Sí te alcanzo a dar cambio, déjame revisar la caja registradora.",
							"El papel moneda carece de valor real en mi galaxia, pero acepto tu tributo.",
							"Muerde el billete para verificar que no sea un holograma sintético de la resistencia."
						}
					},
					{
						"Oye, ¿está sirviendo el cajero automático de afuera? Es que no traigo efectivo.",
						{
							"La neta no sé, carnal, hazte el favor de checarlo tú mismo saliendo a la izquierda.",
							"Ese monolito electrónico suele fallar cuando las alineaciones estelares cambian.",
							"Los cajeros absorben elmas de los humanos desprevenidos. No caigas en su trampa."
						}
					},
					{
						"¿Me das una bolsa por favor? Se me va a caer todo.",
						{
							"Claro, ten. ¿Algo más que se te ofrezca?",
							"Contenedor de plástico asignado para tu materia orgánica.",
							"En mi planeta llevamos las cosas flotando con telequinesis, qué atrasados están."
						}
					},
					{
						"Bueno, ten el dinero exacto. Gracias, joven. ¡Buen día!",
						{
							"Igualmente, que te vaya muy bien en tu chamba. ¡Hasta luego!",
							"Que tu temperatura corporal se mantenga estable, humano.",
							"Larga vida y prosperidad... o lo que sea que dure tu efímera especie."
						}
					}
				}
			},
			{
				"La Señora de los Gatos",
				"Entra con calma arrastrando los pies y oliendo a croquetas. Trae comida enlatada.",
				{
					{
						"Buenas tardes, joven. Oiga, ¿todavía hay de estas latas de atún en oferta? No las veo en el estante.",
						{
							"Buenas tardes, señora. Déjame checar en la bodega si nos quedó algo surtido.",
							"Las provisiones felinas escasean debido a la tormenta solar de ayer.",
							"¿Por qué alimentas a esas criaturas mutantes de cuatro patas? Son espías de otro sistema solar."
						}
					},
					{
						"Ay, hijo, te ves un poco pálido y con los ojos bien abiertos. ¿Te sientes bien?",
						{
							"Es que no he dormido bien, señora, puras desveladas por la chamba.",
							"Mi exoesqueleto exterior se está adaptando a la atmósfera terrestre.",
							"¡Mis filtros oculares se están sobrecalentando! No me mires fijamente."
						}
					},
					{
						"¿Me puedes marcar esto con cuidado? Es que a mi michi le dan miedo los golpes fuertes.",
						{
							"Sí, no se preocupe, se los trato con mucho cuidado para que no se abollen.",
							"Trataré las latas con la precisión de un rayo láser desintegrador.",
							"Los gatos de esta dimensión me observan con desprecio telepático."
						}
					},
					{
						"Oiga, ¿sigue aplicando lo de donar los centavos para la fundación?",
						{
							"Sí, ¿gusta redondear sus centavos el día de hoy?",
							"Donarás tus recursos a la causa de reconstrucción intergaláctica.",
							"El dinero es una ilusión capitalista diseñada para esclavizar a los simios."
						}
					},
					{
						"Tenga su dinero y su ticket. Gracias por la atención, mijito.",
						{
							"De nada, que tenga una excelente tarde y saludos a su michi.",
							"Mis sensores registran transacción exitosa. Adiós, terrícola anciana.",
							"Ejecutando protocolo de despedida humana... beep boop."
						}
					}
				}
			},
			{
				"El Sujeto Misterioso",
				"Usa lentes de sol de noche y actúa de manera muy sospechosa. Compra café y un encendedor.",
				{
					{
						"Buenas. Vengo por un café bien cargado. Oye, ¿no has escuchado ruidos raros en el cielo últimamente?",
						{
							"Buenas. La neta no, aquí en el desierto siempre está bien tranquilo el rollo.",
							"Son solo pruebas de satélites militares gubernamentales, nada de qué preocuparse.",
							"¡Eran mis compas pasando a saludar en su nave nodriza! Digo... ¿cuáles naves?"
						}
					},
					{
						"Hmm, qué respuesta tan defensiva. ¿Estás seguro de que eres de por aquí? Nunca te había visto en este turno.",
						{
							"Acabo de entrar esta semana, carnal, por eso no me ubicas todavía.",
							"Mi ADN cumple con todos los estándares requeridos por la ley laboral mexicana.",
							"¡Me descubriste! Vengo del cuadrante Zeta Reticuli a estudiar sus snacks."
						}
					},
					{
						"Jaja, te estoy cotorreando, tranquilo. ¿Cuánto es del café y el encendedor?",
						{
							"Ah, jotería contigo, me susto. Son 32 pesos cerrados.",
							"Transferencia de 32 unidades monetarias procesada correctamente.",
							"Tus bromas de simio generan confusión en mis circuitos cerebrales."
						}
					},
					{
						"Oye, se te cayeron unas monedas hace rato y caminas bien raro, como si te dolieran las rodillas.",
						{
							"Es que ando bien cansado por la mala postura de estar parado todo el turno.",
							"Las articulaciones bipedestas de los humanos son muy defectuosas.",
							"Mis servomecanismos internos necesitan lubricación con aceite de motor espacial."
						}
					},
					{
						"Bueno, ten tu lana. Suerte con tu turno, compa... y cuídate de lo que cae del cielo.",
						{
							"Gracias, igualmente, cuídate mucho en la calle. Nos vemos.",
							"Mis sistemas de navegación están listos para el despegue nocturno.",
							"Activando secuencia de escape estelar en 3... 2... 1..."
						}
					}
				}
			}
		};

		return guion;
	}

	static const Estante* Estantes()
	{
		static const Estante estantes[TotalEstantes] =
		{
			{ -20.0f, 35.0f, L"Assets/Billboards/cokita.png", "la Cokita" },
			{ -24.0f, 35.0f, L"Assets/Billboards/doritos.png", "los Doritos" },
			{ -28.0f, 35.0f, L"Assets/Billboards/boing.png", "el Boing" },
			{ -32.0f, 30.0f, L"Assets/Billboards/oreo.png", "las Oreo" },
			{ -36.0f, 30.0f, L"Assets/Billboards/nutri.png", "el Nutri" },
			{ -38.0f, 25.0f, L"Assets/Billboards/hersheys.png", "el Hershey's" },
		};

		return estantes;
	}

	// Corre el reloj de 20 s. Devuelve false si el jugador murio.
	bool ContarEspera(float dt)
	{
		espera += dt;

		while (espera >= LimiteEspera)
		{
			espera -= LimiteEspera;
			Danar(DanoEspera);

			if (vida <= 0.0f)
			{
				Perder("Hiciste esperar demasiado a los clientes. Te corrieron del Oxxo.");
				return false;
			}
		}

		return true;
	}

	void Danar(float cantidad)
	{
		vida -= cantidad;
		if (vida < 0.0f)
			vida = 0.0f;
	}

	void Perder(const char* razon)
	{
		vida = vida < 0.0f ? 0.0f : vida;
		fase = Fase::Derrota;
		if (llevando >= 0)
			objetos[llevando].enMano = false;
		llevando = -1;
		sprintf_s(motivo, "%s", razon);
		sprintf_s(aviso, "%s", razon);
	}

	void EntrarDialogo()
	{
		fase = Fase::Dialogo;
		linea = 0;
		ticket = 0.0f;
		clienteMolesto = false;
		pedirReposicion = true;
		PrepararLinea();
	}

	void Responder(int slot)
	{
		const int tipo = orden[slot];
		seleccion = slot;
		respondida = true;
		pausa = 1.1f;

		if (tipo == Humana)
		{
			vida += CuraHumana;
			if (vida > VidaMaxima)
				vida = VidaMaxima;

			if (!clienteMolesto)
			{
				ticket += PagoHumano;
				sprintf_s(aviso, "Respuesta humana. El cliente sigue comprando (+%.0f pesos).", PagoHumano);
			}
			else
			{
				sprintf_s(aviso, "Respuesta humana, pero el cliente ya no quiere comprar nada más.");
			}
		}
		else if (tipo == Rara)
		{
			Danar(DanoRaro);

			if (!clienteMolesto)
			{
				ticket += PagoRaro;
				sprintf_s(aviso, "Respuesta rara. El cliente sospecha (-%.0f vida, solo +%.0f pesos).", DanoRaro, PagoRaro);
			}
			else
			{
				sprintf_s(aviso, "Respuesta rara. El cliente sospecha (-%.0f vida).", DanoRaro);
			}
		}
		else
		{
			Danar(DanoAlien);
			clienteMolesto = true;
			sprintf_s(aviso, "¡Respuesta alienígena! (-%.0f vida). El cliente ya no quiere comprar nada más.", DanoAlien);
		}

		if (vida <= 0.0f)
			Perder("Tus respuestas fueron demasiado alienígenas. Descubrieron que no eres de aquí.");
	}

	void AvanzarLinea()
	{
		linea++;

		if (linea >= 5)
		{
			TerminarCliente();
			return;
		}

		PrepararLinea();
	}

	void TerminarCliente()
	{
		pesos += ticket;
		const float cobrado = ticket;
		ticket = 0.0f;
		respondida = false;
		seleccion = -1;

		if (cliente >= 2)
		{
			fase = Fase::Noche;

			if (pesos >= MetaPesos)
				sprintf_s(aviso, "Terminó el turno y anocheció. Juntaste %.0f pesos: ¡alcanza para la pieza!", pesos);
			else
				sprintf_s(aviso, "Terminó el turno y anocheció. Solo juntaste %.0f de %.0f pesos.", pesos, MetaPesos);
			return;
		}

		fase = Fase::Limpieza;
		espera = 0.0f;
		SoltarObjetos();
		sprintf_s(aviso, "El cliente pagó %.0f pesos y se fue. ¡Se cayeron %d productos!", cobrado, ObjetosActivos());
	}

	// Baraja las 3 opciones y arma los textos de la linea actual.
	void PrepararLinea()
	{
		seleccion = -1;
		respondida = false;
		pausa = 0.0f;

		for (int i = 0; i < 3; ++i)
			orden[i] = i;

		for (int i = 2; i > 0; --i)
		{
			const int j = rand() % (i + 1);
			const int t = orden[i];
			orden[i] = orden[j];
			orden[j] = t;
		}

		const ClienteGuion& guion = Guion()[IndiceCliente()];
		const LineaGuion& actual = guion.lineas[linea];

		sprintf_s(lineaVisible, "%s: %s", guion.nombre, actual.cliente);

		for (int slot = 0; slot < 3; ++slot)
			sprintf_s(opcionVisible[slot], "%d. %s", slot + 1, actual.opciones[orden[slot]]);

		sprintf_s(aviso, "Elige con 1, 2 o 3 y confirma con ENTER o E.  (Pregunta %d de 5)", linea + 1);
	}

	void SoltarObjetos()
	{
		llevando = -1;

		for (int i = 0; i < MaxObjetos; ++i)
			objetos[i].activo = false;

		const int cantidad = 2 + (rand() % 2);
		bool usado[TotalEstantes] = {};
		int creados = 0;
		int guardia = 0;

		while (creados < cantidad && guardia < 60)
		{
			const int estante = rand() % TotalEstantes;
			guardia++;

			if (usado[estante])
				continue;

			usado[estante] = true;

			ObjetoSuelo& objeto = objetos[creados];
			objeto.activo = true;
			objeto.enMano = false;
			objeto.estante = estante;
			objeto.caida = 0.0f;
			objeto.x = Estantes()[estante].x;
			objeto.z = Estantes()[estante].z - 2.5f;
			creados++;
		}

		generacion++;
	}

	bool IntentarLimpieza(float jugadorX, float jugadorZ)
	{
		if (llevando >= 0)
		{
			const int destino = objetos[llevando].estante;
			const Estante& estante = Estantes()[destino];

			if (Distancia(jugadorX, jugadorZ, estante.x, estante.z) <= RadioEstante)
			{
				objetos[llevando].activo = false;
				objetos[llevando].enMano = false;
				llevando = -1;
				generacion++;
				espera = 0.0f; // devolver un producto cuenta como atender
				sprintf_s(aviso, "Producto devuelto a su estante.");
				return true;
			}

			for (int i = 0; i < TotalEstantes; ++i)
			{
				if (i == destino)
					continue;

				if (Distancia(jugadorX, jugadorZ, Estantes()[i].x, Estantes()[i].z) <= RadioEstante)
				{
					sprintf_s(aviso, "Ese no es su estante. Busca las coordenadas del aviso.");
					return true;
				}
			}

			return false;
		}

		int mejor = -1;
		float mejorDistancia = RadioRecoger;

		for (int i = 0; i < MaxObjetos; ++i)
		{
			if (!objetos[i].activo || objetos[i].enMano)
				continue;

			const float distancia = Distancia(jugadorX, jugadorZ, objetos[i].x, objetos[i].z);

			if (distancia <= mejorDistancia)
			{
				mejorDistancia = distancia;
				mejor = i;
			}
		}

		if (mejor < 0)
			return false;

		objetos[mejor].enMano = true;
		llevando = mejor;
		pedirRecogida = true;
		generacion++;
		sprintf_s(aviso, "Recogiste %s. Llévalo a su estante.", Estantes()[objetos[mejor].estante].nombre);
		return true;
	}

	int ObjetosActivos() const
	{
		int total = 0;

		for (int i = 0; i < MaxObjetos; ++i)
		{
			if (objetos[i].activo)
				total++;
		}

		return total;
	}

	void RefrescarEstado()
	{
		if (fase == Fase::Dialogo)
			sprintf_s(estado, "$%.0f / %.0f\nTicket: $%.0f", pesos, MetaPesos, ticket);
		else
			sprintf_s(estado, "$%.0f / %.0f", pesos, MetaPesos);
	}

	static const int MaxObjetos = 3;
	static const int TotalEstantes = 6;
	static constexpr float RadioCaja = 7.0f;
	static constexpr float RadioRecoger = 3.0f;
	static constexpr float RadioEstante = 4.5f;
	static constexpr float RadioNave = 8.0f;

	Fase fase = Fase::Explorando;
	int cliente = 0;
	int linea = 0;
	int seleccion = -1;
	bool respondida = false;
	int orden[3] = { 0, 1, 2 };
	float pausa = 0.0f;
	float timer = 0.0f;
	float espera = 0.0f;
	float vida = VidaMaxima;
	float pesos = 0.0f;
	float ticket = 0.0f;
	bool clienteMolesto = false;
	int llevando = -1;
	int generacion = 0;
	bool pedirReposicion = false;
	bool pedirRecogida = false;

	ObjetoSuelo objetos[MaxObjetos];
	char lineaVisible[512];
	char opcionVisible[3][320];
	char aviso[512];
	char estado[160];
	char motivo[256];
};

#endif
