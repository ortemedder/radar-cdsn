// RADAR CDSN - Caballeros de Sion en una pantalla Kuman 3,5" con ESP32 WROOM-32U
// Lee faccion.json de GitHub (ortemedder/cdsn-datos) cada 10 minutos.
// Librerias: TFT_eSPI (Bodmer, con nuestro User_Setup.h) y ArduinoJson 7 (Benoit Blanchon).
// Placa: ESP32 Dev Module · paquete esp32 2.0.17.
//
// Archivos (pestanas del Arduino IDE):
//   radar.ino  arranque, bucle y toques        config.h  version, posiciones, colores, estado
//   datos.h    lectura de faccion.json         utiles.h  ayudas (textos, horas, colores)
//   botones.h  tabla de botones                mapa.h    radar, nombres y aros
//   panel.h    titulo, panel y alertas         ficha.h   ficha de sistema y lista
//   red.h      WiFi y descarga                 mi_wifi.h TU WiFi (no se toca al actualizar)

#include <new>
#include "config.h"
#include "utiles.h"
#include "botones.h"
#include "mapa.h"
#include "panel.h"
#include "ficha.h"
#include "red.h"

void toque(int x, int y) {
  if (pulsaBoton(x, y)) return;                    // botones de la tabla (todas las pantallas)
  if (modo == LISTA) { tocaLista(x, y); return; }
  if (modo != MAPA) return;
  fichaDesdeLista = false;
  for (int k = 0; k < AL_N; k++) {                 // cajas de alerta
    int i = orden[pagAlerta * AL_N + k];
    if (i >= 0 && dentro(x, y, PX, AL_Y + k * AL_PASO, PW, AL_H + 2)) { pantallaFicha(i); return; }
  }
  if (dentro(x, y, MX, MY, MW, MH)) {              // mapa: sistema o centrar ahi
    int i = sistemaEn(x, y);
    if (i >= 0) { pantallaFicha(i); return; }
    float k = ZOOMS[zoom];
    centroX += (x - CX) / k; centroZ -= (y - CY) / k;
    pantallaMapa();
  }
}

void apagar() { modo = APAGADA; tft.fillScreen(TFT_BLACK); }

void colores() {
  NAR = tft.color565(255, 140, 0);    NAR_MED = tft.color565(170, 95, 10);  NAR_OSC = tft.color565(80, 45, 0);
  FONDO = tft.color565(18, 11, 2);    REJILLA = tft.color565(55, 32, 5);    LINEA = tft.color565(30, 18, 3);
  VERDE = tft.color565(90, 220, 120); AMAR = tft.color565(255, 210, 40);    AZUL = tft.color565(90, 170, 255);
  ROJO = tft.color565(255, 70, 50);   ROSA = tft.color565(255, 110, 200);   BLANCO = TFT_WHITE;
  ILUM = tft.color565(255, 235, 180);
  for (int i = 0; i < N_ESTELA; i++) {
    float t = (float)i / N_ESTELA;
    estela[i] = i == 0 ? tft.color565(255, 200, 90) : mezcla(tft.color565(200, 110, 0), FONDO, t * t * 0.4f + t * 0.6f);
  }
}

void presentacion() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(NAR_MED); tft.drawString("Cmdr", ANCHO / 2, 95, 4);
  tft.setTextColor(NAR); tft.setTextSize(2); tft.drawString("ORTEM ED DER", ANCHO / 2, 150, 4); tft.setTextSize(1);
  tft.drawFastHLine(90, 190, 300, NAR_OSC);
  tft.setTextColor(NAR_MED); tft.drawString("Caballeros de Sion  -  RADAR CDSN", ANCHO / 2, 212, 2);
  tft.setTextColor(NAR_OSC); tft.drawString(VERSION, ANCHO / 2, 300, 2);
  delay(3000);
}

void setup() {
  Serial.begin(115200);
  Serial.println("RADAR CDSN " VERSION);
  Serial.printf("RAM libre %u (bloque mayor %u), tabla de sistemas %u bytes\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap(), (unsigned)sizeof(Datos));
  pDatos = (Datos *)calloc(1, sizeof(Datos));      // tabla de sistemas
  if (pDatos) new (pDatos) Datos();
  pinMode(BOTON_BOOT, INPUT_PULLUP);
  tft.init(); tft.setRotation(GIRAR_180 ? 3 : 1); tft.setTouch(CAL);
  colores();
  if (!pDatos) {                                   // sin memoria para la tabla: se dice y se para
    tft.fillScreen(TFT_BLACK); tft.setTextColor(ROJO); tft.setTextDatum(MC_DATUM);
    tft.drawString("Sin memoria para los datos", ANCHO / 2, ALTO / 2, 4);
    Serial.println("ERROR: sin memoria para la tabla de sistemas");
    while (true) delay(1000);
  }
  preparaTablas();
  preparaSprite();
  presentacion();
  tft.fillScreen(TFT_BLACK);
  titulo("RADAR CDSN", "conectando WiFi...");
  tft.setTextColor(NAR_MED); tft.setTextDatum(MC_DATUM);
  tft.drawString(WIFI_NOMBRE, ANCHO / 2, 160, 4);
  if (conectaWifi()) {
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");
    for (int i = 0; i < 20 && !ahora(); i++) delay(250);
  }
  actualiza(true);
}

void loop() {
  uint32_t m = millis();

  // boton BOOT: pausa del barrido
  static int bootAntes = HIGH;
  int b = digitalRead(BOTON_BOOT);
  if (b == LOW && bootAntes == HIGH && modo == MAPA) { pausa = !pausa; titulo("RADAR CDSN"); delay(30); }
  bootAntes = b;

  // tactil: toque corto = accion, largo (1,5 s) = pantalla en negro
  static bool tocando = false, largo = false;
  static uint32_t tToque = 0, tSuelta = 0;
  static uint16_t ux, uy;                          // ultimo punto tocado
  uint16_t x, y;
  bool t = tft.getTouch(&x, &y);
  if (t && GIRAR_180) { x = ANCHO - 1 - x; y = ALTO - 1 - y; }   // el tactil se calibro sin girar
  if (t) { ux = x; uy = y; tSuelta = m; }
  if (t && !tocando) { tocando = true; tToque = m; largo = false;
    if (modo == APAGADA) { largo = true; pantallaMapa(); } }
  if (t && tocando && !largo && m - tToque > 1500) { largo = true; apagar(); }
  if (!t && tocando && m - tSuelta > 80) { tocando = false; if (!largo && modo != APAGADA) toque(ux, uy); }

  // descarga cada 10 min
  if (m - tDescarga > CADA_MS) actualiza(false);
  if (!hayDatos) return;

  if (modo == MAPA) {
    if (m - tPaso >= MS_PASO) { tPaso = m; tickMapa(); }
    if (m - tReloj > 30000) { tReloj = m; titulo("RADAR CDSN"); panelTick(); }
  } else if ((modo == FICHA || modo == LISTA) && millis() - tFicha > 60000) pantallaMapa();   // vuelve sola al minuto
}
