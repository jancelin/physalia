// =============================================================================
// ph_mqtt.cpp – Publication MQTT compatible Node-RED/PostgreSQL
// =============================================================================
#include "ph_globals.h"

int mapPositionTypeToFix(const String &t)
{
  if (t == "NONE") return 0;
  if (t == "PSRDIFF" || t == "SBAS") return 2;
  if (t == "SINGLE") return 3;
  if (t.indexOf("FLOAT") >= 0) return 4;
  if (t.indexOf("INT") >= 0) return 5;
  return 3;
}

int mapPositionTypeToCarrier(const String &t)
{
  if (t.indexOf("FLOAT") >= 0) return 1;
  if (t.indexOf("INT") >= 0) return 2;
  return 0;
}

void feedTaskWatchdog();
void resetFailureCycleCounter(const char *reason);

boolean reconnect()
{
  if (taskWdtEnabled) (void)esp_task_wdt_reset();
  if (mqtt.connected()) return true;
  if (mqtt.connect(matUuid, mqttUser, mqttPassword)) {
    LOGLN(1, "MQTT connected");
    return true;
  }
  LOGF(1, "MQTT connect failed, state=%d\n", mqtt.state());
  return false;
}

static bool ensureMqttConnected()
{
  mqtt.setBufferSize(MQTT_BUFFER_SIZE);
  if (mqtt.connected()) return true;
  const unsigned long start = millis();
  while (!mqtt.connected() && millis() - start < (unsigned long)ACQUISION_PERIOD_MQTT) {
    feedTaskWatchdog();
    if (reconnect()) break;
    delay(750);
  }
  if (!mqtt.connected()) {
    LOGF(1, "MQTT unavailable after acquisition, state=%d\n", mqtt.state());
    return false;
  }
  return true;
}

static bool publishWithRetry(const char *topic, const String &msg, const char *label)
{
  if (!mqtt.connected()) return false;
  for (uint8_t attempt = 1; attempt <= 3; attempt++) {
    feedTaskWatchdog();
    mqtt.loop();
    if (mqtt.publish(topic, msg.c_str())) {
      LOGF(1, "MQTT %s publish OK (%u bytes, attempt %u)\n", label ? label : "payload", (unsigned)msg.length(), (unsigned)attempt);
      delay(15);
      mqtt.loop();
      return true;
    }
    LOGF(1, "MQTT %s publish failed, state=%d, bytes=%u, attempt=%u\n", label ? label : "payload", mqtt.state(), (unsigned)msg.length(), (unsigned)attempt);
    delay(250);
    if (!mqtt.connected()) ensureMqttConnected();
  }
  return false;
}

static void addFixFieldsToJson(JsonObject obj, const PvtslnData &fix)
{
  obj["datetime"] = fix.datetime;
  obj["lat"] = fix.lat;
  obj["lon"] = fix.lon;
  obj["elv_m"] = fix.ellipsoid;
  obj["alt_m"] = fix.altMSL;
  obj["undulation_m"] = fix.undulation;
  obj["fix"] = mapPositionTypeToFix(fix.bestposType);
  obj["fix_type"] = fix.bestposType;
  obj["car"] = mapPositionTypeToCarrier(fix.bestposType);
  obj["hacc_mm"] = (uint32_t)(max(fix.latStd, fix.lonStd) * 1000.0f);
  obj["vacc_mm"] = (uint32_t)(fix.hgtStd * 1000.0f);
  obj["latstd_m"] = fix.latStd;
  obj["lonstd_m"] = fix.lonStd;
  obj["hgtstd_m"] = fix.hgtStd;
  obj["numsv"] = fix.bestposSolnSvs;
  obj["sv_tracked"] = fix.bestposTrackedSvs;
  obj["diffage_s"] = fix.diffAge;
  obj["pdop"] = fix.pdop;
  obj["hdop"] = fix.hdop;
  obj["gdop"] = fix.gdop;
  obj["vel_n_ms"] = fix.velNorth;
  obj["vel_e_ms"] = fix.velEast;
  obj["spd_ms"] = fix.velGround;
  obj["heading_type"] = fix.headingType;
  obj["heading_deg"] = fix.headingDeg;
  obj["pitch_deg"] = fix.headingPitch;
}

void publishFix(const PvtslnData &fix)
{
  if (!mqtt.connected() || !fix.valid || fix.datetime[0] == '\0') return;
  StaticJsonDocument<1024> doc;
  doc["capteur"] = matUuid;
  addFixFieldsToJson(doc.as<JsonObject>(), fix);
  String msg;
  serializeJson(doc, msg);
  publishWithRetry(mqtttopic, msg, "geo");
}

