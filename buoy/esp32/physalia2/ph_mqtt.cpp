// =============================================================================
// ph_mqtt.cpp – Publication MQTT compatible Node-RED/PostgreSQL
// Geo batch compact v2: fields[] + samples[] arrays, champs inutiles supprimés.
// Fallback robuste: compact x5 -> batch objet x2 -> unitaire.
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

// Champs conservés pour la base/Grafana.
// Champs supprimés volontairement pour réduire la taille MQTT:
// alt_m, undulation_m, pdop, hdop, gdop, vel_n_ms, vel_e_ms, spd_ms,
// heading_type, heading_deg, pitch_deg.
static void addFixFieldsToJson(JsonObject obj, const PvtslnData &fix)
{
  obj["datetime"] = fix.datetime;
  obj["lat"] = fix.lat;
  obj["lon"] = fix.lon;
  obj["elv_m"] = fix.ellipsoid;
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
}

static void addCompactFields(JsonArray fields)
{
  fields.add("seq");
  fields.add("datetime");
  fields.add("lat");
  fields.add("lon");
  fields.add("elv_m");
  fields.add("fix");
  fields.add("fix_type");
  fields.add("car");
  fields.add("hacc_mm");
  fields.add("vacc_mm");
  fields.add("latstd_m");
  fields.add("lonstd_m");
  fields.add("hgtstd_m");
  fields.add("numsv");
  fields.add("sv_tracked");
  fields.add("diffage_s");
}

static void addFixFieldsToCompactRow(JsonArray row, uint16_t seq, const PvtslnData &fix)
{
  row.add(seq);
  row.add(fix.datetime);
  row.add(fix.lat);
  row.add(fix.lon);
  row.add(fix.ellipsoid);
  row.add(mapPositionTypeToFix(fix.bestposType));
  row.add(fix.bestposType);
  row.add(mapPositionTypeToCarrier(fix.bestposType));
  row.add((uint32_t)(max(fix.latStd, fix.lonStd) * 1000.0f));
  row.add((uint32_t)(fix.hgtStd * 1000.0f));
  row.add(fix.latStd);
  row.add(fix.lonStd);
  row.add(fix.hgtStd);
  row.add(fix.bestposSolnSvs);
  row.add(fix.bestposTrackedSvs);
  row.add(fix.diffAge);
}

void publishFix(const PvtslnData &fix)
{
  if (!mqtt.connected() || !fix.valid || fix.datetime[0] == '\0') return;
  StaticJsonDocument<768> doc;
  doc["capteur"] = matUuid;
  addFixFieldsToJson(doc.as<JsonObject>(), fix);
  String msg;
  serializeJson(doc, msg);
  publishWithRetry(mqtttopic, msg, "geo");
}

static bool publishGeoSingleRecord(uint16_t index, const char *status)
{
  const PvtslnData &fix = fixBatch[index];
  if (!fix.valid || fix.datetime[0] == '\0') return true;

  StaticJsonDocument<896> doc;
  doc["capteur"] = matUuid;
  doc["cycle_id"] = cycleId;
  doc["cycle_status"] = status ? status : "unknown";
  doc["seq"] = index + 1;
  doc["seq_total"] = fixBatchCount;
  doc["dropped"] = fixBatchDropped;
  addFixFieldsToJson(doc.as<JsonObject>(), fix);

  String msg;
  serializeJson(doc, msg);
  return publishWithRetry(mqtttopic, msg, "geo");
}

// Fallback conserve le format geo_batch objet, mais avec les champs inutiles supprimés.
static bool publishGeoBatchObjectRecords(uint16_t startIndex, uint16_t endIndex, uint16_t chunkNumber, uint16_t chunkTotal, const char *status)
{
  StaticJsonDocument<2048> doc;
  doc["capteur"] = matUuid;
  doc["type"] = "geo_batch";
  doc["cycle_id"] = cycleId;
  doc["cycle_status"] = status ? status : "unknown";
  doc["chunk"] = chunkNumber;
  doc["chunk_total"] = chunkTotal;
  doc["seq_start"] = startIndex + 1;
  doc["seq_end"] = endIndex;
  doc["seq_total"] = fixBatchCount;
  doc["dropped"] = fixBatchDropped;

  JsonArray samples = doc.createNestedArray("samples");
  uint16_t count = 0;
  for (uint16_t i = startIndex; i < endIndex; i++) {
    const PvtslnData &fix = fixBatch[i];
    if (!fix.valid || fix.datetime[0] == '\0') continue;
    JsonObject sample = samples.createNestedObject();
    sample["seq"] = i + 1;
    addFixFieldsToJson(sample, fix);
    count++;
  }
  doc["count"] = count;
  if (count == 0) return true;

  String msg;
  serializeJson(doc, msg);

  if (publishWithRetry(mqtttopic, msg, "geo_batch_fallback_x2")) return true;

  LOGF(1, "MQTT geo_batch object fallback to unit records for seq %u-%u\n", (unsigned)(startIndex + 1), (unsigned)endIndex);
  bool ok = true;
  for (uint16_t i = startIndex; i < endIndex; i++) {
    ok = publishGeoSingleRecord(i, status) && ok;
  }
  return ok;
}

