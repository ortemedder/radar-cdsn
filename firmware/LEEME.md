# Firmware para la actualización sin cable (OTA)

- `version.txt`: la versión publicada. El radar la mira cada 6 h.
- `radar.bin`: el programa compilado (ESP32 Dev Module, esp32 2.0.17) **sin el WiFi** (usa el guardado en la placa).

Si la versión de aquí es mayor que la del radar, sale en verde en el título y en la pantalla ESTADO
(pulsación larga en el título) aparece el botón ACTUALIZAR.
