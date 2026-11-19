// botones.h - una sola tabla de botones fijos: sirve para dibujarlos y para saber que se ha tocado
#pragma once

struct Boton {
  Modo pantalla;            // en que pantalla esta
  int16_t x, y, w, h;
  const char *txt;          // nullptr = zona invisible (se dibuja aparte)
  void (*accion)();
};

// --- acciones ---
void accZoomMas()   { if (zoom < NZ - 1) { zoom++; pantallaMapa(); } }
void accZoomMenos() { if (zoom > 0) { zoom--; pantallaMapa(); } }
void accCentro()    { zoom = zoomDef; centroX = defX; centroZ = defZ; pantallaMapa(); }
void accLista()     { pagLista = 0; pantallaLista(); }
void accSigAlerta() { if (datos.alertas > AL_N) { pagAlerta = (pagAlerta + 1) % ((datos.alertas + AL_N - 1) / AL_N); panel(); } }
void accVolverFicha() { if (fichaVuelve == LISTA) pantallaLista(); else if (fichaVuelve == VISITAS) pantallaVisitas(); else pantallaMapa(); }
void accMapa()      { pantallaMapa(); }
void accSigLista()  { pagLista++; pantallaLista(); }
void accGrafica()   { fichaGrafica = !fichaGrafica; pantallaFicha(fichaSis); }
void accVisitas()   { pagVis = 0; pantallaVisitas(); }
void accResumen()   { pantallaResumen(true); }
void accSigVis()    { pagVis++; pantallaVisitas(); }
int buscaHash(uint32_t h) { for (int i = 0; i < datos.n; i++) if (hashNombre(datos.s[i].nom) == h) return i; return -1; }
int sigAtaque(uint32_t h) {                        // siguiente alarma despues de h (da la vuelta); -1 si no hay
  int i0 = buscaHash(h);
  for (int k = 1; k <= datos.n; k++) { int i = (i0 + k + datos.n) % datos.n; if (datos.s[i].alarma) return i; }
  return -1;
}
void accAleFicha()  { alertaFija = false; int i = buscaHash(alertaHash); fichaVuelve = MAPA; if (i >= 0) pantallaFicha(i); else pantallaMapa(); }
void accAleSig()    { if (datos.alarmas > 1) { int i = sigAtaque(alertaHash); if (i >= 0) alertaHash = hashNombre(datos.s[i].nom); pantallaAlerta(); } }
void accAleVisto()  { alertaFija = false; pantallaMapa(); }

const Boton BOTONES[] = {
  {MAPA,  PX,                BOT_Y, BOT_W, BOT_H, "+", accZoomMas},
  {MAPA,  PX + BOT_PASO,     BOT_Y, BOT_W, BOT_H, "-", accZoomMenos},
  {MAPA,  PX + 2 * BOT_PASO, BOT_Y, BOT_W, BOT_H, "C", accCentro},
  {MAPA,  PX, TIT_H + 1, PW, 48, nullptr, accLista},                 // CONTROLADOS >
  {MAPA,  PX, SIG_Y - 2, PW, SIG_H + 4, nullptr, accSigAlerta},      // siguientes alertas
  {FICHA, VOLVER_X, VOLVER_Y, VOLVER_W, VOLVER_H, "VOLVER", accVolverFicha},
  {FICHA, GRAF_BX, VOLVER_Y, GRAF_BW, VOLVER_H, nullptr, accGrafica},  // GRAFICA / FACCIONES (texto cambia)
  {LISTA, VOLVER_X, VOLVER_Y, VOLVER_W, VOLVER_H, "VOLVER", accMapa},
  {LISTA, LSIG_X, VOLVER_Y, LSIG_W, VOLVER_H, nullptr, accSigLista}, // siguientes de la lista
  {MAPA,  PX, TRAF_Y, PW, TICK_Y - TRAF_Y - 2, nullptr, accVisitas}, // TRAFICO HOY > = pagina VISITAS
  {VISITAS, VOLVER_X, VOLVER_Y, VOLVER_W, VOLVER_H, "VOLVER", accMapa},
  {VISITAS, LSIG_X, VOLVER_Y, LSIG_W, VOLVER_H, nullptr, accSigVis},   // siguientes de visitas
  {MAPA,  0, 0, ANCHO, TIT_H - 4, nullptr, accVisitas},              // titulo: mapa -> visitas -> resumen
  {VISITAS, 0, 0, ANCHO, TIT_H - 4, nullptr, accResumen},
  {ALERTA, ALE_B1, ALE_BY, ALE_BW, ALE_BH, "VER FICHA", accAleFicha},
  {ALERTA, ALE_B2, ALE_BY, ALE_BW, ALE_BH, nullptr, accAleSig},      // SIGUIENTE (solo si hay mas de uno)
  {ALERTA, ALE_B3, ALE_BY, ALE_BW, ALE_BH, nullptr, accAleVisto},    // VISTO (relleno rojo, se dibuja aparte)
};
const int NBOT = sizeof(BOTONES) / sizeof(BOTONES[0]);

void boton(int x, int y, int w, int h, const char *txt, uint16_t c, int fuente = 4) {
  tft.fillRoundRect(x, y, w, h, 5, TFT_BLACK);
  tft.drawRoundRect(x, y, w, h, 5, c);
  tft.setTextColor(c); tft.setTextDatum(MC_DATUM);
  tft.drawString(txt, x + w / 2, y + h / 2 + 1, fuente);
}

void dibujaBotones(Modo m) {
  for (int i = 0; i < NBOT; i++)
    if (BOTONES[i].pantalla == m && BOTONES[i].txt) boton(BOTONES[i].x, BOTONES[i].y, BOTONES[i].w, BOTONES[i].h, BOTONES[i].txt, NAR, m == ALERTA ? 2 : 4);
}

bool pulsaBoton(int x, int y) {                    // 4 px de margen alrededor para acertar mejor
  for (int i = 0; i < NBOT; i++) {
    const Boton &b = BOTONES[i];
    if (b.pantalla == modo && dentro(x, y, b.x - 4, b.y - 4, b.w + 8, b.h + 8)) { b.accion(); return true; }
  }
  return false;
}
