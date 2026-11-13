// mapa.h - posiciones, nombres, barrido del radar y aros de alerta
#pragma once

void preparaTablas() {
  for (int a = 0; a < 360; a++) { SENO[a] = sinf(a * PI / 180); COSENO[a] = cosf(a * PI / 180); }
}
inline int grado(int a) { a %= 360; return a < 0 ? a + 360 : a; }

// Decide que nombres se pintan: primero alertas, luego controlados (con zoom), luego el resto (mas zoom).
// Si un nombre chocaria con otro ya puesto, no se pinta.
void eligeEtiquetas() {
  static int16_t rx[MAX_SIS], ry[MAX_SIS], rw[MAX_SIS]; int nr = 0;
  for (int i = 0; i < datos.n; i++) datos.s[i].lab = false;
  for (int pasada = 0; pasada < 3; pasada++) {
    if (pasada == 1 && zoom < zoomDef + 1) break;
    if (pasada == 2 && zoom < zoomDef + 3) break;
    for (int i = 0; i < datos.n; i++) {
      Sis &s = datos.s[i];
      bool toca = pasada == 0 ? s.nivel > 0 : pasada == 1 ? (s.ctrl && !s.nivel) : (!s.ctrl && !s.nivel);
      if (!toca || !dentro(s.sx, s.sy, MX, MY, MW, MH)) continue;
      int x = s.sx + (s.nivel ? 16 : 9), y = s.sy - 4, w = s.anchoLab;
      bool choca = false;
      for (int k = 0; k < nr && !choca; k++)
        choca = x < rx[k] + rw[k] + 4 && rx[k] < x + w + 4 && abs(y - ry[k]) < 10;
      if (!choca) { s.lab = true; rx[nr] = x; ry[nr] = y; rw[nr] = w; nr++; }
    }
  }
}

// Todo lo que depende del zoom y del centro se calcula aqui una vez (no en cada paso del barrido)
void calculaPantalla() {
  float k = ZOOMS[zoom];
  for (int i = 0; i < datos.n; i++) {
    Sis &s = datos.s[i];
    float dx = (s.x - centroX) * k, dz = (s.z - centroZ) * k;
    s.sx = constrain(CX + dx, -2000, 2000); s.sy = constrain(CY - dz, -2000, 2000);
    float a = atan2f(dx, dz) * 180 / PI; s.ang = a < 0 ? a + 360 : a;
    s.dist = min(hypotf(dx, dz), 9999.0f);
    s.anchoLab = min((int)strlen(s.nom) * 6, 255);
  }
  eligeEtiquetas();
}

float mediana(float *v, int n) {
  for (int i = 1; i < n; i++) for (int j = i; j > 0 && v[j] < v[j - 1]; j--) { float t = v[j]; v[j] = v[j - 1]; v[j - 1] = t; }
  return n ? v[n / 2] : 0;
}

void vistaPorDefecto() {             // centro = mediana de los controlados; zoom para que quepa el 80 %
  static float xs[MAX_SIS], zs[MAX_SIS], ds[MAX_SIS];
  int n = 0;
  for (int i = 0; i < datos.n; i++) if (datos.s[i].ctrl) { xs[n] = datos.s[i].x; zs[n] = datos.s[i].z; n++; }
  if (!n) for (int i = 0; i < datos.n; i++) { xs[n] = datos.s[i].x; zs[n] = datos.s[i].z; n++; }
  defX = mediana(xs, n); defZ = mediana(zs, n);
  for (int i = 0; i < datos.n; i++) ds[i] = hypotf(datos.s[i].x - defX, datos.s[i].z - defZ);
  mediana(ds, datos.n);
  float d80 = datos.n ? ds[(datos.n * 8) / 10] : 50;
  float k = 0.9f * R / max(d80, 1.0f);
  zoomDef = 0;
  for (int i = 0; i < NZ; i++) if (ZOOMS[i] <= k) zoomDef = i;
  centroX = defX; centroZ = defZ; zoom = zoomDef;
}

// ---------- el mapa se dibuja en memoria y se manda a la pantalla ----------
// Dos sprites de 4 bits (16 colores) de media altura cada uno: 2 x 21,6 KB caben mejor en la RAM troceada
enum : uint8_t { I_FONDO, I_REJ, I_E0, I_E1, I_E2, I_E3, I_E4, I_E5, I_E6, I_VERDE, I_AMAR, I_NAR, I_ROJO, I_BLANCO, I_NMED, I_ILUM };
const int MITAD = MH / 2;
TFT_eSprite sprA(&tft), sprB(&tft);                // A = mitad de arriba, B = mitad de abajo
bool sprOk = false;
uint16_t paleta[16];
const int SCX = CX - MX, SCY = CY - MY;            // centro del radar dentro del mapa

