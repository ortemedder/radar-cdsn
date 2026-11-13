// red.h - WiFi (solo encendido mientras descarga), descarga de faccion.json y refresco
#pragma once

bool conectaWifi() {
  if (WiFi.status() == WL_CONNECTED) { iconoWifi(2); return true; }
  iconoWifi(1);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) delay(250);
  bool ok = WiFi.status() == WL_CONNECTED;
  iconoWifi(ok ? 2 : 0);
  return ok;
}

void apagaWifi() { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); iconoWifi(0); }   // ahorra consumo

void hoyUTC(char *d) {
  long a = ahora(); if (!a) { strcpy(d, "0000-00-00"); return; }
  time_t t = a; struct tm g; gmtime_r(&t, &g); strftime(d, 11, "%Y-%m-%d", &g);
}

bool descarga() {
  liberaSprite();                                  // deja sitio para el JSON (se recrea al repintar)
  Serial.printf("RAM libre antes de descargar: %u (bloque mayor %u)\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  if (!conectaWifi()) { strcpy(ultimoError, "sin WiFi"); apagaWifi(); return false; }
  WiFiClientSecure cli; cli.setInsecure();
  HTTPClient http; http.useHTTP10(true); http.setTimeout(20000);
  if (!http.begin(cli, URL)) { strcpy(ultimoError, "URL"); apagaWifi(); return false; }
  int cod = http.GET();
  if (cod != 200) { snprintf(ultimoError, sizeof ultimoError, "HTTP %d", cod); http.end(); apagaWifi(); return false; }
  char hoy[11]; hoyUTC(hoy);
  WiFiClient &flujo = http.getStream();
  flujo.setTimeout(10000);                         // 10 s sin datos = corte
  bool ok = cargaDatos(datos, flujo, hoy, ultimoError, sizeof ultimoError);
  http.end();
  apagaWifi();                                   // descarga hecha: WiFi apagado hasta la proxima
  if (ok) ultimoError[0] = 0;
  Serial.printf("Descarga %s: %d sistemas, %d alertas, RAM libre %u\n", ok ? "OK" : ultimoError, datos.n, datos.alertas, ESP.getFreeHeap());
  return ok;
}

void actualiza(bool primera) {
  if (modo == MAPA && !primera) titulo("RADAR CDSN", "actualizando...");
  bool ok = descarga();
  tDescarga = ok ? millis() : millis() - CADA_MS + REINTENTO_MS;   // si falla, otra vez en 1 min
  if (ok && !hayDatos) { hayDatos = true; vistaPorDefecto(); }
  int recientes = ok ? marcaNuevas() : 0;
  if (ok) ordenaAlertas();
  if (recientes && modo != APAGADA) destello();
  if (!hayDatos) {
    tft.fillScreen(TFT_BLACK); titulo("RADAR CDSN");
    tft.setTextColor(ROJO); tft.setTextDatum(MC_DATUM);
    tft.drawString(ultimoError, ANCHO / 2, 150, 4);
    tft.setTextColor(NAR_MED); tft.drawString("reintento en 1 min", ANCHO / 2, 190, 2);
    return;
  }
  if (modo == MAPA) pantallaMapa();
  else if (modo == FICHA) pantallaFicha(fichaSis);
  else if (modo == LISTA) pantallaLista();
  if (!ok && modo == MAPA) titulo("RADAR CDSN", ultimoError);
}
