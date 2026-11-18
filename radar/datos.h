// datos.h - lectura de faccion.json y calculo de alertas (sirve en ESP32 y en PC)
#pragma once
#include <ArduinoJson.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_SIS 140       // limite de sistemas (se reserva lo que quepa, hasta este numero)
#define DIAS_TRAF 9       // dias de trafico guardados: hoy, ayer y 7 mas para la media
#define MAX_FAC 7
#define MAX_HIST 10       // puntos de la grafica de influencia

struct Fac { char n[24]; float inf; };

struct Sis {
  char nom[34];
  float x, z;            // posicion galactica (x = este, z = norte)
  float inf, prev;       // influencia y la de ayer (NAN si no hay)
  bool ctrl;
  char act[40], pend[40], rec[40];
  char conf[64];         // conflicto resumido
  bool confActivo, confPend;
  Fac f[MAX_FAC]; uint8_t nf;
  uint16_t trafHoy;
  uint16_t traf[DIAS_TRAF]; // pilotos por dia: [0] hoy, [1] ayer...
  uint8_t ataque;        // 0 nada, 1 ACTIVIDAD INUSUAL (pico de visitas), 2 POSIBLE ATAQUE (pico y baja la influencia)
  bool picoAyer;         // el pico fue ayer (si no, hoy)
  float media7;          // media de pilotos de los 7 dias anteriores al pico (o a hoy si no hay pico)
  long t;                // hora del dato
  uint8_t nivel;         // 0 nada, 1 aviso, 2 grave
  char alerta[20];       // titulo corto (MAYUSCULAS, sin tildes)
  char aviso[56];        // explicacion
  float ang;             // angulo en el radar (lo calcula la pantalla)
  int16_t sx, sy;        // posicion en pantalla
  bool lab;              // se pinta su nombre (lo decide la pantalla)
  bool nueva;            // alerta que no estaba en la descarga anterior
  int8_t ganados, perdidos; // dias del conflicto
  int16_t dist;          // distancia al centro del radar en pixeles (la calcula la pantalla)
  uint8_t anchoLab;      // ancho del nombre en pixeles (fuente 1)
  long tickUlt, tickProx;   // ultimo tick detectado y proximo previsto (0 = sin dato)
  int16_t tickMarg, tickDisp; // margen del ultimo y variacion diaria, en minutos (-1 = sin dato)
  uint8_t tickN;         // cuantos ticks lleva aprendidos
  uint8_t nh;            // historico de influencia: puntos guardados
  uint32_t ht[MAX_HIST]; // hora de cada punto
  int16_t hv[MAX_HIST];  // influencia x100
};

struct Datos {
  Sis *s = nullptr; int cap = 0;   // tabla de sistemas (se reserva aparte) y cuantos caben
  int n = 0, sinSitio = 0;         // sistemas leidos y los que no cupieron
  long t = 0;            // hora del archivo
  long tickGlobal = 0;   // ultimo tick de la galaxia (tick.edcd.io, via el escucha)
  int controlados = 0, alertas = 0, trafHoy = 0, ataques = 0, inusuales = 0;
};

// Quita tildes y pasa a ASCII (las fuentes de la pantalla no tienen tildes)
static void ascii(char *d, const char *o, size_t max) {
  size_t j = 0;
  for (size_t i = 0; o && o[i] && j + 1 < max; i++) {
    unsigned char c = o[i];
    if (c < 128) { d[j++] = c; continue; }
    if (c == 0xC3 && o[i + 1]) {
      unsigned char k = o[++i];
      const char *m = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPsaaaaaaaceeeeiiiidnooooo/ouuuuypy";
      if (k >= 0x80 && k <= 0xBF) d[j++] = m[k - 0x80];
    }
  }
  d[j] = 0;
}

