// estado.h - (v2.8) WiFi guardado en la placa, datos guardados (cache), actualizacion sin cable (OTA) y pantalla ESTADO
#pragma once




// ---------- WiFi y giro guardados en la placa ----------
// Al grabar con cable con tu mi_wifi.h, el WiFi se guarda en la placa. Las versiones que llegan sin cable (OTA)
// se compilan sin tu WiFi (para no subir la clave a GitHub) y usan el guardado.
void cargaWifi() {
  Preferences p; p.begin("radar", false);
  if (strcmp(WIFI_NOMBRE, "TU_WIFI")) {            // mi_wifi.h con WiFi de verdad: se guarda
    p.putString("nom", WIFI_NOMBRE); p.putString("clave", WIFI_CLAVE); p.putBool("girar", GIRAR_180);
  }
  p.getString("nom", wifiNom, sizeof wifiNom); p.getString("clave", wifiClave, sizeof wifiClave);
  girar = p.getBool("girar", GIRAR_180);
  p.end();
  if (!wifiNom[0]) { strncpy(wifiNom, WIFI_NOMBRE, sizeof wifiNom - 1); strncpy(wifiClave, WIFI_CLAVE, sizeof wifiClave - 1); }
}

// ---------- datos guardados: al encender sale el mapa al momento ----------
const uint32_t MAGIA = 0xCD5A0028;
struct CabCache {
  uint32_t magia; uint16_t tamSis; char ver[12];
  int n, sinSitio; long t, tickGlobal;
  int controlados, alertas, trafHoy, ataques, inusuales, favor, alarmas;
  int ctrlFuera;
};
bool fsOk = false;

void iniciaFS() { fsOk = LittleFS.begin(true); Serial.printf("Memoria de datos: %s\n", fsOk ? "OK" : "FALLO"); }

void guardaCache() {
  if (!fsOk) return;
  CabCache c; memset(&c, 0, sizeof c);
  c.magia = MAGIA; c.tamSis = sizeof(Sis); strncpy(c.ver, VERSION, sizeof c.ver - 1);
  c.n = datos.n; c.sinSitio = datos.sinSitio; c.t = datos.t; c.tickGlobal = datos.tickGlobal;
  c.controlados = datos.controlados; c.alertas = datos.alertas; c.trafHoy = datos.trafHoy; c.ataques = datos.ataques;
  c.inusuales = datos.inusuales; c.favor = datos.favor; c.alarmas = datos.alarmas; c.ctrlFuera = datos.ctrlFuera;
  fs::File f = LittleFS.open("/datos.tmp", "w"); if (!f) return;
  bool ok = f.write((uint8_t *)&c, sizeof c) == sizeof c && f.write((uint8_t *)datos.s, datos.n * sizeof(Sis)) == datos.n * sizeof(Sis);
  f.close();
  if (!ok) { LittleFS.remove("/datos.tmp"); return; }
  LittleFS.remove("/datos.bin"); LittleFS.rename("/datos.tmp", "/datos.bin");   // se cambia de golpe: nunca queda a medias
  cacheT = datos.t;
}

bool leeCache() {
  if (!fsOk) return false;
  fs::File f = LittleFS.open("/datos.bin", "r"); if (!f) return false;
  CabCache c;
  bool ok = f.read((uint8_t *)&c, sizeof c) == sizeof c && c.magia == MAGIA && c.tamSis == sizeof(Sis)
            && !strcmp(c.ver, VERSION) && c.n > 0 && c.n <= datos.cap
            && f.read((uint8_t *)datos.s, c.n * sizeof(Sis)) == c.n * sizeof(Sis);
  f.close();
  if (!ok) return false;                           // de otra version o roto: se espera a la descarga
  datos.n = c.n; datos.sinSitio = c.sinSitio; datos.t = c.t; datos.tickGlobal = c.tickGlobal;
  datos.controlados = c.controlados; datos.alertas = c.alertas; datos.trafHoy = c.trafHoy; datos.ataques = c.ataques;
  datos.inusuales = c.inusuales; datos.favor = c.favor; datos.alarmas = c.alarmas; datos.ctrlFuera = c.ctrlFuera;
  for (int i = 0; i < datos.n; i++) datos.s[i].nueva = false;
  cacheT = c.t; desdeCache = true;
  return true;
}

// ---------- version nueva en GitHub y actualizacion sin cable ----------
const char *URL_VER = "https://raw.githubusercontent.com/ortemedder/radar-cdsn/main/firmware/version.txt";
const char *URL_BIN = "https://raw.githubusercontent.com/ortemedder/radar-cdsn/main/firmware/radar.bin";
const uint32_t CADA_VER_MS = 6UL * 3600 * 1000;    // mira si hay version nueva cada 6 h

long numVersion(const char *v) {                   // "v2.10.1" -> 21001
  int a = 0, b = 0, c = 0; if (*v == 'v') v++;
  sscanf(v, "%d.%d.%d", &a, &b, &c); return a * 10000L + b * 100 + c;
}
bool hayNueva() { return verGit[0] && numVersion(verGit) > numVersion(VERSION); }

