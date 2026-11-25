// visitas.h - pagina VISITAS (pilotos por sistema y posibles ataques), pagina RESUMEN y salvapantallas
#pragma once

// ---------- VISITAS ----------
// Orden: posible ataque, actividad inusual y despues por pilotos de hoy (y de ayer).
// Solo salen los sistemas con alguna visita en los ultimos DIAS_LISTA dias, y siempre los que tienen pico.
static int rangoPico(uint8_t a) { return a == 2 ? 3 : a == 1 ? 2 : a == 3 ? 1 : 0; }   // ataque, inusual, a favor
static bool antesVis(const Sis &a, const Sis &b) {
  if (a.ataque != b.ataque) return rangoPico(a.ataque) > rangoPico(b.ataque);
  if (a.traf[0] != b.traf[0]) return a.traf[0] > b.traf[0];
  return a.traf[1] > b.traf[1];
}

void pantallaVisitas() {
  modo = VISITAS; tFicha = millis();
  nVis = 0;
  for (int i = 0; i < datos.n; i++) {
    const Sis &s = datos.s[i]; bool alguna = s.ataque > 0;
    for (int k = 0; k < DIAS_LISTA && !alguna; k++) alguna = s.traf[k] > 0;
    if (alguna) ordenVis[nVis++] = i;
  }
  for (int a = 1; a < nVis; a++)
    for (int b = a; b > 0 && antesVis(datos.s[ordenVis[b]], datos.s[ordenVis[b - 1]]); b--) {
      int t = ordenVis[b]; ordenVis[b] = ordenVis[b - 1]; ordenVis[b - 1] = t; }
  int pags = max(1, (nVis + VFILAS - 1) / VFILAS);
  if (pagVis >= pags) pagVis = 0;

  tft.fillScreen(TFT_BLACK);
  titulo("VISITAS", "pilotos por sistema");
  char t[64], u[64];
  // resumen de arriba
  tft.setTextDatum(TL_DATUM);
  if (!datos.ataques && !datos.inusuales && !datos.favor) { tft.setTextColor(VERDE); tft.drawString("Sin picos de visitas", 6, 32, 2); }
  else {
    int x = 6;
    if (datos.ataques) { snprintf(t, sizeof t, "POSIBLE ATAQUE: %d", datos.ataques); tft.setTextColor(ROJO); tft.drawString(t, x, 32, 2); x += tft.textWidth(t, 2) + 16; }
    if (datos.inusuales) { snprintf(t, sizeof t, "INUSUAL: %d", datos.inusuales); tft.setTextColor(NAR_AV); tft.drawString(t, x, 32, 2); x += tft.textWidth(t, 2) + 16; }
    if (datos.favor) { snprintf(t, sizeof t, "A FAVOR: %d", datos.favor); tft.setTextColor(VERDE); tft.drawString(t, x, 32, 2); }
  }
  snprintf(t, sizeof t, "%d hoy", datos.trafHoy);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(AZUL); tft.drawString(t, 476, 32, 2);
  // cabecera de la tabla
  tft.setTextColor(NAR_OSC); tft.setTextDatum(TL_DATUM);
  tft.drawString("SISTEMA", 6, 54, 1); tft.drawString("AVISO", 214, 54, 1);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("HOY", 350, 54, 1); tft.drawString("AYER", 405, 54, 1); tft.drawString("MEDIA 7D", 476, 54, 1);
  if (!nVis) { tft.setTextDatum(MC_DATUM); tft.setTextColor(NAR_MED); tft.drawString("sin visitas en los ultimos dias", ANCHO / 2, 150, 2); }
  for (int k = 0; k < VFILAS; k++) {
    int j = pagVis * VFILAS + k; if (j >= nVis) break;
    const Sis &s = datos.s[ordenVis[j]]; int y = VLIS_Y + k * LISTA_PASO;
    uint16_t c = s.ataque == 2 ? ROJO : s.ataque == 1 ? NAR_AV : s.ataque == 3 ? VERDE : s.ctrl ? BLANCO : NAR_MED;
    tft.setTextDatum(TL_DATUM); tft.setTextColor(c); corta(u, s.nom, 200, 2); tft.drawString(u, 6, y, 2);
    if (s.ataque) tft.drawString(s.ataque == 2 ? "ATAQUE?" : s.ataque == 1 ? "INUSUAL" : "A FAVOR", 214, y + 4, 1);
    tft.setTextDatum(TR_DATUM);
    snprintf(t, sizeof t, "%d", s.traf[0]);
    tft.setTextColor(s.ataque && !s.picoAyer ? c : AZUL); tft.drawString(t, 350, y, 2);
    snprintf(t, sizeof t, "%d", s.traf[1]);
    tft.setTextColor(s.ataque && s.picoAyer ? c : NAR_MED); tft.drawString(t, 405, y, 2);
    coma(t, sizeof t, s.media7);
    tft.setTextColor(NAR_MED); tft.drawString(t, 476, y, 2);
    tft.drawFastHLine(4, y + 17, 472, LINEA);
  }
  if (pags > 1) {
    snprintf(t, sizeof t, "%d/%d  siguientes >", pagVis + 1, pags);
    boton(LSIG_X, VOLVER_Y, LSIG_W, VOLVER_H, "", NAR);
    tft.setTextColor(NAR); tft.setTextDatum(MC_DATUM); tft.drawString(t, LSIG_X + LSIG_W / 2, VOLVER_Y + VOLVER_H / 2, 2);
  }
  tft.setTextColor(NAR_OSC); tft.setTextDatum(TL_DATUM);
  tft.drawString("ATAQUE? baja inf. / A FAVOR sube", LSIG_X + LSIG_W + 8, VOLVER_Y + 4, 1);
  tft.drawString("titulo: RESUMEN", LSIG_X + LSIG_W + 8, VOLVER_Y + 18, 1);
  dibujaBotones(VISITAS);
}

