// ficha.h - ficha de un sistema y lista de controlados
#pragma once

void fichaTick(const Sis &s, int x, int y) {      // dos lineas: ultimo tick y proximo
  char t[64], h[8], d[16];
  tft.setTextDatum(TL_DATUM);
  if (!s.tickUlt && !s.tickProx) { tft.setTextColor(NAR_MED); tft.drawString("tick: aprendiendo...", x, y, 2); return; }
  if (s.tickUlt) {
    horaLocal(h, sizeof h, s.tickUlt);
    if (s.tickMarg >= 0) snprintf(t, sizeof t, "ultimo tick %s (+-%d min)", h, s.tickMarg);
    else snprintf(t, sizeof t, "ultimo tick %s", h);
    tft.setTextColor(NAR_MED); tft.drawString(t, x, y, 2);
  }
  long p = proxTick(s);
  if (p) {
    horaLocal(h, sizeof h, p); duracion(d, sizeof d, max(0L, p - ahora()));
    if (s.tickDisp >= 0) snprintf(t, sizeof t, "proximo ~%s, en %s (+-%d)", h, d, s.tickDisp);
    else snprintf(t, sizeof t, "proximo ~%s, en %s", h, d);
    tft.setTextColor(ROSA); tft.drawString(t, x, y + 17, 2);
  } else { tft.setTextColor(NAR_OSC); tft.drawString("proximo: faltan datos", x, y + 17, 2); }
}

// Grafica de influencia: escalones (la influencia solo cambia en el tick), del primer punto a ahora
void dibujaGrafica(const Sis &s) {
  const int x0 = GRAF_X + 34, y0 = GRAF_Y + 4, w = GRAF_W - 40, h = GRAF_H - 18;   // zona del trazo
  char t[24], u[24];
  tft.setTextDatum(TL_DATUM);
  if (s.nh < 2) { tft.setTextColor(NAR_MED); tft.drawString("historico: aprendiendo... (hace falta otro tick)", GRAF_X, GRAF_Y + 30, 2); return; }
  int vmin = s.hv[0], vmax = s.hv[0];
  for (int i = 1; i < s.nh; i++) { vmin = min(vmin, (int)s.hv[i]); vmax = max(vmax, (int)s.hv[i]); }
  int medio = (vmin + vmax) / 2;
  if (vmax - vmin < 200) { vmin = medio - 100; vmax = medio + 100; }  // al menos 2 puntos de escala
  vmin -= 50; vmax += 50;
  long t0 = s.ht[0], t1 = max((long)s.ht[s.nh - 1] + 3600, ahora());
  auto X = [&](long tt) { return x0 + (int)((float)(tt - t0) / (t1 - t0) * w); };
  auto Y = [&](int v) { return y0 + h - (int)((float)(v - vmin) / (vmax - vmin) * h); };
  tft.drawRect(x0, y0, w + 1, h + 1, NAR_OSC);                          // marco y linea media
  for (int x = x0; x < x0 + w; x += 6) tft.drawPixel(x, Y((vmin + vmax) / 2), NAR_OSC);
  coma(t, sizeof t, vmax / 100.0f); tft.setTextColor(NAR_MED); tft.setTextDatum(TR_DATUM); tft.drawString(t, x0 - 3, y0, 1);
  coma(t, sizeof t, vmin / 100.0f); tft.setTextDatum(BR_DATUM); tft.drawString(t, x0 - 3, y0 + h, 1);
  for (int i = 0; i < s.nh; i++) {                                       // escalones
    int xa = X(s.ht[i]), ya = Y(s.hv[i]);
    int xb = i + 1 < s.nh ? X(s.ht[i + 1]) : x0 + w;
    tft.drawFastHLine(xa, ya, max(1, xb - xa), NAR); tft.drawFastHLine(xa, ya + 1, max(1, xb - xa), NAR);
    if (i + 1 < s.nh) {
      int yb = Y(s.hv[i + 1]);
      tft.drawFastVLine(xb, min(ya, yb), abs(yb - ya) + 2, s.hv[i + 1] >= s.hv[i] ? VERDE : ROJO);
    }
    tft.fillCircle(xa, ya, 2, NAR);
  }
  struct tm a, b; time_t ta = t0, tb = t1; localtime_r(&ta, &a); localtime_r(&tb, &b);   // fechas abajo
  strftime(t, sizeof t, "%d/%m", &a); strftime(u, sizeof u, "%d/%m", &b);
  tft.setTextDatum(TL_DATUM); tft.drawString(t, x0, y0 + h + 4, 1);
  tft.setTextDatum(TR_DATUM); tft.drawString(u, x0 + w, y0 + h + 4, 1);
  snprintf(t, sizeof t, "%d cambios", s.nh - 1);
  tft.setTextDatum(TC_DATUM); tft.drawString(t, x0 + w / 2, y0 + h + 4, 1);
}

