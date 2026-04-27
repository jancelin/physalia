// =============================================================================
// ph_gnss.cpp – Lecture UART UM980, parsing PVTSLNA/GGA, init GNSS
// =============================================================================
#include "ph_globals.h"

// forwards
void feedTaskWatchdog();
void handleFailureCycleAndSleep(const char *stage);
void publishFix(const PvtslnData &fix);
void publishBatteryIfDue(const char *datetimeValue);

// -------------------------------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------------------------------
int splitCsv(char *str, char **tokens, int maxTokens)
{
  int count = 0;
  char *saveptr = nullptr;
  char *token = strtok_r(str, ",", &saveptr);
  while (token && count < maxTokens) {
    tokens[count++] = token;
    token = strtok_r(nullptr, ",", &saveptr);
  }
  return count;
}

bool gpsWeekTowToUtcString(uint16_t gpsWeek, uint32_t towMs, int leapSeconds,
                            char *out, size_t outSize)
{
  constexpr int64_t GPS_EPOCH_UNIX = 315964800LL;
  int64_t utcMs  = (int64_t)gpsWeek * 604800000LL + towMs - (int64_t)leapSeconds * 1000LL;
  int64_t unixMs = GPS_EPOCH_UNIX * 1000LL + utcMs;
  time_t  sec    = (time_t)(unixMs / 1000LL);
  int     ms     = (int)(unixMs % 1000LL);
  if (ms < 0) { ms += 1000; sec--; }
  struct tm t;
  gmtime_r(&sec, &t);
  snprintf(out, outSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec, ms);
  return true;
}

// -------------------------------------------------------------------------------------------------
// parsePvtslnLine() – décode une trame #PVTSLNA de l'UM980
// -------------------------------------------------------------------------------------------------
bool parsePvtslnLine(const char *line, PvtslnData &out)
{
  if (!line) return false;

  char buf[GNSS_LINE_BUFFER_SIZE];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char *crc = strchr(buf, '*');
  if (crc) *crc = '\0';

  char *semi = strchr(buf, ';');
  if (!semi) return false;
  *semi = '\0';

  char *hdr  = buf;
  char *pay  = semi + 1;

  char *hTok[16] = {};
  char *pTok[96] = {};
  int hc = splitCsv(hdr, hTok, 16);
  int pc = splitCsv(pay, pTok, 96);
  if (hc < 9 || pc < 33) return false;

  uint16_t gpsWeek  = (uint16_t)atoi(hTok[4]);
  uint32_t towMs    = (uint32_t)strtoul(hTok[5], nullptr, 10);
  int      leapSecs = atoi(hTok[8]);
  if (!gpsWeekTowToUtcString(gpsWeek, towMs, leapSecs, out.datetime, sizeof(out.datetime)))
    strncpy(out.datetime, "1970-01-01 00:00:00.000", sizeof(out.datetime) - 1);

  out.bestposType     = pTok[0];
  out.altMSL          = strtod(pTok[1],  nullptr);
  out.lat             = strtod(pTok[2],  nullptr);
  out.lon             = strtod(pTok[3],  nullptr);
  out.hgtStd          = strtof(pTok[4],  nullptr);
  out.latStd          = strtof(pTok[5],  nullptr);
  out.lonStd          = strtof(pTok[6],  nullptr);
  out.diffAge         = strtof(pTok[7],  nullptr);
  out.undulation      = strtod(pTok[12], nullptr);
  out.ellipsoid       = out.altMSL + out.undulation;
  out.bestposTrackedSvs  = (uint8_t)atoi(pTok[13]);
  out.bestposSolnSvs     = (uint8_t)atoi(pTok[14]);
  out.psrposTrackedSvs   = (uint8_t)atoi(pTok[15]);
  out.psrposSolnSvs      = (uint8_t)atoi(pTok[16]);
  out.velNorth        = strtod(pTok[17], nullptr);
  out.velEast         = strtod(pTok[18], nullptr);
  out.velGround       = strtod(pTok[19], nullptr);
  out.headingType     = pTok[20];
  out.headingLength   = strtof(pTok[21], nullptr);
  out.headingDeg      = strtof(pTok[22], nullptr);
  out.headingPitch    = strtof(pTok[23], nullptr);
  out.headingTrackedSvs  = (uint8_t)atoi(pTok[24]);
  out.headingSolnSvs     = (uint8_t)atoi(pTok[25]);
  out.headingGgl1        = (uint8_t)atoi(pTok[26]);
  out.headingGgl1l2      = (uint8_t)atoi(pTok[27]);
  out.gdop  = strtof(pTok[28], nullptr);
  out.pdop  = strtof(pTok[29], nullptr);
  out.hdop  = strtof(pTok[30], nullptr);
  out.htdop = strtof(pTok[31], nullptr);
  out.tdop  = strtof(pTok[32], nullptr);
  out.valid = true;
  return true;
}