bool tocaVisitas(int x, int y) {                   // toque en una fila: abre su ficha
  int k = (y - VLIS_Y) / LISTA_PASO, j = pagVis * VFILAS + k;
  if (y < VLIS_Y || k >= VFILAS || j >= nVis) return false;
  fichaVuelve = VISITAS; pantallaFicha(ordenVis[j]); return true;
}

// ---------- RESUMEN ----------
// Hora grande, proximo tick, cifras y sistemas en peligro. Se mueve unos pixeles cada vez que se abre.
void cajaResumen(int x, int y, const char *tit, const char *val, uint16_t c) {
  tft.drawRoundRect(x, y, 148, 58, 6, NAR_OSC);
  tft.setTextDatum(TC_DATUM); tft.setTextColor(VERDE); tft.drawString(tit, x + 74, y + 4, 2);
  tft.setTextColor(c); tft.drawString(val, x + 74, y + 24, 4);
}

void pantallaResumen(bool mover) {
  static int ox = 0, oy = 0;
  if (mover) { ox = (int)(esp_random() % 25) - 12; oy = (int)(esp_random() % 13) - 6; }
  modo = RESUMEN; tFicha = millis();
  tft.fillScreen(TFT_BLACK);
  titulo("RESUMEN");
  char t[96], u[64], h[8], d[16];
  // hora grande
  horaLocal(h, sizeof h, ahora());
  tft.setTextDatum(MC_DATUM); tft.setTextColor(NAR); tft.setTextSize(2);
  tft.drawString(h, ANCHO / 2 + ox, 88 + oy, 7); tft.setTextSize(1);
  // proximo tick
  long pm; int mejor; bool global;
  calculaTick(pm, mejor, global);
  if (pm) {
    horaLocal(h, sizeof h, pm);
    long falta = pm - ahora();
    if (falta > 0) { duracion(d, sizeof d, falta); snprintf(t, sizeof t, "PROX. TICK ~%s  (en %s)", h, d); }
    else snprintf(t, sizeof t, "PROX. TICK ~%s  (ya deberia)", h);
    tft.setTextColor(ROSA); tft.drawString(t, ANCHO / 2 + ox, 152 + oy, 4);
    tft.setTextColor(NAR_MED); corta(u, global ? "galaxia" : datos.s[mejor].nom, 440, 2);
    tft.drawString(u, ANCHO / 2 + ox, 176 + oy, 2);
  } else { tft.setTextColor(NAR_MED); tft.drawString("PROX. TICK: aprendiendo...", ANCHO / 2 + ox, 152 + oy, 4); }
  // cifras
  int graves = 0; for (int i = 0; i < datos.n; i++) graves += datos.s[i].tono == TN_GRAVE;
  snprintf(t, sizeof t, "%d de %d", datos.controlados + datos.ctrlFuera, datos.n + datos.sinSitio);   // v2.9: totales reales
  cajaResumen(14 + ox, 196 + oy, "CONTROLADOS", t, BLANCO);
  snprintf(t, sizeof t, "%d", datos.alertas);
  cajaResumen(166 + ox, 196 + oy, graves ? "ALERTAS (graves)" : "ALERTAS", t, graves ? ROJO : datos.alertas ? AMAR : VERDE);
  snprintf(t, sizeof t, "%d", datos.trafHoy);
  cajaResumen(318 + ox, 196 + oy, "PILOTOS HOY", t, AZUL);
  // en peligro por visitas
  t[0] = 0;
  for (int nv = 2; nv >= 1; nv--) for (int i = 0; i < datos.n; i++) if (datos.s[i].ataque == nv) {
    if (t[0]) strncat(t, ", ", sizeof t - strlen(t) - 1);
    strncat(t, datos.s[i].nom, sizeof t - strlen(t) - 1);
  }
  tft.setTextDatum(MC_DATUM);
  if (t[0]) {
    snprintf(u, sizeof u, "%s:", datos.ataques ? "POSIBLE ATAQUE" : "ACTIVIDAD INUSUAL");
    tft.setTextColor(datos.ataques ? ROJO : NAR_AV); tft.drawString(u, ANCHO / 2 + ox, 270 + oy, 2);
    corta(u, t, 460, 2); tft.setTextColor(BLANCO); tft.drawString(u, ANCHO / 2 + ox, 288 + oy, 2);
  } else { tft.setTextColor(VERDE); tft.drawString("Sin picos de visitas", ANCHO / 2 + ox, 278 + oy, 2); }
  if (datos.sinSitio) {                            // la faccion ya no cabe en la tabla
    snprintf(t, sizeof t, "En el radar %d de %d sistemas (los mas importantes)", datos.n, datos.n + datos.sinSitio);
    tft.setTextColor(NAR_MED); tft.drawString(t, ANCHO / 2, 310, 1);
  }
}

