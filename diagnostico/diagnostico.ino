// Diagnostico pantalla Kuman + ESP32
// Abre el Monitor Serie a 115200 y pulsa EN en el ESP32.
#include <TFT_eSPI.h>
TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(1500);
  setup_t s;
  tft.getSetup(s);
  Serial.println("\n=== CONFIGURACION QUE USA TFT_eSPI ===");
  Serial.printf("Driver: 0x%X  (0x9486 = ILI9486 bien)\n", s.tft_driver);
  Serial.printf("MOSI %d  MISO %d  SCLK %d\n", s.pin_tft_mosi, s.pin_tft_miso, s.pin_tft_clk);
  Serial.printf("CS %d  DC %d  RST %d  TOUCH_CS %d\n", s.pin_tft_cs, s.pin_tft_dc, s.pin_tft_rst, s.pin_tch_cs);
  Serial.printf("Ancho %d  Alto %d  SPI %d Hz\n", s.tft_width, s.tft_height, s.tft_spi_freq * 100000);
  Serial.println("Debe salir: MOSI 23 MISO 19 SCLK 18 / CS 5 DC 16 RST 17 TOUCH_CS 21");
  tft.init();
  tft.setRotation(1);
}

void loop() {
  uint16_t c[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK};
  const char *n[] = {"ROJO", "VERDE", "AZUL", "NEGRO"};
  for (int i = 0; i < 4; i++) {
    tft.fillScreen(c[i]);
    Serial.printf("Pintando %s\n", n[i]);
    delay(1000);
  }
}