bool creaMitad(TFT_eSprite &g) {
  if (g.created()) return true;
  g.setColorDepth(4);
  if (!g.createSprite(MW, MITAD)) return false;
  g.createPalette(paleta);
  return true;
}

void creaSprite() {
  if (sprOk) return;
  sprOk = creaMitad(sprA) && creaMitad(sprB);
  if (!sprOk) { sprA.deleteSprite(); sprB.deleteSprite(); }
  static bool avisado = false;
  if (sprOk || !avisado) Serial.printf("Mapa en memoria: %s, RAM libre %u (bloque mayor %u)\n",
                                       sprOk ? "OK" : "SIN MEMORIA", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  avisado = !sprOk;
}
void liberaSprite() { sprA.deleteSprite(); sprB.deleteSprite(); sprOk = false; }

void preparaSprite() {
  paleta[I_FONDO] = FONDO; paleta[I_REJ] = REJILLA;
  for (int i = 0; i < 7; i++) paleta[I_E0 + i] = estela[i * 2];      // 7 tonos de la estela
  paleta[I_VERDE] = VERDE; paleta[I_AMAR] = AMAR; paleta[I_NAR] = NAR; paleta[I_ROJO] = ROJO;
  paleta[I_BLANCO] = BLANCO; paleta[I_NMED] = NAR_MED; paleta[I_ILUM] = ILUM;
  creaSprite();
}

uint8_t iColorInf(float inf) { return inf > 50 ? I_VERDE : inf > 20 ? I_AMAR : I_NAR; }

// Todo se dibuja en una mitad "g" desplazada "oy" pixeles (lo que cae fuera se recorta solo)
void cuna(TFT_eSprite &g, int oy, int a1, int a2, uint8_t c) {
  a1 = grado(a1); a2 = grado(a2 + 1);              // +1 grado para que no queden rayas entre cunas
  g.fillTriangle(SCX, SCY - oy, SCX + R * SENO[a1], SCY - oy - R * COSENO[a1], SCX + R * SENO[a2], SCY - oy - R * COSENO[a2], c);
}

void pintaSistema(TFT_eSprite &g, int oy, const Sis &s, bool iluminado) {
  int x = s.sx - MX, y = s.sy - MY - oy;
  if (!dentro(x, y, -20, -20, MW + 40, MITAD + 40)) return;
  uint8_t c = iluminado ? I_ILUM : iColorInf(s.inf);
  if (s.ctrl) g.fillRect(x - 3, y - 3, 7, 7, c);
  else g.fillCircle(x, y, 2, c);
  if (s.nivel == 1) { g.drawCircle(x, y, 10, I_AMAR); g.drawCircle(x, y, 11, I_AMAR); }
  if (s.nivel == 2) { g.drawCircle(x, y, radioAro, I_ROJO); g.drawCircle(x, y, radioAro + 1, I_ROJO); }
  if (s.lab) {
    g.setTextColor(s.nivel == 2 ? I_ROJO : s.nivel == 1 ? I_AMAR : s.ctrl ? I_BLANCO : I_NMED);
    g.setTextDatum(ML_DATUM);
    g.drawString(s.nom, x + (s.nivel ? 16 : 9), y, 1);
  }
}

void pintaMitad(TFT_eSprite &g, int oy) {
  g.fillSprite(I_FONDO);
  for (int i = 1; i <= 3; i++) g.drawCircle(SCX, SCY - oy, R * i / 3, I_REJ);
  g.drawFastHLine(SCX - R, SCY - oy, 2 * R, I_REJ);
  g.drawFastVLine(SCX, SCY - oy - R, 2 * R, I_REJ);
  for (int i = N_ESTELA - 1; i >= 0; i--) cuna(g, oy, angulo - (i + 1) * PASO, angulo - i * PASO, I_E0 + i / 2);
  for (int i = 1; i <= 3; i++) g.drawCircle(SCX, SCY - oy, R * i / 3, I_REJ);   // la rejilla se ve a traves de la estela
  for (int i = 0; i < datos.n; i++) {
    const Sis &s = datos.s[i];
    int d = grado(angulo - (int)s.ang);
    pintaSistema(g, oy, s, d < 2 * PASO && s.dist <= R);
  }
  char t[24];
  g.setTextColor(I_REJ);
  g.setTextDatum(BL_DATUM); g.drawString(VERSION, 3, MH - 2 - oy, 1);
  snprintf(t, sizeof t, "anillo %.0f ly", R / 3.0f / ZOOMS[zoom]);
  g.setTextDatum(BR_DATUM); g.drawString(t, MW - 3, MH - 2 - oy, 1);
}

void pintaMapa() { pintaMitad(sprA, 0); pintaMitad(sprB, MITAD); }

void mandaZona(int x0, int y0, int x1, int y1) {   // manda a la pantalla un trozo del mapa (coordenadas del mapa)
  x0 = max(0, x0); y0 = max(0, y0); x1 = min(MW - 1, x1); y1 = min(MH - 1, y1);
  if (x1 < x0 || y1 < y0) return;
  if (y0 < MITAD) {                                // parte de arriba
    int b = min(y1, MITAD - 1);
    sprA.pushSprite(MX + x0, MY + y0, x0, y0, x1 - x0 + 1, b - y0 + 1);
  }
  if (y1 >= MITAD) {                               // parte de abajo
    int a = max(y0, MITAD);
    sprB.pushSprite(MX + x0, MY + a, x0, a - MITAD, x1 - x0 + 1, y1 - a + 1);
  }
}

void dibujaMapaEntero() {
  creaSprite();                                    // por si se solto en la descarga
  if (!sprOk) {
    tft.fillRect(MX, MY, MW, MH, FONDO);
    tft.setTextColor(ROJO); tft.setTextDatum(MC_DATUM); tft.drawString("preparando el mapa...", CX, CY, 2);
    return;
  }
  pintaMapa();
  sprA.pushSprite(MX, MY); sprB.pushSprite(MX, MY + MITAD);
}

// Cada paso: avanza la barra y el latido, repinta en memoria y manda solo lo que cambia
void tickMapa() {
  if (!sprOk) {                                    // si no habia sitio, se reintenta cada 5 s
    static uint32_t t = 0;
    if (millis() - t > 5000) { t = millis(); creaSprite(); if (sprOk) dibujaMapaEntero(); }
    return;
  }
  static uint8_t medio = 0;
  if (++medio % 2 == 0) { fasePulso = (fasePulso + 1) % 10; radioAro = 8 + (fasePulso < 5 ? fasePulso : 10 - fasePulso); }
  int antes = angulo;
  if (!pausa) angulo = grado(angulo + PASO);
  pintaMapa();
  if (angulo != antes) {                           // zona del barrido: de la cola borrada a la punta
    int x0 = SCX, y0 = SCY, x1 = SCX, y1 = SCY;
    for (int a = antes - (N_ESTELA + 1) * PASO - 2; a <= angulo + 2 + (angulo < antes ? 360 : 0); a += 4) {
      int g = grado(a), px = SCX + R * SENO[g], py = SCY - R * COSENO[g];
      x0 = min(x0, px); x1 = max(x1, px); y0 = min(y0, py); y1 = max(y1, py);
    }
    mandaZona(x0 - 2, y0 - 2, x1 + 2, y1 + 2);
  }
  for (int i = 0; i < datos.n; i++) {              // aros rojos que laten
    const Sis &s = datos.s[i];
    if (s.nivel == 2) mandaZona(s.sx - MX - 15, s.sy - MY - 15, s.sx - MX + 15, s.sy - MY + 15);
  }
}

int sistemaEn(int x, int y) {
  // 1) los que tienen aro tienen prioridad y se pueden tocar en todo el aro
  int mejor = -1, dmin = TOQUE_ARO * TOQUE_ARO;
  for (int i = 0; i < datos.n; i++) {
    if (!datos.s[i].nivel) continue;
    int dx = datos.s[i].sx - x, dy = datos.s[i].sy - y, d = dx * dx + dy * dy;
    if (d < dmin || (d == dmin && datos.s[i].nivel == 2)) { dmin = d; mejor = i; }
  }
  if (mejor >= 0) return mejor;
  // 2) el resto
  dmin = TOQUE_SIS * TOQUE_SIS;
  for (int i = 0; i < datos.n; i++) {
    int dx = datos.s[i].sx - x, dy = datos.s[i].sy - y, d = dx * dx + dy * dy;
    if (d < dmin) { dmin = d; mejor = i; }
  }
  return mejor;
}
