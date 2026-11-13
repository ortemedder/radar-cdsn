// panel.h - titulo, icono WiFi, alertas (nuevas y orden) y panel derecho del mapa
#pragma once

// 0 = apagado (gris con raya roja), 1 = conectando (amarillo), 2 = conectado (verde)
void iconoWifi(int e) {
  estadoWifi = e;
  if (modo != MAPA) return;
  tft.fillRect(WIFI_X - 2, WIFI_Y - 2, 26, 22, TFT_BLACK);
  uint16_t c = e == 2 ? VERDE : e == 1 ? AMAR : NAR_OSC;
  for (int i = 0; i < 4; i++) { int h = 5 + i * 4; tft.fillRect(WIFI_X + i * 6, WIFI_Y + 17 - h, 4, h, c); }
  if (e == 0) { tft.drawLine(WIFI_X - 1, WIFI_Y + 17, WIFI_X + 22, WIFI_Y, ROJO); tft.drawLine(WIFI_X, WIFI_Y + 17, WIFI_X + 23, WIFI_Y, ROJO); }
}

void titulo(const char *izq, const char *extra) {
  bool esMapa = !strcmp(izq, "RADAR CDSN");
  tft.fillRect(0, 0, ANCHO, TIT_H, TFT_BLACK);
  tft.setTextColor(esMapa ? AZUL : NAR); tft.setTextDatum(TL_DATUM);
  tft.drawString(izq, 6, 1, 4);
  char t[48], h[8], hace[24];
  horaLocal(h, sizeof h, ahora());
  haceTexto(hace, sizeof hace, datos.t);
  if (extra) snprintf(t, sizeof t, "%s", extra);
  else snprintf(t, sizeof t, "%s%s  act. %s", pausa ? "PAUSA  " : "", h, hace);
  bool viejo = !extra && ahora() && datos.t && ahora() - datos.t > 3600;   // escucha parado > 1 h
  tft.setTextColor(viejo ? ROJO : ROSA); tft.setTextDatum(TR_DATUM);
  tft.drawString(t, ANCHO - 4, 6, 2);
  tft.drawFastHLine(0, TIT_H, ANCHO, NAR_OSC);
  if (esMapa) iconoWifi(estadoWifi);
}

// ---------- alertas nuevas ----------
// Tras cada descarga: marca las alertas que no estaban antes. Devuelve cuantas nuevas hay.
int marcaNuevas() {
  int recientes = 0;
  uint32_t quedan[MAX_SIS]; int nq = 0;
  for (int i = 0; i < datos.n; i++) {
    Sis &s = datos.s[i];
    if (!s.nivel) continue;
    uint32_t h = hashNombre(s.nom);
    bool nueva = nAntes >= 0 && !estaEn(h, alertasAntes, nAntes);   // la primera descarga no avisa
    if (nueva) recientes++;
    if (nueva || estaEn(h, nuevas, nNuevas)) { s.nueva = true; quedan[nq++] = h; }
  }
  memcpy(nuevas, quedan, nq * sizeof(uint32_t)); nNuevas = nq;    // las que dejaron de ser alerta se olvidan
  nAntes = 0;
  for (int i = 0; i < datos.n; i++) if (datos.s[i].nivel) alertasAntes[nAntes++] = hashNombre(datos.s[i].nom);
  return recientes;
}

void ordenaAlertas() {            // nuevas primero, luego graves, luego avisos
  int k = 0;
  for (int nu = 1; nu >= 0; nu--)
    for (int nv = 2; nv >= 1; nv--) for (int i = 0; i < datos.n; i++)
      if (datos.s[i].nivel == nv && datos.s[i].nueva == nu) orden[k++] = i;
  for (; k < MAX_SIS; k++) orden[k] = -1;
  if (pagAlerta * AL_N >= datos.alertas) pagAlerta = 0;
}

void vista(int i) {               // al abrir su ficha deja de ser "nueva"
  if (!datos.s[i].nueva) return;
  datos.s[i].nueva = false;
  uint32_t h = hashNombre(datos.s[i].nom);
  for (int k = 0; k < nNuevas; k++) if (nuevas[k] == h) { nuevas[k] = nuevas[--nNuevas]; break; }
  ordenaAlertas();
}

void destello() { for (int i = 0; i < 3; i++) { tft.invertDisplay(true); delay(150); tft.invertDisplay(false); delay(150); } }