void pantallaFicha(int i) {
  modo = FICHA; fichaSis = i; tFicha = millis();
  vista(i);
  Sis &s = datos.s[i];
  char t[96], u[64], h[24];
  tft.fillScreen(TFT_BLACK);
  char n[40]; strncpy(n, s.nom, sizeof n - 1); n[sizeof n - 1] = 0;
  for (char *p = n; *p; p++) *p = toupper(*p);
  haceTexto(h, sizeof h, datos.t); snprintf(t, sizeof t, "act. %s", h);
  corta(u, n, 330, 4); titulo(u, t);

  // controlado / lider
  tft.setTextDatum(TL_DATUM);
  if (s.ctrl) { tft.setTextColor(VERDE); tft.drawString("CONTROLADO - Caballeros de Sion", 6, 33, 2); }
  else {
    tft.setTextColor(NAR_MED);
    snprintf(t, sizeof t, "NO CONTROLADO - manda %s", s.nf ? s.f[0].n : "?");
    corta(u, t, 468, 2); tft.drawString(u, 6, 33, 2);
  }
  // influencia grande: 53,2 %
  snprintf(t, sizeof t, "%d", (int)s.inf);
  int x = 6; tft.setTextColor(NAR);
  tft.drawString(t, x, 54, 6); x += tft.textWidth(t, 6);
  tft.drawString(",", x, 76, 4); x += tft.textWidth(",", 4) + 2;
  snprintf(t, sizeof t, "%d", (int)roundf((s.inf - (int)s.inf) * 10) % 10);
  tft.drawString(t, x, 54, 6); x += tft.textWidth(t, 6) + 6;
  tft.drawString("%", x, 76, 4);
  // a la derecha: cambio desde ayer y tick
  if (!isnan(s.prev)) {
    float d = s.inf - s.prev; coma(u, sizeof u, d, "%+.1f");
    snprintf(t, sizeof t, "%s desde ayer", u);
    tft.setTextColor(d >= 0 ? VERDE : ROJO); tft.drawString(t, 200, 54, 2);
  }
  fichaTick(s, 200, 72);
  // aviso
  if (s.nivel) {
    uint16_t cc = colorNivel(s.nivel);
    tft.drawRoundRect(4, 108, 472, 42, 6, cc); tft.drawRoundRect(5, 109, 470, 40, 5, cc);
    tft.setTextColor(cc); tft.drawString(s.alerta, 12, 111, 2);
    tft.setTextColor(BLANCO); corta(u, s.aviso, 456, 2); tft.drawString(u, 12, 129, 2);
  } else {
    tft.drawRoundRect(4, 108, 472, 42, 6, NAR_OSC);
    tft.setTextColor(VERDE); tft.drawString("SIN AVISOS", 12, 120, 2);
  }
  if (fichaGrafica) dibujaGrafica(s);
  else {
  // reparto de facciones
    int filas = min((int)s.nf, 5); float otras = 0;
    for (int k = 5; k < s.nf; k++) otras += s.f[k].inf;
    for (int k = 0; k < filas + (otras > 0); k++) {
      bool esOtras = k == filas; int y = 158 + k * 18;
      const char *nom = esOtras ? "Otras" : s.f[k].n;
      float v = esOtras ? otras : s.f[k].inf;
      bool mia = !esOtras && !strcmp(nom, "Caballeros de Sion");
      tft.setTextColor(mia ? BLANCO : NAR_MED); corta(u, nom, 186, 2); tft.drawString(u, 6, y, 2);
      tft.fillRect(200, y + 3, 200, 10, NAR_OSC);
      tft.fillRect(200, y + 3, constrain(v, 0, 100) * 2, 10, mia ? NAR : NAR_MED);
      coma(u, sizeof u, v); strcat(u, " %");
      tft.setTextDatum(TR_DATUM); tft.setTextColor(mia ? NAR : NAR_MED); tft.drawString(u, 476, y, 2);
      tft.setTextDatum(TL_DATUM);
    }
  }
  // estados y conflicto
  snprintf(t, sizeof t, "ESTADOS: %s", s.act[0] ? s.act : "ninguno");
  if (s.pend[0]) { strncat(t, " - pend.: ", sizeof t - strlen(t) - 1); strncat(t, s.pend, sizeof t - strlen(t) - 1); }
  if (s.rec[0]) { strncat(t, " - recup.: ", sizeof t - strlen(t) - 1); strncat(t, s.rec, sizeof t - strlen(t) - 1); }
  tft.setTextColor(NAR_MED); corta(u, t, 468, 2); tft.drawString(u, 6, 250, 2);
  if (s.conf[0]) {                                // conflicto: texto + cuadritos ganados/perdidos
    tft.setTextColor(s.confActivo ? NAR : NAR_MED); corta(u, s.conf, 300, 2); tft.drawString(u, 6, 268, 2);
    int x = 316;
    for (int k = 0; k < s.ganados; k++, x += 13) tft.fillRect(x, 270, 10, 10, VERDE);
    for (int k = 0; k < s.perdidos; k++, x += 13) tft.fillRect(x, 270, 10, 10, ROJO);
    if (!s.ganados && !s.perdidos) { tft.drawRect(x, 270, 10, 10, NAR_OSC); x += 13; }
    snprintf(t, sizeof t, "%d-%d", s.ganados, s.perdidos);
    tft.setTextColor(BLANCO); tft.drawString(t, x + 4, 268, 2);
  }
  // trafico
  haceTexto(h, sizeof h, s.t);
  snprintf(t, sizeof t, "TRAFICO HOY: %d naves - dato %s", s.trafHoy, h);
  tft.setTextColor(AZUL); corta(u, t, GRAF_BX - 12, 2); tft.drawString(u, 6, 296, 2);
  dibujaBotones(FICHA);
  boton(GRAF_BX, VOLVER_Y, GRAF_BW, VOLVER_H, fichaGrafica ? "FACCIONES" : "GRAFICA", NAR, 2);
}

