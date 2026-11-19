// utiles.h - funciones pequenas de ayuda
#pragma once

uint16_t mezcla(uint16_t a, uint16_t b, float t) {   // t=0 -> a, t=1 -> b
  uint8_t r1 = (a >> 11) << 3, g1 = ((a >> 5) & 63) << 2, b1 = (a & 31) << 3;
  uint8_t r2 = (b >> 11) << 3, g2 = ((b >> 5) & 63) << 2, b2 = (b & 31) << 3;
  return tft.color565(r1 + (r2 - r1) * t, g1 + (g2 - g1) * t, b1 + (b2 - b1) * t);
}

void coma(char *d, size_t n, float v, const char *fmt = "%.1f") {  // 53.2 -> 53,2
  snprintf(d, n, fmt, v);
  for (char *p = d; *p; p++) if (*p == '.') *p = ',';
}

void corta(char *d, const char *o, int ancho, int fuente) {      // recorta con "..." (d de 64)
  strncpy(d, o, 63); d[63] = 0;
  if (tft.textWidth(d, fuente) <= ancho) return;
  int n = strlen(d), puntos = tft.textWidth("...", fuente);
  while (n > 0 && tft.textWidth(d, fuente) > ancho - puntos) d[--n] = 0;
  strcat(d, "...");
}

long ahora() { time_t t; time(&t); return t > 1700000000 ? (long)t : 0; }

void horaLocal(char *d, size_t n, long t) {                       // "15:30"
  if (!t) { snprintf(d, n, "--:--"); return; }
  struct tm lt; time_t tt = t; localtime_r(&tt, &lt); strftime(d, n, "%H:%M", &lt);
}

void duracion(char *d, size_t n, long s) {                       // "25 min", "3 h 10", "2 d"
  long m = s / 60;
  if (m < 60) snprintf(d, n, "%ld min", m);
  else if (m < 48 * 60) snprintf(d, n, m % 60 ? "%ld h %02ld" : "%ld h", m / 60, m % 60);
  else snprintf(d, n, "%ld d", m / 1440);
}

void haceTexto(char *d, size_t n, long t) {                      // "hace 12 min"
  long a = ahora();
  if (!a || !t) { snprintf(d, n, "--"); return; }
  if (a - t < 60) { snprintf(d, n, "ahora"); return; }
  char x[16]; duracion(x, sizeof x, a - t); snprintf(d, n, "hace %s", x);
}

uint16_t colorInf(float inf) { return inf > 50 ? VERDE : inf > 20 ? AMAR : NAR; }
uint16_t colorNivel(uint8_t nv) { return nv == 2 ? ROJO : AMAR; }

// ---------- colores de los avisos (ver Tono en datos.h) ----------
bool parpOn() { return (millis() / 500) % 2 == 0; }              // fase del parpadeo (medio segundo)
uint16_t colorTono(uint8_t t, bool on = true) {                  // color del borde / texto (on = false: apagado del parpadeo)
  uint16_t c;
  switch (t) {
    case TN_AVISO: c = NAR_AV; break;
    case TN_DISPUTA: c = AMAR_AV; break;
    case TN_FAVOR: c = VERDE; break;
    case TN_GRAVE: c = ROJO; break;
    case TN_PEND: c = MORADO; break;
    default: c = BLANCO;
  }
  return on ? c : mezcla(c, TFT_BLACK, 0.65f);
}
uint16_t textoTono(uint8_t t) { return t == TN_DISPUTA || t == TN_FAVOR ? TFT_BLACK : BLANCO; }   // texto sobre la banda (negro en amarillo y verde)
uint16_t colorSis(const Sis &s) { return s.nivel ? colorTono(s.tono) : BLANCO; }

// Icono de dos espadas cruzadas (14 x 14) en x,y (esquina de arriba a la izquierda)
void espadas(int x, int y, uint16_t c) {
  for (int k = 0; k < 2; k++) {                   // hojas (2 px de gruesas)
    tft.drawLine(x + 1 + k, y + 1, x + 11 + k, y + 11, c);
    tft.drawLine(x + 12 - k, y + 1, x + 2 - k, y + 11, c);
  }
  tft.drawLine(x + 7, y + 10, x + 10, y + 7, c);  // guardas
  tft.drawLine(x + 3, y + 7, x + 6, y + 10, c);
  tft.fillRect(x, y + 12, 2, 2, c); tft.fillRect(x + 12, y + 12, 2, 2, c);   // pomos
}

// Sobre (elecciones), 14 x 14
void sobre(int x, int y, uint16_t c) {
  tft.drawRect(x, y + 2, 14, 10, c); tft.drawRect(x + 1, y + 3, 12, 8, c);
  tft.drawLine(x + 1, y + 3, x + 7, y + 8, c); tft.drawLine(x + 13, y + 3, x + 7, y + 8, c);
}

// Banda de titulo de un aviso: fondo del color del tono y texto encima (espadas = guerra, sobre = elecciones).
// Las de conflictos en marcha se vuelven a pintar cada medio segundo para que parpadeen.
void bandaAviso(int x, int y, int w, int h, const Sis &s, int fuente, bool on = true) {
  on = on || !s.parpadea;
  uint16_t fondo = colorTono(s.tono, on), txt = on ? textoTono(s.tono) : mezcla(textoTono(s.tono), fondo, 0.4f);
  tft.fillRoundRect(x, y, w, h, 3, fondo);
  int tx = x + 4;
  if (s.icono) { (s.icono == IC_SOBRE ? sobre : espadas)(tx, y + (h - 14) / 2, txt); tx += 18; }
  char u[64]; corta(u, s.alerta, x + w - tx - 4, fuente);
  tft.setTextColor(txt); tft.setTextDatum(ML_DATUM); tft.drawString(u, tx, y + h / 2 + 1, fuente);
  tft.setTextDatum(TL_DATUM);
}

bool dentro(int x, int y, int x0, int y0, int w, int h) { return x >= x0 && x < x0 + w && y >= y0 && y < y0 + h; }

uint32_t hashNombre(const char *s) { uint32_t h = 2166136261u; while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; } return h; }
bool estaEn(uint32_t h, const uint32_t *v, int n) { for (int i = 0; i < n; i++) if (v[i] == h) return true; return false; }

// proximo tick de un sistema: si la prevision ya paso, se pasa al dia siguiente
long proxTick(const Sis &s) {
  long p = s.tickProx, a = ahora();
  if (!p || !a) return 0;
  while (p < a - 1800) p += 86400;                 // media hora de cortesia por si va con retraso
  return p;
}
