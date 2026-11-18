#ifndef _turno_extra_shift
#define _turno_extra_shift

// Maquina de estados del turno en el Oxxo.
//
// Explorando -> (el jugador entra) puerta bloqueada
//   -> cliente en camino -> cliente esperando en la caja
//   -> dialogo de 5 lineas en primera persona
//   -> caen 2 o 3 productos; hay que devolverlos estante por estante
//   -> el siguiente cliente no avanza mientras quede algo en el piso
//   -> al tercer cliente: noche, la puerta se abre, abordar la nave con 600 pesos
//
// La vida baja de golpe: respuesta sospechosa, respuesta alienigena,
// o 20 segundos sin atender a un cliente que ya esta en la caja.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

class TurnoExtraShift
{
public:
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
		opcionMarcada = 0;
		pausa = 0.0f;
		timer = 0.0f;
		espera = 0.0f;
		vida = 100.0f;
		pesos = 0.0f;
		llevando = -1;
		generacion = 0;
		pedirReposicion = false;
		pedirRecogida = false;

		for (int i = 0; i < MaxObjetos; ++i)
			objetos[i].activo = false;

		lineaVisible[0] = '\0';
		aviso[0] = '\0';
		estado[0] = '\0';

		for (int i = 0; i < 3; ++i)
			opcionVisible[i][0] = '\0';
	}

	void Actualizar(
		float dt,
		float jugadorX,
		float jugadorZ,
		bool interactuar,
		int opcion)
	{
		if (dt < 0.0f)
			dt = 0.0f;

		for (int i = 0; i < MaxObjetos; ++i)
		{
			if (!objetos[i].activo || objetos[i].enMano)
				continue;

			objetos[i].caida += dt / 0.45f;
			if (objetos[i].caida > 1.0f)
				objetos[i].caida = 1.0f;
		}

		for (int i = 0; i < MaxObjetos; ++i)
		{
			if (!objetos[i].activo || !objetos[i].enMano)
				continue;

			objetos[i].x = jugadorX;
			objetos[i].z = jugadorZ;
		}

		switch (fase)
		{
		case Fase::Explorando:
			sprintf_s(
				aviso,
				"ENTRA AL OXXO PARA EMPEZAR EL TURNO. LAS PUERTAS SE CERRARAN.");

			if (DentroDelOxxo(jugadorX, jugadorZ))
			{
				fase = Fase::ClienteEnCamino;
				cliente = 0;
				timer = 0.0f;
				sprintf_s(
					aviso,
					"EMPIEZA EL TURNO. LAS PUERTAS QUEDAN BLOQUEADAS.");
			}
			break;

		case Fase::ClienteEnCamino:
			timer += dt;
			sprintf_s(
				aviso,
				"%s SE ACERCA. %s",
				Guion()[cliente].nombre,
				Guion()[cliente].contexto);

			if (timer >= 2.2f)
			{
				fase = Fase::ClienteEsperando;
				espera = 0.0f;
			}
			break;

		case Fase::ClienteEsperando:
			espera += dt;

			while (espera >= 20.0f)
			{
				espera -= 20.0f;
				Danar(20.0f);
				sprintf_s(
					aviso,
					"EL CLIENTE ESPERO MAS DE 20 SEGUNDOS. LA VIDA BAJA DE GOLPE.");

				if (vida <= 0.0f)
				{
					Morir();
					RefrescarEstado();
					return;
				}
			}

			if (interactuar && Distancia(jugadorX, jugadorZ, CajaX, CajaZ) <= RadioCaja)
			{
				EntrarDialogo();
			}
			else if (vida > 0.0f)
			{
				int segundos = (int)(20.0f - espera);
				if (segundos < 0)
					segundos = 0;

				sprintf_s(
					aviso,
					"%s YA ESTA EN LA CAJA. ACERCATE Y PULSA E. QUEDAN %d S.",
					Guion()[cliente].nombre,
					segundos);
			}
			break;

		case Fase::Dialogo:
			if (pausa > 0.0f)
			{
				pausa -= dt;
				if (pausa <= 0.0f)
					AvanzarLinea();
			}
			else if (opcion >= 1 && opcion <= 3)
			{
				Elegir(opcion - 1);
			}
			break;

		case Fase::Limpieza:
		{
			bool evento = false;
			if (interactuar)
				evento = IntentarLimpieza(jugadorX, jugadorZ);

			if (ObjetosActivos() == 0)
			{
				cliente++;
				fase = Fase::ClienteEnCamino;
				timer = 0.0f;
				sprintf_s(aviso, "EL PISO ESTA LIMPIO. LLEGA EL SIGUIENTE CLIENTE.");
				break;
			}

			if (!evento)
			{
				if (llevando >= 0)
				{
					const Estante& estante = Estantes()[objetos[llevando].estante];
					sprintf_s(
						aviso,
						"LLEVAS %s. DEVUELVELO AL ESTANTE X %.0f  Z %.0f Y PULSA E.",
						estante.nombre,
						estante.x,
						estante.z);
				}
				else
				{
					sprintf_s(
						aviso,
						"HAY %d PRODUCTOS EN EL PISO. RECOGELOS CON E. EL SIGUIENTE CLIENTE ESPERA.",
						ObjetosActivos());
				}
			}
			break;
		}

		case Fase::Noche:
			if (Distancia(jugadorX, jugadorZ, NaveX, NaveZ) <= RadioNave)
			{
				if (interactuar)
				{
					if (pesos >= 600.0f)
					{
						fase = Fase::Victoria;
						sprintf_s(
							aviso,
							"ABORDASTE LA NAVE Y COMPRASTE LA PIEZA. GANASTE EL TURNO.");
					}
					else
					{
						sprintf_s(
							aviso,
							"TE FALTAN PESOS PARA LA PIEZA AVERIADA. LLEVAS %.0f DE 600.",
							pesos);
					}
				}
				else
				{
					sprintf_s(
						aviso,
						"ESTAS JUNTO A TU NAVE. PULSA E PARA ABORDARLA.");
				}
			}
			else
			{
				sprintf_s(
					aviso,
					"YA ES DE NOCHE. SAL DEL OXXO Y ABORDA TU NAVE, FRENTE A LA PUERTA.");
			}
			break;

		case Fase::Victoria:
			sprintf_s(
				aviso,
				"JUNTASTE LOS 600 PESOS. LA PIEZA ES TUYA. FIN DEL TURNO.");
			break;

		case Fase::Derrota:
			sprintf_s(
				aviso,
				"TE QUEDASTE SIN VIDA. EL TURNO SE ARRUINO.");
			break;
		}

		RefrescarEstado();
	}

	bool PuertaBloqueada() const
	{
		return fase != Fase::Explorando &&
			fase != Fase::Noche &&
			fase != Fase::Victoria;
	}

	bool ForzarPrimeraPersona() const
	{
		return fase == Fase::Dialogo;
	}

	bool BloquearMovimiento() const
	{
		return fase == Fase::Dialogo;
	}

	bool EsNoche() const
	{
		return fase == Fase::Noche || fase == Fase::Victoria;
	}

	bool EnDialogo() const
	{
		return fase == Fase::Dialogo;
	}

	bool ClienteVisibleEnCaja() const
	{
		return fase == Fase::ClienteEnCamino ||
			fase == Fase::ClienteEsperando ||
			fase == Fase::Dialogo;
	}

	int IndiceCliente() const
	{
		if (cliente < 0)
			return 0;
		if (cliente > 2)
			return 2;
		return cliente;
	}

	float Vida() const
	{
		return vida;
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

	int GeneracionObjetos() const
	{
		return generacion;
	}

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
	const char* LineaCliente() const { return lineaVisible; }
	const char* Opcion(int indice) const
	{
		if (indice < 0 || indice > 2)
			return "";

		return opcionVisible[indice];
	}

	int OpcionMarcada() const
	{
		return opcionMarcada;
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

	// Hueco de la puerta del Oxxo, en coordenadas de mundo.
	// Sale de las cajas entrada_der / entrada_izq (modelo en -35, 25).
	static bool DentroDelOxxo(float x, float z)
	{
		const bool porLaPuerta =
			x > -39.0f && x < -30.0f &&
			z < 43.2f && z > 18.0f;

		const bool enElFondo =
			x > -58.0f && x < -12.0f &&
			z > 8.0f && z < 40.0f;

		return porLaPuerta || enElFondo;
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
				"VECINO APURADO",
				"LLEGA DE PRISA GOLPEANDO EL MOSTRADOR. LLEVA UNA COCA Y UNAS PAPAS.",
				{
					{
						"QUE TRANSA, BRO! ME URGE PAGAR ESTO QUE VOY TARDE AL JALE. ME COBRAS RAPIDO?",
						{
							"CLARO QUE SI, JEFE. SON 45 PESOS EN TOTAL, VA A SER EN EFECTIVO O TARJETA?",
							"AFIRMATIVO, TERRICOLA. INTERCAMBIO ESTE CILINDRO DE AZUCAR POR TUS CREDITOS TERRESTRES.",
							"MIS ANTENAS CAPTAN QUE TU PULSO CARDIACO ES ACELERADO. VENERAS AL GRAN DIOS ZORGON?"
						}
					},
					{
						"TIENES CAMBIO DE UN BILLETE DE 500? ES QUE NO TRAIGO MAS CHICO.",
						{
							"SI TE ALCANZO A DAR CAMBIO, DEJAME REVISAR LA CAJA REGISTRADORA.",
							"EL PAPEL MONEDA CARECE DE VALOR REAL EN MI GALAXIA, PERO ACEPTO TU TRIBUTO.",
							"MUERDE EL BILLETE PARA VERIFICAR QUE NO SEA UN HOLOGRAMA SINTETICO DE LA RESISTENCIA."
						}
					},
					{
						"OYE, ESTA SIRVIENDO EL CAJERO AUTOMATICO DE AFUERA? ES QUE NO TRAIGO EFECTIVO.",
						{
							"LA NETA NO SE, CARNAL, HAZTE EL FAVOR DE CHECARLO TU MISMO SALIENDO A LA IZQUIERDA.",
							"ESE MONOLITO ELECTRONICO SUELE FALLAR CUANDO LAS ALINEACIONES ESTELARES CAMBIAN.",
							"LOS CAJEROS ABSORBEN ELMAS DE LOS HUMANOS DESPREVENIDOS. NO CAIGAS EN SU TRAMPA."
						}
					},
					{
						"ME DAS UNA BOLSA POR FAVOR? SE ME VA A CAER TODO.",
						{
							"CLARO, TEN. ALGO MAS QUE SE TE OFREZCA?",
							"CONTENEDOR DE PLASTICO ASIGNADO PARA TU MATERIA ORGANICA.",
							"EN MI PLANETA LLEVAMOS LAS COSAS FLOTANDO CON TELEQUINESIS, QUE ATRASADOS ESTAN."
						}
					},
					{
						"BUENO, TEN EL DINERO EXACTO. GRACIAS, JOVEN. BUEN DIA!",
						{
							"IGUALMENTE, QUE TE VAYA MUY BIEN EN TU CHAMBA. HASTA LUEGO!",
							"QUE TU TEMPERATURA CORPORAL SE MANTENGA ESTABLE, HUMANO.",
							"LARGA VIDA Y PROSPERIDAD... O LO QUE SEA QUE DURE TU EFIMERA ESPECIE."
						}
					}
				}
			},
			{
				"SENORA DE LOS GATOS",
				"ENTRA CON CALMA ARRASTRANDO LOS PIES Y OLIENDO A CROQUETAS. LLEVA LATAS.",
				{
					{
						"BUENAS TARDES, JOVEN. OIGA, TODAVIA HAY DE ESTAS LATAS DE ATUN EN OFERTA? NO LAS VEO EN EL ESTANTE.",
						{
							"BUENAS TARDES, SENORA. DEJAME CHECAR EN LA BODEGA SI NOS QUEDO ALGO SURTIDO.",
							"LAS PROVISIONES FELINAS ESCASEAN DEBIDO A LA TORMENTA SOLAR DE AYER.",
							"POR QUE ALIMENTAS A ESAS CRIATURAS MUTANTES DE CUATRO PATAS? SON ESPIAS DE OTRO SISTEMA SOLAR."
						}
					},
					{
						"AY, HIJO, TE VES UN POCO PALIDO Y CON LOS OJOS BIEN ABIERTOS. TE SIENTES BIEN?",
						{
							"ES QUE NO HE DORMIDO BIEN, SENORA, PURAS DESVELADAS POR LA CHAMBA.",
							"MI EXOESQUELETO EXTERIOR SE ESTA ADAPTANDO A LA ATMOSFERA TERRESTRE.",
							"MIS FILTROS OCULARES SE ESTAN SOBRECALENTANDO! NO ME MIRES FIJAMENTE."
						}
					},
					{
						"ME PUEDES MARCAR ESTO CON CUIDADO? ES QUE A MI MICHI LE DAN MIEDO LOS GOLPES FUERTES.",
						{
							"SI, NO SE PREOCUPE, SE LOS TRATO CON MUCHO CUIDADO PARA QUE NO SE ABOLLEN.",
							"TRATARE LAS LATAS CON LA PRECISION DE UN RAYO LASER DESINTEGRADOR.",
							"LOS GATOS DE ESTA DIMENSION ME OBSERVAN CON DESPRECIO TELEPATICO."
						}
					},
					{
						"OIGA, SIGUE APLICANDO LO DE DONAR LOS CENTAVOS PARA LA FUNDACION?",
						{
							"SI, GUSTA REDONDEAR SUS CENTAVOS EL DIA DE HOY?",
							"DONARAS TUS RECURSOS A LA CAUSA DE RECONSTRUCCION INTERGALACTICA.",
							"EL DINERO ES UNA ILUSION CAPITALISTA DISENADA PARA ESCLAVIZAR A LOS SIMIOS."
						}
					},
					{
						"TENGA SU DINERO Y SU TICKET. GRACIAS POR LA ATENCION, MIJITO.",
						{
							"DE NADA, QUE TENGA UNA EXCELENTE TARDE Y SALUDOS A SU MICHI.",
							"MIS SENSORES REGISTRAN TRANSACCION EXITOSA. ADIOS, TERRICOLA ANCIANA.",
							"EJECUTANDO PROTOCOLO DE DESPEDIDA HUMANA... BEEP BOOP."
						}
					}
				}
			},
			{
				"SUJETO MISTERIOSO",
				"TRAE LENTES DE SOL DE NOCHE Y ACTUA DE MANERA SOSPECHOSA. COMPRA CAFE Y UN ENCENDEDOR.",
				{
					{
						"BUENAS. VENGO POR UN CAFE BIEN CARGADO. OYE, NO HAS ESCUCHADO RUIDOS RAROS EN EL CIELO ULTIMAMENTE?",
						{
							"BUENAS. LA NETA NO, AQUI EN EL DESIERTO SIEMPRE ESTA BIEN TRANQUILO EL ROLLO.",
							"SON SOLO PRUEBAS DE SATELITES MILITARES GUBERNAMENTALES, NADA DE QUE PREOCUPARSE.",
							"ERAN MIS COMPAS PASANDO A SALUDAR EN SU NAVE NODRIZA! DIGO... CUALES NAVES?"
						}
					},
					{
						"QUE RESPUESTA TAN DEFENSIVA. ESTAS SEGURO DE QUE ERES DE POR AQUI? NUNCA TE HABIA VISTO EN ESTE TURNO.",
						{
							"ACABO DE ENTRAR ESTA SEMANA, CARNAL, POR ESO NO ME UBICAS TODAVIA.",
							"MI ADN CUMPLE CON TODOS LOS ESTANDARES REQUERIDOS POR LA LEY LABORAL MEXICANA.",
							"ME DESCUBRISTE! VENGO DEL CUADRANTE ZETA RETICULI A ESTUDIAR SUS SNACKS."
						}
					},
					{
						"JAJA, TE ESTOY COTORREANDO, TRANQUILO. CUANTO ES DEL CAFE Y EL ENCENDEDOR?",
						{
							"AH, JOTERIA CONTIGO, ME SUSTO. SON 32 PESOS CERRADOS.",
							"TRANSFERENCIA DE 32 UNIDADES MONETARIAS PROCESADA CORRECTAMENTE.",
							"TUS BROMAS DE SIMIO GENERAN CONFUSION EN MIS CIRCUITOS CEREBRALES."
						}
					},
					{
						"OYE, SE TE CAYERON UNAS MONEDAS HACE RATO Y CAMINAS BIEN RARO, COMO SI TE DOLIERAN LAS RODILLAS.",
						{
							"ES QUE ANDO BIEN CANSADO POR LA MALA POSTURA DE ESTAR PARADO TODO EL TURNO.",
							"LAS ARTICULACIONES BIPEDESTAS DE LOS HUMANOS SON MUY DEFECTUOSAS.",
							"MIS SERVOMECANISMOS INTERNOS NECESITAN LUBRICACION CON ACEITE DE MOTOR ESPACIAL."
						}
					},
					{
						"BUENO, TEN TU LANA. SUERTE CON TU TURNO, COMPA... Y CUIDATE DE LO QUE CAE DEL CIELO.",
						{
							"GRACIAS, IGUALMENTE, CUIDATE MUCHO EN LA CALLE. NOS VEMOS.",
							"MIS SISTEMAS DE NAVEGACION ESTAN LISTOS PARA EL DESPEGUE NOCTURNO.",
							"ACTIVANDO SECUENCIA DE ESCAPE ESTELAR EN 3... 2... 1..."
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
			{ -20.0f, 35.0f, L"Assets/Billboards/cokita.png", "COKITA" },
			{ -24.0f, 35.0f, L"Assets/Billboards/doritos.png", "DORITOS" },
			{ -28.0f, 35.0f, L"Assets/Billboards/boing.png", "BOING" },
			{ -32.0f, 30.0f, L"Assets/Billboards/oreo.png", "OREO" },
			{ -36.0f, 30.0f, L"Assets/Billboards/nutri.png", "NUTRI" },
			{ -40.0f, 25.0f, L"Assets/Billboards/hersheys.png", "HERSHEYS" },
		};

		return estantes;
	}

	void Danar(float cantidad)
	{
		vida -= cantidad;
		if (vida < 0.0f)
			vida = 0.0f;
	}

	void Morir()
	{
		vida = 0.0f;
		fase = Fase::Derrota;
		llevando = -1;
		sprintf_s(
			aviso,
			"TE QUEDASTE SIN VIDA. DESCUBRIERON QUE NO ERES DE AQUI.");
	}

	void EntrarDialogo()
	{
		fase = Fase::Dialogo;
		linea = 0;
		opcionMarcada = 0;
		pausa = 0.0f;
		pedirReposicion = true;
		ConstruirTextos();
		sprintf_s(aviso, "PULSA 1, 2 O 3 PARA RESPONDER.");
	}

	void Elegir(int indice)
	{
		opcionMarcada = indice;

		if (indice == 0)
		{
			vida += 8.0f;
			if (vida > 100.0f)
				vida = 100.0f;

			sprintf_s(aviso, "RESPUESTA HUMANA. EL CLIENTE NO SOSPECHA.");
		}
		else if (indice == 1)
		{
			Danar(12.0f);
			sprintf_s(aviso, "RESPUESTA SOSPECHOSA. PIERDES UN BLOQUE DE VIDA.");
		}
		else
		{
			Danar(30.0f);
			sprintf_s(aviso, "RESPUESTA ALIENIGENA. LA VIDA BAJA DE GOLPE.");
		}

		if (vida <= 0.0f)
		{
			Morir();
			return;
		}

		pausa = 0.55f;
	}

	void AvanzarLinea()
	{
		linea++;
		opcionMarcada = 0;

		if (linea >= 5)
		{
			TerminarCliente();
			return;
		}

		ConstruirTextos();
		sprintf_s(aviso, "PULSA 1, 2 O 3 PARA RESPONDER.");
	}

	void TerminarCliente()
	{
		pesos += 200.0f;

		if (cliente >= 2)
		{
			fase = Fase::Noche;
			sprintf_s(
				aviso,
				"ATENDISTE AL TERCER CLIENTE. ANOCHECIO. JUNTASTE %.0f PESOS.",
				pesos);
			return;
		}

		fase = Fase::Limpieza;
		SoltarObjetos();
		sprintf_s(
			aviso,
			"EL CLIENTE SE FUE. CAYERON %d PRODUCTOS DE LOS ESTANTES.",
			ObjetosActivos());
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

		while (creados < cantidad && guardia < 40)
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
			objeto.z = Estantes()[estante].z + 1.4f;
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
				sprintf_s(aviso, "PRODUCTO DEVUELTO A SU ESTANTE.");
				return true;
			}

			for (int i = 0; i < TotalEstantes; ++i)
			{
				if (i == destino)
					continue;

				if (Distancia(jugadorX, jugadorZ, Estantes()[i].x, Estantes()[i].z) <= RadioEstante)
				{
					sprintf_s(aviso, "ESE NO ES SU ESTANTE. BUSCA EL MARCADO EN EL AVISO.");
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

			const float distancia = Distancia(
				jugadorX,
				jugadorZ,
				objetos[i].x,
				objetos[i].z);

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
		sprintf_s(
			aviso,
			"RECOGISTE %s. LLEVALO A SU ESTANTE.",
			Estantes()[objetos[mejor].estante].nombre);
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

	void ConstruirTextos()
	{
		if (cliente < 0 || cliente > 2 || linea < 0 || linea > 4)
			return;

		const ClienteGuion& guion = Guion()[cliente];
		const LineaGuion& actual = guion.lineas[linea];

		sprintf_s(lineaVisible, "%s: %s", guion.nombre, actual.cliente);

		for (int i = 0; i < 3; ++i)
		{
			sprintf_s(
				opcionVisible[i],
				"%d  %s",
				i + 1,
				actual.opciones[i]);
		}
	}

	void RefrescarEstado()
	{
		sprintf_s(
			estado,
			"VIDA %.0f    PESOS %.0f / 600",
			vida,
			pesos);
	}

	static const int MaxObjetos = 3;
	static const int TotalEstantes = 6;
	static constexpr float CajaX = -48.0f;
	static constexpr float CajaZ = 30.0f;
	static constexpr float PuestoX = -45.5f;
	static constexpr float PuestoZ = 30.0f;
	static constexpr float RadioCaja = 7.0f;
	static constexpr float RadioRecoger = 2.6f;
	static constexpr float RadioEstante = 4.2f;
	static constexpr float NaveX = -35.0f;
	static constexpr float NaveZ = 62.0f;
	static constexpr float RadioNave = 8.0f;

	Fase fase = Fase::Explorando;
	int cliente = 0;
	int linea = 0;
	int opcionMarcada = 0;
	float pausa = 0.0f;
	float timer = 0.0f;
	float espera = 0.0f;
	float vida = 100.0f;
	float pesos = 0.0f;
	int llevando = -1;
	int generacion = 0;
	bool pedirReposicion = false;
	bool pedirRecogida = false;

	ObjetoSuelo objetos[MaxObjetos];
	char lineaVisible[420];
	char opcionVisible[3][260];
	char aviso[420];
	char estado[160];
};

#endif
