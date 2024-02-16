#include "DataFormat.h"
#include "config.h"

DataFormat::DataFormat(UBX_NAV_PVT_data_t *ubx){
    this->ubxDataStruct = ubx;
}
DataFormat::~DataFormat(){}

StaticJsonDocument<256> DataFormat::getDocJson(){
// create an object
  StaticJsonDocument<256> docJson;
  JsonObject object = docJson.to<JsonObject>();

  //doc["capteur"] = matUuid + WiFi.macAddress()+"'";//matUuid; // Print capteur uuid
  docJson["capteur"] = matUuid; // Print capteur uuid

  uint16_t y = this->ubxDataStruct->year; // Print the year
  uint8_t mo = this->ubxDataStruct->month; // Print the year
  String mo1;
  if (mo < 10) {mo1 = "0"+ String(mo);} else { mo1 = String(mo);};
  uint8_t d = this->ubxDataStruct->day; // Print the year
  String d1;
    if (d < 10) {d1 = "0"+ String(d);} else { d1 = String(d);};
  uint8_t h = this->ubxDataStruct->hour; // Print the hours
  String h1;
    if (h < 10) {h1 = "0"+ String(h);} else { h1 = String(h);};
  uint8_t m = this->ubxDataStruct->min; // Print the minutes
  String m1;
    if (m < 10) {m1 = "0"+ String(m);} else { m1 = String(m);};
  uint8_t s = this->ubxDataStruct->sec; // Print the seconds
  String s1;
    if (s < 10) {s1 = "0"+ String(s);} else { s1 = String(s);};
  unsigned long millisecs = this->ubxDataStruct->iTOW % 1000; // Print the milliseconds
  String a = ":";
  String b = "-";
  String date1 = y+b+mo1+b+d1+" ";
  String time1 = h1 +a+ m1 +a+ s1 + "." + millisecs;
  object["datetime"] = "'"+date1+time1+"'"; // print date time for postgresql data injection.

  double latitude = this->ubxDataStruct->lat; // Print the latitude
  docJson["lat"] = latitude / 10000000.0;

  double longitude = this->ubxDataStruct->lon; // Print the longitude
  docJson["lon"] = longitude / 10000000.0;

  double elevation = this->ubxDataStruct->height; // Print the height above mean sea level
  docJson["elv_m"] = elevation / 1000.0;

  double altitude = this->ubxDataStruct->hMSL; // Print the height above mean sea level
  docJson["alt_m"] = altitude / 1000.0;

  uint8_t fixType = this->ubxDataStruct->fixType; // Print the fix type
  // 0 none/1 Dead reckoning/2 2d/3 3d/4 GNSS + Dead reckoning/ 5 time only
  docJson["fix"] = fixType;

  uint8_t carrSoln = this->ubxDataStruct->flags.bits.carrSoln; // Print the carrier solution
  // 0 none/1 floating/ 2 Fixed
  docJson["car"] = carrSoln;

  uint32_t hAcc = this->ubxDataStruct->hAcc; // Print the horizontal accuracy estimate
  docJson["hacc_mm"] = hAcc;

  uint32_t vAcc = this->ubxDataStruct->vAcc; // Print the vertical accuracy estimate
  docJson["vacc_mm"] = vAcc;

  uint8_t numSV = this->ubxDataStruct->numSV; // Print tle number of SVs used in nav solution
  docJson["numsv"] = numSV;
  
  return docJson;
}