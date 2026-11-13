// config.h - version, constantes, posiciones de pantalla y variables globales
#pragma once
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include "datos.h"

#define VERSION "v2.3b"      // subir el numero en cada cambio
#if CONFIG_IDF_TARGET_ESP32S3
#error "Este radar es para el ESP32 normal: Herramientas -> Placa -> ESP32 Dev Module"
#endif
#include "mi_wifi.h"        // nombre y clave del WiFi (archivo aparte para no perderlos al actualizar)
#ifndef GIRAR_180
#define GIRAR_180 1         // 1 = pantalla girada 180 grados, 0 = normal (tambien se puede poner en mi_wifi.h)
#endif

// ---------- red ----------
const char *URL = "https://raw.githubusercontent.com/ortemedder/cdsn-datos/main/faccion.json";
const uint32_t CADA_MS = 10UL * 60 * 1000;         // descarga cada 10 min (WiFi apagado entre medias)
const uint32_t REINTENTO_MS = 60UL * 1000;         // si falla, reintenta al minuto
uint16_t CAL[5] = {374, 3413, 468, 3160, 3};       // calibracion del tactil
const int BOTON_BOOT = 0;

// ---------- pantalla ----------
const int ANCHO = 480, ALTO = 320;
const int TIT_H = 27;                              // barra de titulo
const int WIFI_X = 200, WIFI_Y = 5;                // icono WiFi

// mapa
const int MX = 2, MY = 30, MW = 300, MH = 288;
const int CX = MX + MW / 2, CY = MY + MH / 2, R = 138;
const float ZOOMS[] = {0.08, 0.12, 0.18, 0.27, 0.4, 0.6, 0.9, 1.35, 2, 3, 4.5, 7, 10};
const int NZ = sizeof(ZOOMS) / sizeof(ZOOMS[0]);
const int TOQUE_ARO = 24, TOQUE_SIS = 15;          // radio para tocar un sistema con aro / sin aro

// panel derecho
const int PX = 310, PW = ANCHO - PX - 2;
const int CTRL_Y = 32;                             // CONTROLADOS
const int AL_TIT_Y = 80, AL_Y = 98, AL_PASO = 40, AL_H = 36, AL_N = 3;   // cajas de alerta
const int SIG_Y = 216, SIG_H = 18;                 // "siguientes >"
const int TRAF_Y = 238;                            // TRAFICO HOY
const int TICK_Y = 262;                            // PROX. TICK
const int BOT_Y = 288, BOT_W = 50, BOT_H = 28, BOT_PASO = 58;   // + - C

// ficha y lista
const int VOLVER_X = 370, VOLVER_Y = 284, VOLVER_W = 106, VOLVER_H = 32;
const int LISTA_Y = 44, LISTA_PASO = 19, FILAS = 12;
const int LSIG_X = 6, LSIG_W = 200;
const int GRAF_BX = 256, GRAF_BW = 106;           // boton GRAFICA / FACCIONES de la ficha
const int GRAF_X = 6, GRAF_Y = 158, GRAF_W = 470, GRAF_H = 88;   // zona de la grafica

// barrido
const int N_ESTELA = 14, PASO = 4;                 // grados por paso; 90 pasos = 1 vuelta
const uint32_t MS_PASO = 4000 / (360 / PASO);      // una vuelta cada 4 s

// ---------- objetos y colores ----------
TFT_eSPI tft;
Datos *pDatos = nullptr;                           // se reserva en setup() (memoria dinamica)
#define datos (*pDatos)
uint16_t ROSA, NAR, NAR_OSC, NAR_MED, FONDO, REJILLA, VERDE, AMAR, AZUL, ROJO, BLANCO, ILUM, LINEA;
uint16_t estela[N_ESTELA];
float SENO[360], COSENO[360];                      // tablas: se calculan una vez

// ---------- estado ----------
enum Modo : uint8_t { MAPA, FICHA, LISTA, APAGADA };
Modo modo = MAPA;
int zoomDef = 6, zoom = 6;
float centroX = 0, centroZ = 0, defX = 0, defZ = 0;
int fichaSis = -1, angulo = 0, estadoWifi = 0;
bool pausa = false, hayDatos = false, fichaDesdeLista = false, fichaGrafica = false;
int fasePulso = 0, radioAro = 8;                   // latido de los aros rojos (radio 8..13)
uint32_t tPaso = 0, tDescarga = 0, tParpadeo = 0, tReloj = 0, tFicha = 0;
char ultimoError[64] = "";
int orden[MAX_SIS], pagAlerta = 0;                 // alertas ordenadas y pagina (de 3 en 3)
int ordenLista[MAX_SIS], nLista = 0, pagLista = 0; // lista de controlados
uint32_t alertasAntes[MAX_SIS]; int nAntes = -1;   // alertas de la descarga anterior (hash del nombre)
uint32_t nuevas[MAX_SIS]; int nNuevas = 0;         // alertas nuevas aun no vistas

// ---------- funciones que se usan antes de definirse ----------
void pantallaMapa(); void pantallaFicha(int i); void pantallaLista(); void panel();
void titulo(const char *izq, const char *extra = nullptr);
void ordenaAlertas(); void iconoWifi(int e);
