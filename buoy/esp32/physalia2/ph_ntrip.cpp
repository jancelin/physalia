// =============================================================================
// ph_ntrip.cpp – Connexion au caster NTRIP, flux RTCM, envoi GGA NEAR optimisé
// =============================================================================
#include "ph_globals.h"

// forwards
void feedTaskWatchdog();

static bool copyLastGga(char *buf, size_t size)
{
  if (!buf || size == 0) return false;
  buf[0] = '\0';

  if (ggaMutex && xSemaphoreTake(ggaMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    strncpy(buf, lastGGASentence.c_str(), size - 1);
    buf[size - 1] = '\0';
    xSemaphoreGive(ggaMutex);
  } else if (!ggaMutex) {
    strncpy(buf, lastGGASentence.c_str(), size - 1);
    buf[size - 1] = '\0';
  }

  return buf[0] != '\0';
}

// -------------------------------------------------------------------------------------------------
// maintainNtrip() – connexion au caster uniquement dès qu'une GGA exploitable existe.
// La fenêtre d'acquisition ne démarre PAS ici ; elle démarre au premier RTK FIX.
// -------------------------------------------------------------------------------------------------
void maintainNtrip()
{
  if (ntripClient.connected()) return;

  const unsigned long now = millis();
  if (now - lastNtripAttempt_ms < NTRIP_RETRY_DELAY_MS) return;

  char gga[GNSS_LINE_BUFFER_SIZE];
  if (!copyLastGga(gga, sizeof(gga))) {
    LOGF(2, "[NTRIP] Waiting for GGA before opening NEAR stream (%lu ms)\n",
         (unsigned long)(now - bootStarted_ms));
    return;
  }

  lastNtripAttempt_ms = now;
  LOGF(1, "Opening socket to %s:%u mount=%s\n", casterHost, casterPort, mountPoint);

  if (ntripClient.reqRaw(casterHost, casterPort, mountPoint, casterUser, casterUserPW)) {
    const unsigned long connectedAt = millis();
    if (ntripConnected_ms == 0) ntripConnected_ms = connectedAt;
    lastReceivedRTCM_ms = connectedAt;
    ggaSentToCaster = false;
    LOGLN(1, "Connected to NTRIP caster");

    if (transmitLocation && ntripClient.sendGGA(gga)) {
      ggaSentToCaster = true;
      lastGgaToCaster_ms = millis();
      LOGF(1, "[NTRIP] Initial GGA sent to caster at %lu ms\n",
           (unsigned long)lastGgaToCaster_ms);
    }
    return;
  }

  LOGLN(1, "Could not connect to NTRIP caster; retry scheduled.");
}

// -------------------------------------------------------------------------------------------------
// processNtripStream() – lit les octets RTCM disponibles et les envoie à l'UM980
// -------------------------------------------------------------------------------------------------
void processNtripStream()
{
  if (!ntripClient.connected()) return;

  uint8_t buf[2048];
  size_t  n = 0;

  while (ntripClient.available()) {
    int c = ntripClient.read();
    if (c < 0) break;
    buf[n++] = (uint8_t)c;
    if (n == sizeof(buf)) break;
  }

  if (n > 0) {
    const unsigned long now = millis();
    if (firstRtcm_ms == 0) {
      firstRtcm_ms = now;
      LOGF(1, "[NTRIP] First RTCM at %lu ms (%lu ms after NTRIP connect)\n",
           (unsigned long)firstRtcm_ms,
           (unsigned long)(ntripConnected_ms ? firstRtcm_ms - ntripConnected_ms : 0));
    }
    lastReceivedRTCM_ms = now;
    GNSSSerial.write(buf, n);
    feedTaskWatchdog();
    LOGF(2, "Forwarded %u RTCM bytes to UM980\n", (unsigned)n);
  }

  if (lastReceivedRTCM_ms != 0 && millis() - lastReceivedRTCM_ms > MAX_HANGUP_MS) {
    LOGLN(1, "RTCM timeout – disconnecting");
    ntripClient.stop();
    ggaSentToCaster = false;
  }
}

// -------------------------------------------------------------------------------------------------
// sendPeriodicGGA() – mode nominal one-shot. En mode secours, renvoie périodiquement.
// -------------------------------------------------------------------------------------------------
void sendPeriodicGGA()
{
  if (!transmitLocation) return;
  if (!ntripClient.connected()) return;

  if (SEND_GGA_ONCE && ggaSentToCaster) return;
  if (!SEND_GGA_ONCE &&
      millis() - lastGgaToCaster_ms < (unsigned long)SEND_GGA_PERIOD * 1000UL) return;

  char buf[GNSS_LINE_BUFFER_SIZE];
  if (!copyLastGga(buf, sizeof(buf))) return;

  if (ntripClient.sendGGA(buf)) {
    ggaSentToCaster = true;
    lastGgaToCaster_ms = millis();
    LOGF(2, "Pushed GGA to caster: %s", buf);
  }
}