// ---------- panel derecho ----------
void panelTick() {               // proximo tick: el del sistema mas cercano si es fiable; si no, el de la galaxia
  const int FIABLE = 180;        // +- minutos maximos para fiarse de la prevision de un sistema
  int mejor = -1; long pm = 0;
  for (int i = 0; i < datos.n; i++) {
    const Sis &s = datos.s[i];
    if (s.tickDisp < 0 || s.tickDisp > FIABLE) continue;
    long p = proxTick(s);
    if (p && (!pm || p < pm)) { pm = p; mejor = i; }
  }
  bool global = false;
  if (mejor < 0 && datos.tickGlobal && ahora()) {          // respaldo: tick global + 24 h
    pm = datos.tickGlobal + 86400;
    while (pm < ahora() - 1800) pm += 86400;
    global = true;
  }
  char t[48], h[8], d[16];
  tft.fillRect(PX, TICK_Y, ANCHO - PX, BOT_Y - TICK_Y - 1, TFT_BLACK);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(VERDE); tft.drawString("PROX. TICK", PX + 2, TICK_Y, 2);
  tft.setTextDatum(TR_DATUM);
  if (!pm) { tft.setTextColor(NAR_MED); tft.drawString("aprendiendo...", ANCHO - 4, TICK_Y, 2); return; }
  horaLocal(h, sizeof h, pm); snprintf(t, sizeof t, "~%s", h);
  tft.setTextColor(ROSA); tft.drawString(t, ANCHO - 4, TICK_Y, 2);
  long falta = pm - ahora();
  const char *quien = global ? "galaxia" : datos.s[mejor].nom;
  if (falta > 0) { duracion(d, sizeof d, falta); snprintf(t, sizeof t, "%s - en %s", quien, d); }
  else snprintf(t, sizeof t, "%s - ya deberia", quien);
  char u[64]; corta(u, t, PW, 1);
  tft.setTextDatum(TL_DATUM); tft.setTextColor(NAR_MED); tft.drawString(u, PX + 2, TICK_Y + 17, 1);
}

void panel() {
  tft.fillRect(PX, TIT_H + 3, ANCHO - PX, ALTO - TIT_H - 3, TFT_BLACK);
  char t[64];
  tft.setTextDatum(TL_DATUM); tft.setTextColor(VERDE);
  tft.drawString("CONTROLADOS >", PX + 2, CTRL_Y, 2);
  snprintf(t, sizeof t, "%d de %d", datos.controlados, datos.n);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(BLANCO);
  tft.drawString(t, ANCHO - 4, CTRL_Y + 16, 4);

  tft.setTextDatum(TL_DATUM); tft.setTextColor(VERDE);
  snprintf(t, sizeof t, "ALERTAS (%d)", datos.alertas);
  tft.drawString(t, PX + 2, AL_TIT_Y, 2);
  for (int k = 0; k < AL_N; k++) {
    int i = orden[pagAlerta * AL_N + k]; if (i < 0) break;
    Sis &s = datos.s[i]; int y = AL_Y + k * AL_PASO;
    uint16_t cc = colorNivel(s.nivel);
    tft.drawRoundRect(PX, y, PW, AL_H, 5, cc);
    tft.setTextColor(cc);
    corta(t, s.alerta, s.nueva ? 100 : PW - 10, 2); tft.drawString(t, PX + 6, y + 2, 2);
    if (s.nueva) {                                 // etiqueta NUEVA
      tft.fillRoundRect(ANCHO - 56, y + 3, 50, 14, 3, cc);
      tft.setTextColor(TFT_BLACK); tft.setTextDatum(MC_DATUM); tft.drawString("NUEVA", ANCHO - 31, y + 10, 1);
      tft.setTextDatum(TL_DATUM);
    }
    tft.setTextColor(BLANCO); corta(t, s.nom, PW - 10, 2); tft.drawString(t, PX + 6, y + 18, 2);
  }
  if (datos.alertas > AL_N) {                      // boton para pasar de pagina
    int pags = (datos.alertas + AL_N - 1) / AL_N;
    tft.drawRoundRect(PX, SIG_Y, PW, SIG_H, 4, NAR_OSC);
    snprintf(t, sizeof t, "%d/%d   siguientes >", pagAlerta + 1, pags);
    tft.setTextColor(NAR); tft.setTextDatum(MC_DATUM); tft.drawString(t, PX + PW / 2, SIG_Y + SIG_H / 2, 2);
  }
  tft.setTextDatum(TL_DATUM); tft.setTextColor(VERDE); tft.drawString("TRAFICO HOY", PX + 2, TRAF_Y + 4, 2);
  snprintf(t, sizeof t, "%d", datos.trafHoy);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(AZUL); tft.drawString(t, ANCHO - 4, TRAF_Y - 2, 4);
  panelTick();
  dibujaBotones(MAPA);
}

void pantallaMapa() {
  modo = MAPA;
  tft.fillScreen(TFT_BLACK);
  titulo("RADAR CDSN");
  calculaPantalla();
  dibujaMapaEntero();
  panel();
}