static bool publishGeoRecords(const char *status)
{
  if (fixBatchCount == 0) {
    LOGLN(1, "No geo sample to publish");
    return true;
  }
  bool ok = true;
  for (uint16_t i = 0; i < fixBatchCount; i++) {
    const PvtslnData &fix = fixBatch[i];
    if (!fix.valid || fix.datetime[0] == '\0') continue;
    StaticJsonDocument<1152> doc;
    doc["capteur"] = matUuid;
    doc["cycle_id"] = cycleId;
    doc["cycle_status"] = status ? status : "unknown";
    doc["seq"] = i + 1;
    doc["seq_total"] = fixBatchCount;
    doc["dropped"] = fixBatchDropped;
    addFixFieldsToJson(doc.as<JsonObject>(), fix);
    String msg;
    serializeJson(doc, msg);
    ok = publishWithRetry(mqtttopic, msg, "geo") && ok;
  }
  return ok;
}

static bool publishBatteryRecords()
{
  bool ok = true;
  uint8_t published = 0;
  const char *fallbackDt = (lastFix.valid && lastFix.datetime[0] != '\0') ? lastFix.datetime : "1970-01-01 00:00:00.000";
  for (uint8_t i = 0; i < batterySampleCount; i++) {
    if (!batterySamples[i].valid) continue;
    const char *dt = batterySamples[i].datetime;
    if (!dt || dt[0] == '\0' || strcmp(dt, "unknown") == 0) dt = fallbackDt;
    StaticJsonDocument<384> doc;
    doc["capteur"] = matUuid;
    doc["datetime"] = dt;
    doc["stage"] = batterySamples[i].stage;
    doc["t_ms"] = batterySamples[i].tMs;
    doc["vbat_mv"] = batterySamples[i].vbatMv;
    String msg;
    serializeJson(doc, msg);
    ok = publishWithRetry(mqttbat, msg, "battery") && ok;
    published++;
  }
  if (published == 0) LOGLN(1, "No valid battery sample to publish");
  return ok;
}

static bool publishCycleStatus(const char *status)
{
  StaticJsonDocument<768> doc;
  const unsigned long now = millis();
  const char *dt = (lastFix.valid && lastFix.datetime[0] != '\0') ? lastFix.datetime : "1970-01-01 00:00:00.000";
  doc["capteur"] = matUuid;
  doc["datetime"] = dt;
  doc["type"] = "cycle_status";
  doc["cycle_id"] = cycleId;
  doc["status"] = status ? status : "unknown";
  doc["uptime_ms"] = now - bootStarted_ms;
  doc["fix_count"] = fixBatchCount;
  doc["fix_dropped"] = fixBatchDropped;
  doc["first_gga_ms"] = firstGga_ms ? (long)(firstGga_ms - bootStarted_ms) : -1;
  doc["ntrip_connect_ms"] = ntripConnected_ms ? (long)(ntripConnected_ms - bootStarted_ms) : -1;
  doc["first_rtcm_ms"] = firstRtcm_ms ? (long)(firstRtcm_ms - bootStarted_ms) : -1;
  doc["first_float_ms"] = firstFloat_ms ? (long)(firstFloat_ms - bootStarted_ms) : -1;
  doc["first_fix_ms"] = firstFix_ms ? (long)(firstFix_ms - bootStarted_ms) : -1;
  doc["rtk_window_ms"] = rtkFixStart_ms ? (long)(now - rtkFixStart_ms) : -1;
  doc["gga_sent"] = ggaSentToCaster;
  doc["mqtt_buffer_size"] = mqtt.getBufferSize();
  String msg;
  serializeJson(doc, msg);
  return publishWithRetry(mqttstatus, msg, "cycle_status");
}

bool publishCycleAndStatus(const char *status)
{
  if (cycleClosed) return true;
  cycleClosed = true;
  if (!ensureMqttConnected()) {
    LOGLN(1, "Cycle data kept only in RAM; MQTT failed before deep-sleep.");
    return false;
  }
  bool ok = true;
  ok = publishGeoRecords(status) && ok;
  ok = publishBatteryRecords() && ok;
  ok = publishCycleStatus(status) && ok;
  mqtt.loop();
  delay(100);
  mqtt.loop();
  if (ok) {
    resetFailureCycleCounter("cycle published");
    failureSleepCount = 0;
  }
  return ok;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  LOGBLOCK(2) {
    Serial.print("MQTT message on: ");
    Serial.println(topic);
    for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
    Serial.println();
  }
}
