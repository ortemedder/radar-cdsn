# Radar CDSN en ESP32 (proyecto aparte del Panel Elite)

> **IMPORTANTE:** paquete de placas **esp32 2.0.17** (NO la 3.x: con la 3.x la pantalla se cuelga y el radar se reinicia). Si el IDE ofrece actualizar el paquete esp32, decir que no.

Radar de la facción Caballeros de Sion en una pantalla propia, sin la Raspberry.
Lee `faccion.json` (el que sube el escucha del pueblo a GitHub `ortemedder/cdsn-datos`) por WiFi cada 10 min.

## Hardware
- ESP32 DevKit 38 pines con **ESP32-WROOM-32U** (¡conectar la antena externa al u.FL!).
- Pantalla **Kuman 3,5" para Raspberry Pi** (480×320, ILI9486, táctil XPT2046), la que se quitó del panel.
- 10 cables dupont hembra-hembra (el 3,3 V NO se conecta: la pantalla lo saca de los 5 V). Alimentación por USB, 5 V ≥ 1 A.
- Alternativa: M5Stack StackChan (CoreS3, 2" 320×240, altavoz y servos).

## Conexión (pin de la pantalla → ESP32)
| Pantalla | Función | ESP32 |
|---|---|---|
| 2 | 5 V | 5V |
| 6 | GND | GND |
| 19 | MOSI | 23 |
| 21 | MISO | 19 |
| 23 | SCLK | 18 |
| 24 | CS pantalla | 5 |
| 26 | CS táctil | 21 |
| 18 | DC | 16 |
| 22 | RESET | 17 |
| 11 | IRQ táctil | 4 |

Señales a 3,3 V: compatibles directamente. Nunca 5 V a un pin de datos.

## Programa
- Arduino IDE + placa «ESP32 Dev Module» (paquete esp32 de Espressif; si TFT_eSPI da problemas, usar la versión 2.0.17 del paquete).
- Librería **TFT_eSPI de Bodmer** (NO la «TFT_eSPI_ES32Lab»): copiar `User_Setup.h` encima del de la librería.
- `prueba_pantalla/`: colores + calibración del táctil + puntos donde se toca.
- `diagnostico/`: muestra por el Monitor Serie qué pines usa TFT_eSPI (deben ser 23/19/18, 5/16/17, 21).
- Ruta de la librería: `C:\Users\OEDMSISHOW\Documents\Arduino\libraries\TFT_eSPI`.
- **Calibración táctil: `uint16_t cal[5] = {374, 3413, 468, 3160, 3};`** (11/11/2026)
- `radar/`: EL RADAR. Necesita además la librería **ArduinoJson** (de Benoit Blanchon, versión 7).
  Poner el nombre y la clave del WiFi (2,4 GHz) al principio de `radar.ino`.
  Lee faccion.json con filtro (solo lo que se pinta, ~50 KB de RAM).
- Alertas: guerra/elecciones activas o pendientes, retirada, ventaja < 5 puntos en un controlado (graves, aro ROJO que late);
  bajada ≥ 3 puntos desde ayer y expansión pendiente (avisos, aro AMARILLO fijo).
- Manejo: tocar sistema o caja de alerta = ficha; tocar vacío = centrar ahí; + / − zoom; C = vista inicial;
  BOOT = pausa del barrido; pulsación larga (1,5 s) = pantalla en negro (otro toque la enciende); la ficha vuelve sola al minuto.

## Diseño acordado
- Mapa con tus sistemas (cuadrado = controlado; verde >50 %, amarillo >20 %, naranja menos), aro que parpadea en los que tienen alerta.
- Barra de radar girando (una vuelta cada 4 s, sentido horario, estela que se apaga); el sistema se ilumina al pasar.
- Derecha: controlados, alertas, tráfico de hoy. Botones + y − de zoom.
- Tocar un sistema: ficha (influencia, cambio desde ayer, aviso, reparto de facciones, estados, tráfico) y VOLVER.
- Botón BOOT: pausa / apagar pantalla. Pulsación larga en la pantalla: apagarla. Opcional: apagado nocturno salvo alertas graves.
- Sin actividad EDDN en directo (demasiado para un ESP32).

Imágenes de prueba en `imagenes/`.

## Versiones del radar
- v1.0: primera versión (mapa, barrido, panel, ficha).
- v1.1: la ficha ya no se cierra sola; menos nombres en el mapa; toques sin rebote.
- v1.2: alertas de 3 en 3 con «siguientes >»; presentación «Cmdr ORTEM ED DER»; WiFi en `wifi.h`.
- v1.3: aros rojos que laten (graves) y amarillos fijos (avisos).
- v1.4: número de versión abajo a la izquierda del mapa y en la presentación.
- v1.5: tocar en cualquier parte del aro (rojo o amarillo) abre la ficha; los sistemas con aro tienen prioridad al tocar.
- v1.6: título RADAR CDSN en azul; hora y tiempo de actualización en rosa; CONTROLADOS, ALERTAS y TRAFICO HOY en verde.
- v1.7: WiFi apagado entre descargas; descarga cada 10 min (si falla, reintenta al minuto) y vuelve a apagar el WiFi. Icono de WiFi en el título: verde = conectado, amarillo = conectando, gris tachado en rojo = apagado.
- v1.8: alerta nueva = destello de pantalla + etiqueta NUEVA y sale la primera (hasta abrir su ficha);
  «act. hace…» en rojo si el escucha lleva más de 1 h sin subir datos; tocar «CONTROLADOS >» abre la lista
  de controlados de menor a mayor influencia (con ventaja sobre la 2.ª facción); en la ficha, conflicto con
  cuadritos verdes (días ganados) y rojos (perdidos). Cambia también `datos.h`.
- v1.9: pantalla girada 180° (GIRAR_180 1); el táctil se corrige solo. Para volver a la posición normal: #define GIRAR_180 0 en wifi.h.
- v2.0: código ordenado en pestañas (config.h, utiles.h, botones.h, mapa.h, panel.h, ficha.h, red.h);
  posiciones con nombre en config.h; una sola tabla de botones (dibujar y tocar); senos/cosenos y
  distancias precalculados (el barrido gasta menos CPU); nombres cortos («Hyades SU-N b7-5») desde la carga.
  TICK: en el panel «PROX. TICK ~15:30» con el sistema y cuánto falta; en la ficha último tick (± margen)
  y próximo previsto. Necesita el escucha con detección de tick (escucha_faccion.py con tick_track).
  La escala «anillo X ly» pasa a la esquina inferior derecha del mapa; TRAFICO HOY sin la palabra «naves».
- v2.1: hasta 120 sistemas (tabla en memoria dinámica y textos internos algo más cortos para que quepa) (la facción ya va por 78); PROX. TICK usa el tick del sistema solo si su
  previsión es fiable (± 3 h o menos) y si no, el tick global de la galaxia (+24 h).
  Escucha: cruza ventanas de hasta 20 h de varios días (las cortas pesan más) y lee el tick global
  de tick.edcd.io cada 30 min (campo tick_global).
- v2.2: el mapa se dibuja en memoria (sprite de 4 bits, 16 colores, 43 KB) y solo se manda a la pantalla
  lo que cambia: sin nombres cortados ni restos del barrido; estela en 7 tonos.
  Ficha: botón GRAFICA / FACCIONES con la gráfica de influencia (escalones, últimos 10 cambios).
  Escucha: guarda el histórico de influencia de cada sistema (campo hinf, últimos 14 cambios).
- v2.2b: el mapa en memoria se suelta durante la descarga y se vuelve a crear al repintar (arregla JSON NoMemory).
- v2.2c: el mapa en memoria va en dos mitades de 21,6 KB (caben en la RAM troceada tras la descarga); si no hay sitio, reintenta cada 5 s.
- v2.3: faccion.json se lee sistema a sistema (sin cargarlo entero): arregla JSON NoMemory al crecer la facción (ya 91 sistemas); se ignoran los sistemas donde CDSN ya no está.
- v2.3b: la tabla de sistemas se reserva en setup(); si no hay memoria lo dice en pantalla en vez de reiniciarse.