void pantallaLista() {
  modo = LISTA; tFicha = millis();
  nLista = 0;
  for (int i = 0; i < datos.n; i++) if (datos.s[i].ctrl) ordenLista[nLista++] = i;
  for (int a = 1; a < nLista; a++)                 // de menor a mayor influencia
    for (int b = a; b > 0 && datos.s[ordenLista[b]].inf < datos.s[ordenLista[b - 1]].inf; b--) {
      int t = ordenLista[b]; ordenLista[b] = ordenLista[b - 1]; ordenLista[b - 1] = t; }
  int pags = max(1, (nLista + FILAS - 1) / FILAS);
  if (pagLista >= pags) pagLista = 0;
  tft.fillScreen(TFT_BLACK);
  char t[64], u[64];
  snprintf(t, sizeof t, "CONTROLADOS (%d)", nLista);
  titulo(t, "de menor a mayor influencia");
  tft.setTextColor(NAR_OSC); tft.setTextDatum(TL_DATUM);
  tft.drawString("SISTEMA", 6, 32, 1); tft.drawString("VENTAJA", 330, 32, 1);
  tft.setTextDatum(TR_DATUM); tft.drawString("INFL.", 476, 32, 1);
  for (int k = 0; k < FILAS; k++) {
    int j = pagLista * FILAS + k; if (j >= nLista) break;
    Sis &s = datos.s[ordenLista[j]]; int y = LISTA_Y + k * LISTA_PASO;
    uint16_t c = s.nivel ? colorNivel(s.nivel) : BLANCO;
    tft.setTextDatum(TL_DATUM); tft.setTextColor(c); corta(u, s.nom, 230, 2); tft.drawString(u, 6, y, 2);
    if (s.nivel) { corta(u, s.alerta, 80, 1); tft.drawString(u, 244, y + 4, 1); }
    float margen = s.nf > 1 ? s.f[0].inf - s.f[1].inf : s.inf;
    coma(u, sizeof u, margen, "+%.1f");
    tft.setTextColor(margen < 5 ? ROJO : margen < 15 ? AMAR : VERDE); tft.drawString(u, 330, y, 2);
    coma(u, sizeof u, s.inf); strcat(u, " %");
    tft.setTextDatum(TR_DATUM); tft.setTextColor(colorInf(s.inf)); tft.drawString(u, 476, y, 2);
    tft.drawFastHLine(4, y + 17, 472, LINEA);
  }
  if (pags > 1) {
    snprintf(t, sizeof t, "%d/%d  siguientes >", pagLista + 1, pags);
    boton(LSIG_X, VOLVER_Y, LSIG_W, VOLVER_H, "", NAR);
    tft.setTextColor(NAR); tft.setTextDatum(MC_DATUM); tft.drawString(t, LSIG_X + LSIG_W / 2, VOLVER_Y + VOLVER_H / 2, 2);
  }
  dibujaBotones(LISTA);
}

// toque en una fila de la lista: abre su ficha
bool tocaLista(int x, int y) {
  int k = (y - LISTA_Y) / LISTA_PASO, j = pagLista * FILAS + k;
  if (y < LISTA_Y || k >= FILAS || j >= nLista) return false;
  fichaDesdeLista = true; pantallaFicha(ordenLista[j]); return true;
}