// ---------- ALERTA: pantalla completa de POSIBLE ATAQUE, DIA DECISIVO o SISTEMA EN DERROTA ----------
void marcoAlerta(bool fuerte) {                    // marco rojo que late
  uint16_t c = fuerte ? ROJO : tft.color565(110, 20, 15);
  for (int k = 0; k < 4; k++) tft.drawRect(k, k, ANCHO - 2 * k, ALTO - 2 * k, c);
}

void pantallaAlerta() {
  int i = buscaHash(alertaHash);
  if (i < 0 || !datos.s[i].alarma) i = sigAtaque(alertaHash);      // ya no esta: el siguiente que haya
  if (i < 0) { alertaFija = false; pantallaMapa(); return; }        // no queda ninguno
  alertaHash = hashNombre(datos.s[i].nom);
  modo = ALERTA; tFicha = millis();
  const Sis &s = datos.s[i];
  char t[96], u[64], h[16], d[16];
  tft.fillScreen(TFT_BLACK);
  marcoAlerta(true);
  uint16_t ROJO_OSC = tft.color565(150, 25, 20);
  tft.fillRect(10, 10, ANCHO - 20, 36, ROJO_OSC);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(BLANCO); tft.drawString(s.alerta, ANCHO / 2, 29, 4);
  if (s.icono) { int w = tft.textWidth(s.alerta, 4); (s.icono == IC_SOBRE ? sobre : espadas)(ANCHO / 2 - w / 2 - 24, 21, BLANCO); }
  // sistema
  char n[40]; strncpy(n, s.nom, sizeof n - 1); n[sizeof n - 1] = 0;
  for (char *p = n; *p; p++) *p = toupper(*p);
  corta(u, n, 456, 4); tft.drawString(u, ANCHO / 2, 70, 4);
  if (s.ctrl && s.nf > 1) {
    coma(h, sizeof h, s.inf - s.f[1].inf, "%+.1f");
    snprintf(t, sizeof t, "controlado - ventaja %s sobre %s", h, s.f[1].n);
  } else if (s.ctrl) snprintf(t, sizeof t, "controlado");
  else snprintf(t, sizeof t, "no controlado - manda %s", s.nf ? s.f[0].n : "?");
  tft.setTextColor(s.ctrl ? VERDE : NAR_MED); corta(u, t, 456, 2); tft.drawString(u, ANCHO / 2, 94, 2);
  // datos
  if (s.confActivo) {                              // conflicto: marcador, dia, que se juega y que hay en juego
    tft.setTextDatum(TL_DATUM); tft.setTextColor(BLANCO); corta(u, s.aviso, 432, 2); tft.drawString(u, 24, 120, 2);
    int x = 24;
    for (int k = 0; k < s.ganados; k++, x += 15) tft.fillRect(x, 148, 12, 12, VERDE);
    for (int k = 0; k < s.perdidos; k++, x += 15) tft.fillRect(x, 148, 12, 12, ROJO);
    if (s.juego[0]) { tft.setTextColor(AMAR_AV); corta(u, s.juego, 440 - (x - 24), 2); tft.drawString(u, x + 8, 146, 2); }
    if (s.enJuego[0]) { snprintf(t, sizeof t, "EN JUEGO: %s", s.enJuego); tft.setTextColor(NAR); corta(u, t, 432, 2); tft.drawString(u, 24, 172, 2); }
  } else {
  int v = s.traf[s.picoAyer ? 1 : 0];
  coma(h, sizeof h, s.media7);
  snprintf(t, sizeof t, "PILOTOS  %d %s  (media %s)  x%.1f", v, s.picoAyer ? "ayer" : "hoy", h, v / (s.media7 > 1 ? s.media7 : 1.0f));
  for (char *p = strrchr(t, 'x'); p && *p; p++) if (*p == '.') *p = ',';
  tft.setTextDatum(TL_DATUM); tft.setTextColor(AZUL); tft.drawString(t, 24, 120, 2);
  coma(h, sizeof h, s.inf); coma(d, sizeof d, s.inf - s.prev, "%+.1f");
  snprintf(t, sizeof t, "INFLUENCIA  %s %%  (%s desde ayer)", h, d);
  tft.setTextColor(ROJO); tft.drawString(t, 24, 146, 2);
  }
  long p = proxTick(s); bool global = false;
  if (!p) { int mejor; calculaTick(p, mejor, global); global = true; }
  if (p) {
    horaLocal(h, sizeof h, p); duracion(d, sizeof d, max(0L, p - ahora()));
    snprintf(t, sizeof t, "PROX. TICK  ~%s  en %s%s", h, d, global ? " (galaxia)" : "");
  } else snprintf(t, sizeof t, "PROX. TICK  aprendiendo...");
  tft.setTextColor(ROSA); tft.drawString(t, 24, s.confActivo && s.enJuego[0] ? 198 : 172, 2);
  // si no es nuestro: quien manda
  if (s.nf > 1 && !s.ctrl && !s.confActivo) {
    coma(h, sizeof h, s.f[0].inf); snprintf(t, sizeof t, "%s: %s %%", s.f[0].n, h);
    tft.setTextColor(NAR_MED); corta(u, t, 440, 2); tft.drawString(u, 24, 198, 2);
  }
  haceTexto(h, sizeof h, s.t);
  snprintf(t, sizeof t, "dato %s  -  alerta %d de %d", h,
           [&] { int k = 0; for (int j = 0; j <= i; j++) k += datos.s[j].alarma; return k; }(), datos.alarmas);
  tft.setTextColor(NAR_MED); tft.drawString(t, 24, 226, 2);
  if (!alertaFija) { tft.setTextColor(NAR_OSC); tft.drawString("toca para volver al mapa", 24, 246, 1); }
  // botones
  dibujaBotones(ALERTA);
  if (datos.alarmas > 1) boton(ALE_B2, ALE_BY, ALE_BW, ALE_BH, "SIGUIENTE", NAR, 2);
  tft.fillRoundRect(ALE_B3, ALE_BY, ALE_BW, ALE_BH, 5, ROJO_OSC);
  tft.setTextColor(BLANCO); tft.setTextDatum(MC_DATUM); tft.drawString("VISTO", ALE_B3 + ALE_BW / 2, ALE_BY + ALE_BH / 2 + 1, 2);
}