static bool publishGeoBatchObjectFallbackX2(uint16_t startIndex, uint16_t endIndex, const char *status)
{
  bool ok = true;
  const uint16_t fallbackChunkSize = 2;
  const uint16_t recordCount = endIndex - startIndex;
  const uint16_t chunkTotal = (recordCount + fallbackChunkSize - 1) / fallbackChunkSize;
  uint16_t chunkNumber = 1;

  for (uint16_t start = startIndex; start < endIndex; start += fallbackChunkSize, chunkNumber++) {
    const uint16_t end = ((uint16_t)(start + fallbackChunkSize) > endIndex) ? endIndex : (uint16_t)(start + fallbackChunkSize);
    ok = publishGeoBatchObjectRecords(start, end, chunkNumber, chunkTotal, status) && ok;
  }
  return ok;
}

static bool publishGeoBatchCompactRecords(uint16_t startIndex, uint16_t endIndex, uint16_t chunkNumber, uint16_t chunkTotal, const char *status)
{
  StaticJsonDocument<3072> doc;
  doc["capteur"] = matUuid;
  doc["type"] = "geo_batch_v2";
  doc["cycle_id"] = cycleId;
  doc["cycle_status"] = status ? status : "unknown";
  doc["chunk"] = chunkNumber;
  doc["chunk_total"] = chunkTotal;
  doc["seq_start"] = startIndex + 1;
  doc["seq_end"] = endIndex;
  doc["seq_total"] = fixBatchCount;
  doc["dropped"] = fixBatchDropped;

  JsonArray fields = doc.createNestedArray("fields");
  addCompactFields(fields);

  JsonArray samples = doc.createNestedArray("samples");
  uint16_t count = 0;
  for (uint16_t i = startIndex; i < endIndex; i++) {
    const PvtslnData &fix = fixBatch[i];
    if (!fix.valid || fix.datetime[0] == '\0') continue;
    JsonArray row = samples.createNestedArray();
    addFixFieldsToCompactRow(row, i + 1, fix);
    count++;
  }
  doc["count"] = count;
  if (count == 0) return true;

  String msg;
  serializeJson(doc, msg);

  if (publishWithRetry(mqtttopic, msg, "geo_batch_v2")) return true;

  LOGF(1, "MQTT geo_batch_v2 fallback to object x2 for seq %u-%u\n", (unsigned)(startIndex + 1), (unsigned)endIndex);
  return publishGeoBatchObjectFallbackX2(startIndex, endIndex, status);
}

static bool publishGeoRecords(const char *status)
{
  if (fixBatchCount == 0) {
    LOGLN(1, "No geo sample to publish");
    return true;
  }

  bool ok = true;
  const uint16_t chunkSize = (GEO_BATCH_CHUNK_RECORDS < 1) ? 1 : GEO_BATCH_CHUNK_RECORDS;
  const uint16_t chunkTotal = (fixBatchCount + chunkSize - 1) / chunkSize;
  uint16_t chunkNumber = 1;

  for (uint16_t start = 0; start < fixBatchCount; start += chunkSize, chunkNumber++) {
    const uint16_t end = ((uint16_t)(start + chunkSize) > fixBatchCount) ? fixBatchCount : (uint16_t)(start + chunkSize);
    if (chunkSize == 1) {
      ok = publishGeoSingleRecord(start, status) && ok;
    } else {
      ok = publishGeoBatchCompactRecords(start, end, chunkNumber, chunkTotal, status) && ok;
    }
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
  doc["rtk_window_start_ms"] = rtkFixStart_ms ? (long)(rtkFixStart_ms - bootStarted_ms) : -1;
  doc["rtk_window_duration_ms"] = rtkFixStart_ms ? (long)RTK_ACQUISITION_PERIOD * 1000L : -1;
  doc["rtk_window_end_ms"] = rtkFixStart_ms ? (long)(rtkFixStart_ms - bootStarted_ms + (unsigned long)RTK_ACQUISITION_PERIOD * 1000UL) : -1;
  doc["gga_sent"] = ggaSentToCaster;
  doc["mqtt_buffer_size"] = mqtt.getBufferSize();
  doc["geo_batch_format"] = "geo_batch_v2";
  doc["geo_chunk_records"] = GEO_BATCH_CHUNK_RECORDS;
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