void miraVersion() {                               // con el WiFi ya encendido (lo llama la descarga)
  if (tVer && millis() - tVer < CADA_VER_MS) return;
  WiFiClientSecure cli; cli.setInsecure();
  HTTPClient http; http.setTimeout(10000);
  if (!http.begin(cli, URL_VER)) return;
  if (http.GET() == 200) {
    String v = http.getString(); v.trim();
    if (v.length() > 1 && v.length() < sizeof verGit && v[0] == 'v') { strcpy(verGit, v.c_str()); tVer = millis(); }
  }
  http.end();
  Serial.printf("Version en GitHub: %s\n", verGit[0] ? verGit : "?");
}

void barraOta(int hecho, int total) {
  if (total <= 0) return;
  int w = (long)hecho * 400 / total;
  tft.fillRect(40, 190, w, 20, VERDE);
}

void actualizaFirmware() {
  modo = ESTADO;
  tft.fillScreen(TFT_BLACK); titulo("ACTUALIZANDO", verGit);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(NAR);
  tft.drawString("Bajando la version nueva...", ANCHO / 2, 100, 4);
  tft.setTextColor(ROJO); tft.drawString("NO LO APAGUES", ANCHO / 2, 145, 4);
  tft.drawRect(38, 188, 404, 24, NAR_OSC);
  liberaSprite();
  if (!conectaWifi()) { strcpy(ultimoError, "sin WiFi"); apagaWifi(); pantallaEstado(); return; }
  WiFiClientSecure cli; cli.setInsecure();
  httpUpdate.rebootOnUpdate(true);                 // si sale bien, se reinicia solo con la version nueva
  httpUpdate.onProgress(barraOta);
  t_httpUpdate_return r = httpUpdate.update(cli, URL_BIN);
  apagaWifi();
  if (r != HTTP_UPDATE_OK) {                       // sigue con la version de antes
    snprintf(ultimoError, sizeof ultimoError, "OTA: %s", httpUpdate.getLastErrorString().c_str());
    tft.setTextColor(ROJO); tft.drawString(ultimoError, ANCHO / 2, 240, 2);
    delay(4000); pantallaEstado();
  }
}

// ---------- pantalla ESTADO (toque largo en el titulo) ----------
void filaEstado(int k, const char *nom, const char *val, uint16_t c) {
  int y = 36 + k * 22;
  tft.setTextDatum(TL_DATUM); tft.setTextColor(NAR_MED); tft.drawString(nom, 8, y, 2);
  tft.setTextColor(c); tft.drawString(val, 150, y, 2);
}

void pantallaEstado() {
  modo = ESTADO; tFicha = millis();
  tft.fillScreen(TFT_BLACK); titulo("ESTADO");
  char t[80], h[8], d[24];
  snprintf(t, sizeof t, "%s   (GitHub: %s)", VERSION, verGit[0] ? verGit : "sin mirar");
  filaEstado(0, "Version", t, hayNueva() ? VERDE : BLANCO);
  const char *cal = rssiUlt >= -60 ? "buena" : rssiUlt >= -72 ? "normal" : "flojita";
  if (rssiUlt) snprintf(t, sizeof t, "%s  %d dBm (%s)", wifiNom, rssiUlt, cal); else snprintf(t, sizeof t, "%s  (sin conectar aun)", wifiNom);
  filaEstado(1, "WiFi", t, rssiUlt && rssiUlt < -72 ? AMAR : BLANCO);
  if (tUltOk) { horaLocal(h, sizeof h, tUltOk); duracion(d, sizeof d, ahora() - tUltOk); snprintf(t, sizeof t, "%s (hace %s)", h, d); }
  else strcpy(t, "ninguna aun");
  filaEstado(2, "Ultima descarga", t, BLANCO);
  snprintf(t, sizeof t, "%u buenas, %u fallos", descOk, descFallos);
  filaEstado(3, "Descargas", t, descFallos ? AMAR : BLANCO);
  filaEstado(4, "Ultimo error", ultimoError[0] ? ultimoError : "ninguno", ultimoError[0] ? ROJO : VERDE);
  if (datos.t) { haceTexto(d, sizeof d, datos.t); snprintf(t, sizeof t, "%d sistemas (caben %d), escucha %s", datos.n, datos.cap, d); }
  else strcpy(t, "sin datos");
  filaEstado(5, "Datos", t, BLANCO);
  if (cacheT) { haceTexto(d, sizeof d, cacheT); snprintf(t, sizeof t, "guardados (%s)%s", d, desdeCache ? ", arranco con ellos" : ""); }
  else strcpy(t, fsOk ? "aun no guardados" : "memoria con fallo");
  filaEstado(6, "Datos guardados", t, fsOk ? BLANCO : ROJO);
  snprintf(t, sizeof t, "%u KB libres (bloque mayor %u KB)", ESP.getFreeHeap() / 1024, ESP.getMaxAllocHeap() / 1024);
  filaEstado(7, "RAM", t, ESP.getMaxAllocHeap() < 40000 ? AMAR : BLANCO);
  duracion(d, sizeof d, millis() / 1000);
  filaEstado(8, "Encendido", d, BLANCO);
  if (hayNueva()) { snprintf(t, sizeof t, "ACTUALIZAR %s", verGit); boton(LSIG_X, VOLVER_Y, 250, VOLVER_H, t, VERDE, 2); }
  dibujaBotones(ESTADO);
}