void tickAlerta(uint32_t m) {                      // latido del marco
  static uint32_t t = 0; static bool fuerte = true;
  if (m - t < 500) return;
  t = m; fuerte = !fuerte; marcoAlerta(fuerte);
}

// Tras cada descarga: si hay una alarma que no estaba antes (o ha cambiado, p. ej. de DERROTA a DIA DECISIVO),
// sale la alerta (enciende la pantalla si estaba en negro)
void revisaAtaques() {   // la primera vez (datos guardados o primera descarga) solo se apuntan: el radar arranca en el mapa
  static bool listos = false; bool primera = !listos; listos = true;
  uint32_t ahoraH[MAX_SIS]; int na = 0, nuevo = -1;
  for (int i = 0; i < datos.n; i++) if (datos.s[i].alarma) {
    uint32_t h = hashNombre(datos.s[i].nom) ^ (hashNombre(datos.s[i].alerta) * 31u); ahoraH[na++] = h;
    if (nuevo < 0 && !estaEn(h, ataqAntes, nAtaqAntes)) nuevo = i;
  }
  memcpy(ataqAntes, ahoraH, na * sizeof(uint32_t)); nAtaqAntes = na;
  if (nuevo < 0 || primera) return;
  if (modo == APAGADA) destello();
  alertaHash = hashNombre(datos.s[nuevo].nom); alertaFija = true;
  carrusel = false; tUltToque = millis();          // el carrusel espera a que se quite la alerta
  modo = ALERTA;
}