static const char *traduce(const char *e) {
  static const char *tab[][2] = {
    {"Expansion","Expansion"},{"Retreat","Retirada"},{"War","Guerra"},{"CivilWar","Guerra civil"},
    {"Election","Elecciones"},{"Boom","Auge"},{"Bust","Crisis"},{"CivilUnrest","Disturbios"},
    {"CivilLiberty","Libertad civil"},{"Famine","Hambruna"},{"Outbreak","Epidemia"},
    {"Lockdown","Cierre"},{"PirateAttack","Ataque pirata"},{"InfrastructureFailure","Fallo infraestr."},
    {"NaturalDisaster","Desastre"},{"PublicHoliday","Festivo"},{"Investment","Inversion"},
    {"Drought","Sequia"},{"Blight","Plaga"},{"TerroristAttack","Atentado"},{"ColdWar","Guerra fria"},
    {"TradeWar","Guerra comercial"},{"Revolution","Revolucion"},{"Theocracy","Teocracia"}};
  for (auto &p : tab) if (!strcmp(p[0], e)) return p[1];
  return e;
}

static void lista(char *d, size_t max, JsonArrayConst a) {
  d[0] = 0;
  for (JsonVariantConst v : a) {
    const char *e = v.as<const char *>(); if (!e) continue;
    if (d[0]) strncat(d, ", ", max - strlen(d) - 1);
    strncat(d, traduce(e), max - strlen(d) - 1);
  }
}

static bool contiene(JsonArrayConst a, const char *e) {
  for (JsonVariantConst v : a) { const char *x = v.as<const char *>(); if (x && !strcmp(x, e)) return true; }
  return false;
}

// ---------- lectura por partes: el archivo se lee SISTEMA A SISTEMA, sin cargarlo entero ----------
// (faccion.json ya pasa de 200 KB; asi la memoria necesaria es la de un solo sistema, unos pocos KB)

// Lector con un caracter de vuelta atras; sirve igual para el WiFi (Stream) y para un archivo del PC
#ifndef ARDUINO
#include <istream>
inline int rellena(std::istream &s, char *b, int n) { s.read(b, 1); return (int)s.gcount(); }
#endif
template <typename S> inline int rellena(S &s, char *b, int n) {   // lee lo que haya llegado (de golpe, no byte a byte)
  int k = s.available();
  return (int)s.readBytes(b, k > 0 ? (k < n ? k : n) : 1);          // si no hay nada, espera 1 byte (con timeout)
}