// -------------------------------------------------------------------------------------------------
// processGnssLine() – dispatch GGA / PVTSLNA
// Core 0 en fonctionnement normal ; Core 1 pendant setupGnss() (avant gnssTask).
// -------------------------------------------------------------------------------------------------
void processGnssLine(const char *line)
{
  if (!line || line[0] == '\0') return;

  // --- GGA ---
  if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0) {
    String gga = String(line) + "\r\n";
    if (firstGga_ms == 0) {
      firstGga_ms = millis();
      LOGF(1, "[GNSS] First GGA ready at %lu ms\n", (unsigned long)firstGga_ms);
    }

    if (ggaMutex && xSemaphoreTake(ggaMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      lastGGASentence = gga;
      ntripClient.setLastGGA(lastGGASentence);
      xSemaphoreGive(ggaMutex);
    } else if (!ggaMutex) {
      lastGGASentence = gga;
      ntripClient.setLastGGA(lastGGASentence);
    }
    return;
  }

  // --- PVTSLNA ---
  if (strncmp(line, "#PVTSLNA", 8) == 0) {
    PvtslnData parsed;
    if (!parsePvtslnLine(line, parsed)) return;
    lastFix      = parsed;
    lastPvtsln_ms = millis();

    if (pvtQueue) {
      if (xQueueSend(pvtQueue, &parsed, 0) != pdTRUE) {
        PvtslnData disc;
        xQueueReceive(pvtQueue, &disc, 0);
        xQueueSend(pvtQueue, &parsed, 0);
        LOGLN(1, "[GNSS] PVT queue full – oldest frame dropped");
      }
    } else {
      // chemin d'init (setupGnss avant gnssTask)
      publishFix(parsed);
      publishBatteryIfDue(parsed.datetime);
    }
  }
}

// -------------------------------------------------------------------------------------------------
// processGnssSerial() – vide le buffer UART UM980, construit les lignes
// Appelée en boucle depuis gnssTask (Core 0) et depuis setupGnss (Core 1 / init).
// -------------------------------------------------------------------------------------------------
void processGnssSerial()
{
  while (GNSSSerial.available()) {
    char c = (char)GNSSSerial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (gnssLineLen > 0) {
        gnssLine[gnssLineLen] = '\0';
        processGnssLine(gnssLine);
        gnssLineLen = 0;
      }
      continue;
    }
    if (gnssLineLen < GNSS_LINE_BUFFER_SIZE - 1)
      gnssLine[gnssLineLen++] = c;
    else
      gnssLineLen = 0;
  }
}

// -------------------------------------------------------------------------------------------------
// setupGnss() – démarre UART UM980 et attend le premier message
// -------------------------------------------------------------------------------------------------
void setupGnss()
{
  GNSSSerial.setRxBufferSize(4096);
  GNSSSerial.begin(UM980_BAUD, SERIAL_8N1, UM980_RX_PIN, UM980_TX_PIN);
  LOGLN(1, "UM980 UART @ 460800 baud");

  unsigned long start = millis();
  bool ok = false;
  while (millis() - start < (unsigned long)ACQUISION_PERIOD_GNSS) {
    feedTaskWatchdog();
    processGnssSerial();
    if (lastPvtsln_ms != 0 || lastGGASentence.length() > 0) { ok = true; break; }
    delay(20);
  }

  if (!ok) {
    LOGLN(1, "No UM980 output detected");
    if (DEEP_SLEEP_ACTIVATED) handleFailureCycleAndSleep("GNSS");
  } else {
    LOGLN(1, "UM980 output detected");
  }
}