// ---------- salvapantallas: carrusel mapa / alerta / visitas / resumen ----------
// La pantalla de alerta solo entra en el bucle mientras haya alguna alarma.
void tickCarrusel(uint32_t m) {
  if (alertaFija) return;
  if (!carrusel) {
    if (modo == APAGADA || m - tUltToque < CARRUSEL_ESPERA) return;
    carrusel = true; tCarrusel = m;
    if (modo != MAPA) pantallaMapa();
    return;
  }
  uint32_t ale = datos.alarmas ? CAR_ALE_S : 0;
  uint32_t f = ((m - tCarrusel) / 1000) % (CAR_MAPA_S + ale + CAR_VIS_S + CAR_RES_S);
  Modo q = f < CAR_MAPA_S ? MAPA : f < CAR_MAPA_S + ale ? ALERTA : f < CAR_MAPA_S + ale + CAR_VIS_S ? VISITAS : RESUMEN;
  if (q == modo) return;
  if (q == MAPA) pantallaMapa();
  else if (q == ALERTA) { int i = sigAtaque(alertaHash); if (i >= 0) alertaHash = hashNombre(datos.s[i].nom); pantallaAlerta(); }   // cada vuelta, el siguiente
  else if (q == VISITAS) { pagVis = 0; pantallaVisitas(); }
  else pantallaResumen(true);
}