template <typename S> struct Lector {
  S &s; int atras = -1; char buf[256]; int pos = 0, len = 0;
  explicit Lector(S &x) : s(x) {}
  int read() {
    if (atras >= 0) { int c = atras; atras = -1; return c; }
    if (pos >= len) { len = rellena(s, buf, sizeof buf); pos = 0; if (len <= 0) { len = 0; return -1; } }
    return (uint8_t)buf[pos++];
  }
  size_t readBytes(char *b, size_t n) { size_t i = 0; for (; i < n; i++) { int c = read(); if (c < 0) break; b[i] = (char)c; } return i; }
  int salta() { int c; do c = read(); while (c == ' ' || c == '\n' || c == '\r' || c == '\t'); return c; }   // siguiente no-espacio
  bool busca(const char *p) {                       // avanza hasta justo despues del texto p
    size_t n = strlen(p), k = 0; int c;
    while ((c = read()) >= 0) { if (c == p[k]) { if (++k == n) return true; } else k = (c == p[0]) ? 1 : 0; }
    return false;
  }
  double numero() {                                 // lee un numero suelto
    char b[32]; int i = 0, c = salta();
    while (c >= 0 && i < 31 && (isdigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E' || c == '+')) { b[i++] = (char)c; c = read(); }
    atras = c; b[i] = 0; return atof(b);
  }
};

// ---------- dias: "AAAA-MM-DD" menos n dias (calendario civil, sin depender de la hora del sistema) ----------
static long diasDesdeCivil(int y, int m, int d) {
  y -= m <= 2; long era = (y >= 0 ? y : y - 399) / 400; long yoe = y - era * 400;
  long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}
static void diaMenos(char *o, const char *hoy, int n) {
  int y = 0, m = 0, d = 0;
  if (sscanf(hoy, "%d-%d-%d", &y, &m, &d) != 3 || y < 2000) { strcpy(o, "0000-00-00"); return; }
  long z = diasDesdeCivil(y, m, d) - n + 719468;
  long era = (z >= 0 ? z : z - 146096) / 146097; long doe = z - era * 146097;
  long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  long mp = (5 * doy + 2) / 153; int dd = doy - (153 * mp + 2) / 5 + 1, mm = mp < 10 ? mp + 3 : mp - 9;
  int yy = yoe + era * 400 + (mm <= 2);
  snprintf(o, 11, "%04d-%02d-%02d", yy, mm, dd);
}

// Pico de visitas (mismas reglas que la web): hoy o ayer al menos 5 pilotos y el triple de la media
// de los 7 dias anteriores. Si ademas la influencia baja 1 punto o mas desde ayer: posible ataque.
static void calculaPico(Sis &s) {
  s.ataque = 0; s.picoAyer = false;
  for (int k = 0; k <= 1; k++) {
    float m = 0; for (int i = k + 1; i <= k + 7; i++) m += s.traf[i];
    m /= 7;
    if (k == 0) s.media7 = m;
    if (s.traf[k] >= 5 && s.traf[k] >= 3 * (m > 1 ? m : 1)) {
      s.media7 = m; s.picoAyer = k == 1;
      s.ataque = (!isnan(s.prev) && s.prev - s.inf >= 1.0f) ? 2 : 1;
      return;
    }
  }
}
static void textoPico(char *d, size_t n, const Sis &s) {     // "25 pilotos ayer (media 4.4), baja 1.2"
  int v = s.traf[s.picoAyer ? 1 : 0];
  if (s.ataque == 2) snprintf(d, n, "%d pilotos %s (media %.1f), baja %.1f", v, s.picoAyer ? "ayer" : "hoy", s.media7, s.prev - s.inf);
  else snprintf(d, n, "%d pilotos %s (media %.1f)", v, s.picoAyer ? "ayer" : "hoy", s.media7);
}

// Filtro de UN sistema: solo se guarda lo que se pinta
static void preparaFiltro(JsonDocument &f) {
  for (const char *k : {"sys","t","inf","ctrl","fuera","activos","pendientes","recuperando","segunda","segunda_inf","prev","trafico",
                        "tick_ult","tick_prox","tick_marg","tick_disp","tick_n","hinf"}) f[k] = true;
  f["pos"] = true;
  JsonObject c = f["conflictos"][0].to<JsonObject>();
  for (const char *k : {"tipo","estado","contra","ganados","perdidos"}) c[k] = true;
  f["todas"][0]["n"] = true; f["todas"][0]["inf"] = true;
}

static void calculaAlerta(Sis &s, JsonObjectConst o) {
  s.nivel = 0; s.alerta[0] = 0; s.aviso[0] = 0;
  char tmp[40];
  // 1) conflicto activo
  for (JsonObjectConst c : o["conflictos"].as<JsonArrayConst>()) {
    const char *tipo = c["tipo"] | "", *est = c["estado"] | "";
    const char *t = !strcmp(tipo, "war") ? "GUERRA" : !strcmp(tipo, "civilwar") ? "GUERRA CIVIL" : "ELECCIONES";
    ascii(tmp, c["contra"] | "?", sizeof tmp);
    int g = c["ganados"] | 0, p = c["perdidos"] | 0;
    s.ganados = g; s.perdidos = p;
    if (!strcmp(est, "active")) {
      snprintf(s.alerta, sizeof s.alerta, "%s", t);
      snprintf(s.aviso, sizeof s.aviso, "contra %s, dias %d-%d", tmp, g, p);
      snprintf(s.conf, sizeof s.conf, "%s contra %s", t, tmp);
      s.confActivo = true; s.nivel = 2; return;
    }
    if (!strcmp(est, "pending")) {
      s.confPend = true;
      snprintf(s.conf, sizeof s.conf, "%s pendiente contra %s", t, tmp);
      snprintf(s.alerta, sizeof s.alerta, "%s PEND.", t);
      snprintf(s.aviso, sizeof s.aviso, "contra %s", tmp);
      s.nivel = 2; return;
    }
  }
  JsonArrayConst act = o["activos"], pen = o["pendientes"];
  // 2) retirada
  if (contiene(act, "Retreat") || contiene(pen, "Retreat")) {
    strcpy(s.alerta, "RETIRADA"); snprintf(s.aviso, sizeof s.aviso, "influencia %.1f %%", s.inf); s.nivel = 2; return;
  }
  // 3) ventaja baja en sistema controlado
  float seg = o["segunda_inf"] | 0.0f;
  if (s.ctrl && s.inf - seg < 5.0f) {
    ascii(tmp, o["segunda"] | "?", sizeof tmp);
    strcpy(s.alerta, "VENTAJA BAJA");
    snprintf(s.aviso, sizeof s.aviso, "solo %.1f puntos sobre %s", s.inf - seg, tmp); s.nivel = 2; return;
  }
  // 4) posible ataque: pico de visitas y baja la influencia
  if (s.ataque == 2) { strcpy(s.alerta, "POSIBLE ATAQUE"); textoPico(s.aviso, sizeof s.aviso, s); s.nivel = 2; return; }
  // 5) bajada fuerte desde ayer
  if (!isnan(s.prev) && s.prev - s.inf >= 3.0f) {
    strcpy(s.alerta, "BAJADA");
    snprintf(s.aviso, sizeof s.aviso, "%.1f puntos desde ayer", s.inf - s.prev); s.nivel = 1; return;
  }
  // 6) actividad inusual: pico de visitas sin bajada
  if (s.ataque == 1) { strcpy(s.alerta, "ACTIVIDAD INUSUAL"); textoPico(s.aviso, sizeof s.aviso, s); s.nivel = 1; return; }
  // 7) expansion pendiente
  if (contiene(pen, "Expansion")) {
    strcpy(s.alerta, "EXPANSION PEND.");
    if (seg > 0) { ascii(tmp, o["segunda"] | "?", sizeof tmp);
      if (s.inf >= seg) snprintf(s.aviso, sizeof s.aviso, "ventaja de %.1f sobre %s", s.inf - seg, tmp);
      else snprintf(s.aviso, sizeof s.aviso, "a %.1f puntos de %s", seg - s.inf, tmp); }
    s.nivel = 1; return;
  }
}

static void llenaSistema(Sis &s, JsonObjectConst o, const char *clave, const char *hoy) {
  memset(&s, 0, sizeof s);
  ascii(s.nom, o["sys"] | clave, sizeof s.nom);
  char *sec = strstr(s.nom, " Sector ");                 // nombre corto: "Hyades Sector SU-N b7-5" -> "Hyades SU-N b7-5"
  if (sec) memmove(sec, sec + 7, strlen(sec + 7) + 1);
  s.x = o["pos"][0] | 0.0f; s.z = o["pos"][2] | 0.0f;
  s.inf = o["inf"] | 0.0f;
  s.prev = o["prev"].isNull() ? NAN : o["prev"].as<float>();
  s.ctrl = o["ctrl"] | false;
  s.t = (long)o["t"].as<double>();
  lista(s.act, sizeof s.act, o["activos"]);
  lista(s.pend, sizeof s.pend, o["pendientes"]);
  lista(s.rec, sizeof s.rec, o["recuperando"]);
  for (int k = 0; k < DIAS_TRAF; k++) {                  // trafico de hoy hacia atras
    char dia[11]; diaMenos(dia, hoy, k);
    s.traf[k] = o["trafico"][dia] | 0;
  }
  s.trafHoy = s.traf[0];
  s.tickUlt = (long)(o["tick_ult"] | 0.0);  s.tickProx = (long)(o["tick_prox"] | 0.0);
  s.tickMarg = o["tick_marg"].isNull() ? -1 : o["tick_marg"].as<int>();
  s.tickDisp = o["tick_disp"].isNull() ? -1 : o["tick_disp"].as<int>();
  s.tickN = o["tick_n"] | 0;
  JsonArrayConst hi = o["hinf"];                          // los ultimos MAX_HIST puntos
  int ini = hi.size() > MAX_HIST ? hi.size() - MAX_HIST : 0, k = 0;
  for (JsonArrayConst pt : hi) {
    if (k++ < ini) continue;
    s.ht[s.nh] = pt[0].as<uint32_t>(); s.hv[s.nh] = (int16_t)roundf(pt[1].as<float>() * 100); s.nh++;
  }
  for (JsonObjectConst f : o["todas"].as<JsonArrayConst>()) {
    if (s.nf >= MAX_FAC) break;
    ascii(s.f[s.nf].n, f["n"] | "?", sizeof s.f[0].n);
    s.f[s.nf].inf = f["inf"] | 0.0f; s.nf++;
  }
  calculaPico(s);
  for (int i = 1; i < s.nf; i++)            // ordenar de mayor a menor
    for (int j = i; j > 0 && s.f[j].inf > s.f[j - 1].inf; j--) { Fac t = s.f[j]; s.f[j] = s.f[j - 1]; s.f[j - 1] = t; }
  calculaAlerta(s, o);
}

// hoy = "AAAA-MM-DD" en UTC (Elite usa UTC)
template <typename Fuente>
static bool cargaDatos(Datos &d, Fuente &in, const char *hoy, char *err, size_t errmax) {
  Lector<Fuente> L(in);
  if (!L.busca("\"sistemas\"") || L.salta() != ':' || L.salta() != '{') { snprintf(err, errmax, "JSON: sin sistemas"); return false; }
  JsonDocument filtro; preparaFiltro(filtro);
  JsonDocument doc;                                  // un solo sistema cada vez
  int n = 0, ctrl = 0, al = 0, traf = 0, at = 0, inu = 0, sin = 0;
  char clave[48];
  while (true) {
    int c = L.salta();
    if (c == '}') break;                             // fin de "sistemas"
    if (c == ',') continue;
    if (c != '"') { snprintf(err, errmax, "JSON: formato raro (%c)", c); return false; }
    int i = 0;                                       // nombre del sistema (la clave)
    while ((c = L.read()) >= 0 && c != '"') { if (c == '\\') c = L.read(); if (i < 47) clave[i++] = (char)c; }
    clave[i] = 0;
    if (L.salta() != ':') { snprintf(err, errmax, "JSON: falta ':'"); return false; }
    DeserializationError e = deserializeJson(doc, L, DeserializationOption::Filter(filtro));
    if (e) { snprintf(err, errmax, "JSON %s: %s", clave, e.c_str()); return false; }
    JsonObjectConst o = doc.as<JsonObjectConst>();
    if ((o["fuera"] | false) || o["inf"].isNull()) continue;   // ya no esta la faccion
    if (n >= d.cap) { sin++; continue; }                        // no cabe: se cuenta para avisar
    Sis &s = d.s[n];
    llenaSistema(s, o, clave, hoy);
    ctrl += s.ctrl; al += s.nivel > 0; traf += s.trafHoy; at += s.ataque == 2; inu += s.ataque == 1;
    n++;
  }
  // despues de "sistemas" vienen la hora del archivo y el tick global
  long t = 0, tg = 0;
  if (L.busca("\"t\"") && L.salta() == ':') t = (long)L.numero();
  if (L.busca("\"tick_global\"") && L.salta() == ':') tg = (long)L.numero();
  d.n = n; d.controlados = ctrl; d.alertas = al; d.trafHoy = traf; d.ataques = at; d.inusuales = inu; d.sinSitio = sin;
  d.t = t; d.tickGlobal = tg;
  return true;
}
