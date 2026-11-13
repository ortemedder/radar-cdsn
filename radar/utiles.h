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
