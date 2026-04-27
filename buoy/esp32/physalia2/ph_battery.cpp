// =============================================================================
// ph_battery.cpp – Surveillance batterie Li-Ion 18650 via ADC GPIO35 / fallback modem
// =============================================================================
#include "ph_globals.h"

// forwards
void feedTaskWatchdog();
int32_t readVbatMv();

static bool isVbatPlausible(int32_t mv)
{
  return (mv >= 2000 && mv <= 5000);
}

// -------------------------------------------------------------------------------------------------
// setupBatteryCalibration() – configure l'ADC et vérifie la tension au boot
// -------------------------------------------------------------------------------------------------
void setupBatteryCalibration()
{
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  const esp_adc_cal_value_t cal = esp_adc_cal_characterize(
      ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adcChars);
  adcCalibrated = true;

  if (cal == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    LOGF(1, "ADC cal: eFuse Vref=%u mV (GPIO%d, div ×%d)\n",
         adcChars.vref, ADC_PIN, VBAT_ADC_DIVIDER_RATIO);
    vref = adcChars.vref;
  } else if (cal == ESP_ADC_CAL_VAL_EFUSE_TP) {
    LOGF(1, "ADC cal: Two-Point coeff_a=%u coeff_b=%u (GPIO%d, div ×%d)\n",
         adcChars.coeff_a, adcChars.coeff_b, ADC_PIN, VBAT_ADC_DIVIDER_RATIO);
  } else {
    LOGF(1, "ADC cal: Default 1100 mV (GPIO%d, div ×%d)\n",
         ADC_PIN, VBAT_ADC_DIVIDER_RATIO);
  }

  const int32_t boot_v = readVbatMv();
  LOGF(1, "ADC check: Vbat@boot=%d mV (attendu 3600-4250 mV pour 18650)\n", (int)boot_v);
  if (!isVbatPlausible(boot_v)) {
    LOGLN(1, "ADC WARN: tension hors plage – boot ignoré ; fallback modem utilisé après LTE si disponible");
  }
}

// -------------------------------------------------------------------------------------------------
// readVbatMv() – lecture robuste : 16 échantillons, médiane des 8 centraux, calibration eFuse
// -------------------------------------------------------------------------------------------------
int32_t readVbatMv()
{
  static const uint8_t N    = 16;
  static const uint8_t TRIM = 4;
  static const uint8_t KEPT = N - 2 * TRIM;

  uint32_t s[N];
  for (uint8_t i = 0; i < N; i++) {
    s[i] = (uint32_t)analogRead(ADC_PIN);
    delayMicroseconds(200);
  }

  for (uint8_t i = 1; i < N; i++) {
    uint32_t key = s[i];
    int8_t   j   = (int8_t)i - 1;
    while (j >= 0 && s[j] > key) { s[j + 1] = s[j]; j--; }
    s[j + 1] = key;
  }

  uint64_t sum = 0;
  for (uint8_t i = TRIM; i < N - TRIM; i++) sum += s[i];
  const uint32_t raw = (uint32_t)(sum / KEPT);

  uint32_t half;
  if (adcCalibrated)
    half = esp_adc_cal_raw_to_voltage(raw, &adcChars);
  else
    half = (uint32_t)((raw * 3300UL) / 4095UL);

  return (int32_t)(half * (uint32_t)VBAT_ADC_DIVIDER_RATIO);
}

