// TFT_eSPI · Pantalla Kuman 3,5" para Raspberry Pi (ILI9486 + tactil XPT2046)
// conectada a una ESP32 DevKit con WROOM-32U.
// Copiar este archivo encima de:  Documentos/Arduino/libraries/TFT_eSPI/User_Setup.h

#define ILI9486_DRIVER
#define RPI_DISPLAY_TYPE        // pantallas de Raspberry Pi (registros de 16 bits)

#define TFT_MISO 19             // pin 21 de la pantalla
#define TFT_MOSI 23             // pin 19
#define TFT_SCLK 18             // pin 23
#define TFT_CS    5             // pin 24
#define TFT_DC   16             // pin 18
#define TFT_RST  17             // pin 22
#define TOUCH_CS 21             // pin 26   (IRQ del tactil, pin 11 -> GPIO 4, no hace falta en la libreria)

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY        20000000   // si sale ruido o colores raros, bajar a 16000000
#define SPI_READ_FREQUENCY   16000000
#define SPI_TOUCH_FREQUENCY   2500000
