// Prueba de la pantalla y del tactil para el radar Elite (ESP32 WROOM-32U + Kuman 3,5")
// 1) Al arrancar pinta barras de colores y un texto.
// 2) Pide tocar las 4 esquinas para calibrar el tactil y muestra los numeros (apuntalos).
// 3) Despues, donde toques sale un punto naranja.
#include <TFT_eSPI.h>
TFT_eSPI tft;
#define NARANJA 0xFC60

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);                       // horizontal, 480x320
  tft.fillScreen(TFT_BLACK);
  uint16_t col[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, NARANJA, TFT_YELLOW};
  for (int i = 0; i < 6; i++) tft.fillRect(i * 80, 0, 80, 120, col[i]);
  tft.setTextColor(NARANJA, TFT_BLACK);
  tft.drawString("RADAR CDSN - prueba de pantalla", 10, 140, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("rojo verde azul blanco naranja amarillo", 10, 180, 2);
  tft.drawString("Si los colores salen cambiados, dimelo.", 10, 200, 2);
  tft.drawString("Toca la pantalla para calibrar el tactil...", 10, 260, 2);
  uint16_t x, y;
  while (!tft.getTouchRaw(&x, &y) || tft.getTouchRawZ() < 400) delay(20);
  delay(400);

  uint16_t cal[5];
  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("Toca la flecha de cada esquina", 240, 150, 2);
  tft.calibrateTouch(cal, NARANJA, TFT_BLACK, 15);
  tft.setTouch(cal);

  char t[80];
  snprintf(t, sizeof t, "CAL: %u, %u, %u, %u, %u", cal[0], cal[1], cal[2], cal[3], cal[4]);
  Serial.println(t);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(t, 10, 10, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Apunta estos 5 numeros. Ahora toca donde quieras:", 10, 50, 2);
}

void loop() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) tft.fillCircle(x, y, 4, NARANJA);
  delay(10);
}