// -------------------------------------------------------------------------------------------------
// readVbatMvBestEffort() – ADC prioritaire ; fallback AT+CBC après démarrage modem.
// Le fallback évite un batch vide lorsque GPIO35 n'est pas câblé comme attendu sur la carte utilisée.
// -------------------------------------------------------------------------------------------------
static int32_t readVbatMvBestEffort(const char *stage)
{
  const int32_t adcMv = readVbatMv();
  if (isVbatPlausible(adcMv)) return adcMv;

  // Au boot, le modem n'est pas encore initialisé : ne pas tenter AT+CBC.
  if (stage && strcmp(stage, "boot") == 0) {
    LOGF(1, "Vbat ADC hors plage (%s): %d mV – sample ignoré\n", stage, (int)adcMv);
    return -1;
  }

  int8_t cs = 0;
  int8_t pct = 0;
  int16_t modemMv = 0;
  modem.getBattStats(cs, pct, modemMv);
  if (isVbatPlausible(modemMv)) {
    LOGF(1, "Vbat ADC hors plage (%s): %d mV – fallback modem=%d mV\n",
         stage ? stage : "?", (int)adcMv, (int)modemMv);
    return (int32_t)modemMv;
  }

  LOGF(1, "Vbat hors plage (%s): ADC=%d mV, modem=%d mV – sample ignoré\n",
       stage ? stage : "?", (int)adcMv, (int)modemMv);
  return -1;
}

// -------------------------------------------------------------------------------------------------
// recordBatterySample() – stocke localement un échantillon mV, sans publication réseau.
// -------------------------------------------------------------------------------------------------
void recordBatterySample(const char *stage, const char *datetimeValue)
{
  const int32_t vbatMv = readVbatMvBestEffort(stage);
  if (!isVbatPlausible(vbatMv)) return;

  if (!rtcVbatEmaInit) {
    rtcVbatEmaMv   = vbatMv;
    rtcVbatEmaInit = true;
  } else if (vbatMv >= rtcVbatEmaMv) {
    rtcVbatEmaMv = rtcVbatEmaMv + (vbatMv - rtcVbatEmaMv) / 4;
  } else {
    rtcVbatEmaMv = rtcVbatEmaMv + (vbatMv - rtcVbatEmaMv) / 8;
  }
  if (vbatMv < rtcVbatMinMv) rtcVbatMinMv = vbatMv;

  uint8_t idx = batterySampleCount;
  if (idx >= PH_BATTERY_BATCH_MAX_RECORDS) {
    idx = PH_BATTERY_BATCH_MAX_RECORDS - 1;
  } else {
    batterySampleCount++;
  }

  BatterySample &b = batterySamples[idx];
  b.valid = true;
  strncpy(b.stage, stage ? stage : "sample", sizeof(b.stage) - 1);
  b.stage[sizeof(b.stage) - 1] = '\0';

  if (datetimeValue && datetimeValue[0] != '\0') {
    strncpy(b.datetime, datetimeValue, sizeof(b.datetime) - 1);
  } else if (lastFix.valid && lastFix.datetime[0] != '\0') {
    strncpy(b.datetime, lastFix.datetime, sizeof(b.datetime) - 1);
  } else {
    strncpy(b.datetime, "unknown", sizeof(b.datetime) - 1);
  }
  b.datetime[sizeof(b.datetime) - 1] = '\0';
  b.tMs = millis();
  b.vbatMv = vbatMv;

  LOGF(1, "[BAT] sample %s: %d mV at %lu ms\n",
       b.stage, (int)b.vbatMv, (unsigned long)b.tMs);
}

// -------------------------------------------------------------------------------------------------
// publishBatteryIfDue() – désormais échantillonnage local périodique, pas publication MQTT.
// -------------------------------------------------------------------------------------------------
void publishBatteryIfDue(const char *datetimeValue)
{
  if (millis() - timeStamp <= (unsigned long)BAT_PERIOD * 1000UL) return;
  timeStamp = millis();
  recordBatterySample("mid", datetimeValue);
}

// -------------------------------------------------------------------------------------------------
// publishPreSleepVbat() – mesure locale avant modem_off() = sous charge maximale du cycle.
// -------------------------------------------------------------------------------------------------
void publishPreSleepVbat()
{
  recordBatterySample("pre_sleep",
      (lastFix.valid && lastFix.datetime[0] != '\0') ? lastFix.datetime : nullptr);
}
