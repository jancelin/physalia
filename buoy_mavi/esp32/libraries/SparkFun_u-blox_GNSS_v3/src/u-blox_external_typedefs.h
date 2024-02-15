/*
  An Arduino Library which allows you to communicate seamlessly with u-blox GNSS modules using the Configuration Interface

  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/15136
  https://www.sparkfun.com/products/16481
  https://www.sparkfun.com/products/16344
  https://www.sparkfun.com/products/18037
  https://www.sparkfun.com/products/18719
  https://www.sparkfun.com/products/18774
  https://www.sparkfun.com/products/19663
  https://www.sparkfun.com/products/17722

  Original version by Nathan Seidle @ SparkFun Electronics, September 6th, 2018
  v2.0 rework by Paul Clark @ SparkFun Electronics, December 31st, 2020
  v3.0 rework by Paul Clark @ SparkFun Electronics, December 8th, 2022

  https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3

  This library is an updated version of the popular SparkFun u-blox GNSS Arduino Library.
  v3 uses the u-blox Configuration Interface (VALSET and VALGET) to:
  detect the module (during begin); configure message intervals; configure the base location; etc..

  This version of the library will not work with older GNSS modules.
  It is specifically written for newer modules like the ZED-F9P, ZED-F9R and MAX-M10S.
  For older modules, please use v2 of the library: https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.19

  SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
  The MIT License (MIT)
  Copyright (c) 2018 SparkFun Electronics
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
  do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial
  portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#define kUBLOXGNSSDefaultAddress 0x42 // Default 7-bit unshifted address of the ublox 6/7/8/M8/F9 series

// A default of 250ms for maxWait seems fine for I2C but is not enough for SerialUSB.
// If you know you are only going to be using I2C / Qwiic communication, you can
// safely reduce kUBLOXGNSSDefaultMaxWait to 250.
#ifndef kUBLOXGNSSDefaultMaxWait // Let's allow the user to define their own value if they want to
#define kUBLOXGNSSDefaultMaxWait 1100
#endif

// Global Status Returns
typedef enum
{
  SFE_UBLOX_STATUS_SUCCESS,
  SFE_UBLOX_STATUS_FAIL,
  SFE_UBLOX_STATUS_CRC_FAIL,
  SFE_UBLOX_STATUS_TIMEOUT,
  SFE_UBLOX_STATUS_COMMAND_NACK, // Indicates that the command was unrecognised, invalid or that the module is too busy to respond
  SFE_UBLOX_STATUS_OUT_OF_RANGE,
  SFE_UBLOX_STATUS_INVALID_ARG,
  SFE_UBLOX_STATUS_INVALID_OPERATION,
  SFE_UBLOX_STATUS_MEM_ERR,
  SFE_UBLOX_STATUS_HW_ERR,
  SFE_UBLOX_STATUS_DATA_SENT,     // This indicates that a 'set' was successful
  SFE_UBLOX_STATUS_DATA_RECEIVED, // This indicates that a 'get' (poll) was successful
  SFE_UBLOX_STATUS_I2C_COMM_FAILURE,
  SFE_UBLOX_STATUS_SPI_COMM_FAILURE,
  SFE_UBLOX_STATUS_DATA_OVERWRITTEN // This is an error - the data was valid but has been or _is being_ overwritten by another packet
} sfe_ublox_status_e;

// ubxPacket validity
typedef enum
{
  SFE_UBLOX_PACKET_VALIDITY_NOT_VALID,
  SFE_UBLOX_PACKET_VALIDITY_VALID,
  SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED,
  SFE_UBLOX_PACKET_NOTACKNOWLEDGED // This indicates that we received a NACK
} sfe_ublox_packet_validity_e;

// Identify which packet buffer is in use:
// packetCfg (or a custom packet), packetAck or packetBuf
// packetAuto is used to store expected "automatic" messages
typedef enum
{
  SFE_UBLOX_PACKET_PACKETCFG,
  SFE_UBLOX_PACKET_PACKETACK,
  SFE_UBLOX_PACKET_PACKETBUF,
  SFE_UBLOX_PACKET_PACKETAUTO
} sfe_ublox_packet_buffer_e;

// Define a struct to allow selective logging / processing of NMEA messages
// Set the individual bits to pass the NMEA messages to the file buffer and/or processNMEA
// Setting bits.all will pass all messages to the file buffer and processNMEA
typedef struct
{
  union
  {
    uint32_t all;
    struct
    {
      uint32_t all : 1;
      uint32_t UBX_NMEA_DTM : 1;
      uint32_t UBX_NMEA_GAQ : 1;
      uint32_t UBX_NMEA_GBQ : 1;
      uint32_t UBX_NMEA_GBS : 1;
      uint32_t UBX_NMEA_GGA : 1;
      uint32_t UBX_NMEA_GLL : 1;
      uint32_t UBX_NMEA_GLQ : 1;
      uint32_t UBX_NMEA_GNQ : 1;
      uint32_t UBX_NMEA_GNS : 1;
      uint32_t UBX_NMEA_GPQ : 1;
      uint32_t UBX_NMEA_GQQ : 1;
      uint32_t UBX_NMEA_GRS : 1;
      uint32_t UBX_NMEA_GSA : 1;
      uint32_t UBX_NMEA_GST : 1;
      uint32_t UBX_NMEA_GSV : 1;
      uint32_t UBX_NMEA_RLM : 1;
      uint32_t UBX_NMEA_RMC : 1;
      uint32_t UBX_NMEA_TXT : 1;
      uint32_t UBX_NMEA_VLW : 1;
      uint32_t UBX_NMEA_VTG : 1;
      uint32_t UBX_NMEA_ZDA : 1;
      uint32_t UBX_NMEA_THS : 1;
    } bits;
  };
} sfe_ublox_nmea_filtering_t;

// Define an enum to make it easy to enable/disable selected NMEA messages for logging / processing
typedef enum
{
  SFE_UBLOX_FILTER_NMEA_ALL = 0x00000001,
  SFE_UBLOX_FILTER_NMEA_DTM = 0x00000002,
  SFE_UBLOX_FILTER_NMEA_GAQ = 0x00000004,
  SFE_UBLOX_FILTER_NMEA_GBQ = 0x00000008,
  SFE_UBLOX_FILTER_NMEA_GBS = 0x00000010,
  SFE_UBLOX_FILTER_NMEA_GGA = 0x00000020,
  SFE_UBLOX_FILTER_NMEA_GLL = 0x00000040,
  SFE_UBLOX_FILTER_NMEA_GLQ = 0x00000080,
  SFE_UBLOX_FILTER_NMEA_GNQ = 0x00000100,
  SFE_UBLOX_FILTER_NMEA_GNS = 0x00000200,
  SFE_UBLOX_FILTER_NMEA_GPQ = 0x00000400,
  SFE_UBLOX_FILTER_NMEA_GQQ = 0x00000800,
  SFE_UBLOX_FILTER_NMEA_GRS = 0x00001000,
  SFE_UBLOX_FILTER_NMEA_GSA = 0x00002000,
  SFE_UBLOX_FILTER_NMEA_GST = 0x00004000,
  SFE_UBLOX_FILTER_NMEA_GSV = 0x00008000,
  SFE_UBLOX_FILTER_NMEA_RLM = 0x00010000,
  SFE_UBLOX_FILTER_NMEA_RMC = 0x00020000,
  SFE_UBLOX_FILTER_NMEA_TXT = 0x00040000,
  SFE_UBLOX_FILTER_NMEA_VLW = 0x00080000,
  SFE_UBLOX_FILTER_NMEA_VTG = 0x00100000,
  SFE_UBLOX_FILTER_NMEA_ZDA = 0x00200000,
  SFE_UBLOX_FILTER_NMEA_THS = 0x00400000,
} sfe_ublox_nmea_filtering_e;

// Define a struct to allow selective logging of RTCM messages
// Set the individual bits to pass the RTCM messages to the file buffer
// Setting bits.all will pass all messages to the file buffer
typedef struct
{
  union
  {
    uint32_t all;
    struct
    {
      uint32_t all : 1;
      uint32_t UBX_RTCM_TYPE1001 : 1;
      uint32_t UBX_RTCM_TYPE1002 : 1;
      uint32_t UBX_RTCM_TYPE1003 : 1;
      uint32_t UBX_RTCM_TYPE1004 : 1;
      uint32_t UBX_RTCM_TYPE1005 : 1;
      uint32_t UBX_RTCM_TYPE1006 : 1;
      uint32_t UBX_RTCM_TYPE1007 : 1;
      uint32_t UBX_RTCM_TYPE1009 : 1;
      uint32_t UBX_RTCM_TYPE1010 : 1;
      uint32_t UBX_RTCM_TYPE1011 : 1;
      uint32_t UBX_RTCM_TYPE1012 : 1;
      uint32_t UBX_RTCM_TYPE1033 : 1;
      uint32_t UBX_RTCM_TYPE1074 : 1;
      uint32_t UBX_RTCM_TYPE1075 : 1;
      uint32_t UBX_RTCM_TYPE1077 : 1;
      uint32_t UBX_RTCM_TYPE1084 : 1;
      uint32_t UBX_RTCM_TYPE1085 : 1;
      uint32_t UBX_RTCM_TYPE1087 : 1;
      uint32_t UBX_RTCM_TYPE1094 : 1;
      uint32_t UBX_RTCM_TYPE1095 : 1;
      uint32_t UBX_RTCM_TYPE1097 : 1;
      uint32_t UBX_RTCM_TYPE1124 : 1;
      uint32_t UBX_RTCM_TYPE1125 : 1;
      uint32_t UBX_RTCM_TYPE1127 : 1;
      uint32_t UBX_RTCM_TYPE1230 : 1;
      uint32_t UBX_RTCM_TYPE4072_0 : 1;
      uint32_t UBX_RTCM_TYPE4072_1 : 1;
    } bits;
  };
} sfe_ublox_rtcm_filtering_t;

// Define an enum to make it easy to enable/disable selected NMEA messages for logging / processing
typedef enum
{
  SFE_UBLOX_FILTER_RTCM_ALL = 0x00000001,
  SFE_UBLOX_FILTER_RTCM_TYPE1001 = 0x00000002,
  SFE_UBLOX_FILTER_RTCM_TYPE1002 = 0x00000004,
  SFE_UBLOX_FILTER_RTCM_TYPE1003 = 0x00000008,
  SFE_UBLOX_FILTER_RTCM_TYPE1004 = 0x00000010,
  SFE_UBLOX_FILTER_RTCM_TYPE1005 = 0x00000020,
  SFE_UBLOX_FILTER_RTCM_TYPE1006 = 0x00000040,
  SFE_UBLOX_FILTER_RTCM_TYPE1007 = 0x00000080,
  SFE_UBLOX_FILTER_RTCM_TYPE1009 = 0x00000100,
  SFE_UBLOX_FILTER_RTCM_TYPE1010 = 0x00000200,
  SFE_UBLOX_FILTER_RTCM_TYPE1011 = 0x00000400,
  SFE_UBLOX_FILTER_RTCM_TYPE1012 = 0x00000800,
  SFE_UBLOX_FILTER_RTCM_TYPE1033 = 0x00001000,
  SFE_UBLOX_FILTER_RTCM_TYPE1074 = 0x00002000,
  SFE_UBLOX_FILTER_RTCM_TYPE1075 = 0x00004000,
  SFE_UBLOX_FILTER_RTCM_TYPE1077 = 0x00008000,
  SFE_UBLOX_FILTER_RTCM_TYPE1084 = 0x00010000,
  SFE_UBLOX_FILTER_RTCM_TYPE1085 = 0x00020000,
  SFE_UBLOX_FILTER_RTCM_TYPE1087 = 0x00040000,
  SFE_UBLOX_FILTER_RTCM_TYPE1094 = 0x00080000,
  SFE_UBLOX_FILTER_RTCM_TYPE1095 = 0x00100000,
  SFE_UBLOX_FILTER_RTCM_TYPE1097 = 0x00200000,
  SFE_UBLOX_FILTER_RTCM_TYPE1124 = 0x00400000,
  SFE_UBLOX_FILTER_RTCM_TYPE1125 = 0x00800000,
  SFE_UBLOX_FILTER_RTCM_TYPE1127 = 0x01000000,
  SFE_UBLOX_FILTER_RTCM_TYPE1230 = 0x02000000,
  SFE_UBLOX_FILTER_RTCM_TYPE4072_0 = 0x04000000,
  SFE_UBLOX_FILTER_RTCM_TYPE4072_1 = 0x08000000,
} sfe_ublox_rtcm_filtering_e;

// Define a linked list which defines which UBX messages should be logged automatically
// - without needing to have and use the Auto methods
struct sfe_ublox_ubx_logging_list_t
{
  uint8_t UBX_CLASS;
  uint8_t UBX_ID;
  bool enable;
  sfe_ublox_ubx_logging_list_t *next;
};

//-=-=-=-=-

#ifndef MAX_PAYLOAD_SIZE
// v2.0: keep this for backwards-compatibility, but this is largely superseded by setPacketCfgPayloadSize
#define MAX_PAYLOAD_SIZE 276 // We need >=250 bytes for getProtocolVersion on the NEO-F10N
// #define MAX_PAYLOAD_SIZE 768 //Worst case: UBX_CFG_VALSET packet with 64 keyIDs each with 64 bit values
#endif

// Limit for SPI transactions
#define SFE_UBLOX_SPI_TRANSACTION_SIZE 32
// Default size of the SPI buffer - 8 larger than packetCfg payload
#define SFE_UBLOX_SPI_BUFFER_DEFAULT_SIZE (MAX_PAYLOAD_SIZE + 8)

// Default maximum NMEA byte count
// maxNMEAByteCount was set to 82: https://en.wikipedia.org/wiki/NMEA_0183#Message_structure
// but the u-blox HP (RTK) GGA messages are 88 bytes long
// The user can adjust maxNMEAByteCount by calling setMaxNMEAByteCount
// To be safe - use 100 like we do for the Auto NMEA messages
#define SFE_UBLOX_MAX_NMEA_BYTE_COUNT 100

//-=-=-=-=- UBX binary specific variables
struct ubxPacket
{
  uint8_t cls;
  uint8_t id;
  uint16_t len;          // Length of the payload. Does not include cls, id, or checksum bytes
  uint16_t counter;      // Keeps track of number of overall bytes received. Some responses are larger than 255 bytes.
  uint16_t startingSpot; // The counter value needed to go past before we begin recording into payload array
  uint8_t *payload;      // We will allocate RAM for the payload if/when needed.
  uint8_t checksumA;     // Given to us from module. Checked against the rolling calculated A/B checksums.
  uint8_t checksumB;
  sfe_ublox_packet_validity_e valid;           // Goes from NOT_DEFINED to VALID or NOT_VALID when checksum is checked
  sfe_ublox_packet_validity_e classAndIDmatch; // Goes from NOT_DEFINED to VALID or NOT_VALID when the Class and ID match the requestedClass and requestedID
};

// Struct to hold the results returned by getGeofenceState (returned by UBX-NAV-GEOFENCE)
typedef struct
{
  uint8_t status;    // Geofencing status: 0 - Geofencing not available or not reliable; 1 - Geofencing active
  uint8_t numFences; // Number of geofences
  uint8_t combState; // Combined (logical OR) state of all geofences: 0 - Unknown; 1 - Inside; 2 - Outside
  uint8_t states[4]; // Geofence states: 0 - Unknown; 1 - Inside; 2 - Outside
} geofenceState;

// Struct to hold the current geofence parameters
typedef struct
{
  uint8_t numFences; // Number of active geofences
  int32_t lats[4];   // Latitudes of geofences (in degrees * 10^-7)
  int32_t longs[4];  // Longitudes of geofences (in degrees * 10^-7)
  uint32_t rads[4];  // Radii of geofences (in m * 10^-2)
} geofenceParams_t;

// Struct to hold the module software version
#define firmwareTypeLen 7 // HPG, SPG, SPGL1L5, etc.
#define moduleNameMaxLen 13 // Allow for: 4-chars minus 4-chars minus 3-chars
typedef struct
{
  uint8_t protocolVersionLow; // Loaded from getModuleInfo()
  uint8_t protocolVersionHigh;
  uint8_t firmwareVersionLow;
  uint8_t firmwareVersionHigh;
  char firmwareType[firmwareTypeLen + 1]; // Include space for a NULL
  char moduleName[moduleNameMaxLen + 1]; // Include space for a NULL
  bool moduleQueried;
} moduleSWVersion_t;

const uint32_t SFE_UBLOX_DAYS_FROM_1970_TO_2020 = 18262; // Jan 1st 2020 Epoch = 1577836800 seconds
const uint16_t SFE_UBLOX_DAYS_SINCE_2020[80] =
    {
        0, 366, 731, 1096, 1461, 1827, 2192, 2557, 2922, 3288,
        3653, 4018, 4383, 4749, 5114, 5479, 5844, 6210, 6575, 6940,
        7305, 7671, 8036, 8401, 8766, 9132, 9497, 9862, 10227, 10593,
        10958, 11323, 11688, 12054, 12419, 12784, 13149, 13515, 13880, 14245,
        14610, 14976, 15341, 15706, 16071, 16437, 16802, 17167, 17532, 17898,
        18263, 18628, 18993, 19359, 19724, 20089, 20454, 20820, 21185, 21550,
        21915, 22281, 22646, 23011, 23376, 23742, 24107, 24472, 24837, 25203,
        25568, 25933, 26298, 26664, 27029, 27394, 27759, 28125, 28490, 28855};
const uint16_t SFE_UBLOX_DAYS_SINCE_MONTH[2][12] =
    {
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}, // Leap Year (Year % 4 == 0)
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334}  // Normal Year
};

const uint32_t SFE_UBLOX_JAN_1ST_2020_WEEK = 2086; // GPS Week Number for Jan 1st 2020
const uint32_t SFE_UBLOX_EPOCH_WEEK_2086 = 1577836800 - 259200; // Epoch for the start of GPS week 2086
const uint32_t SFE_UBLOX_SECS_PER_WEEK = 60 * 60 * 24 * 7; // Seconds per week

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// SPARTN CRC calculation
// Stolen from https://github.com/u-blox/ubxlib/blob/master/common/spartn/src/u_spartn_crc.c

typedef struct
{
  uint8_t messageType;
  uint16_t payloadLength;
  uint16_t EAF;
  uint8_t crcType;
  uint8_t frameCRC;
  uint8_t messageSubtype;
  uint16_t timeTagType;
  uint16_t authenticationIndicator;
  uint16_t embeddedApplicationLengthBytes;
} sfe_ublox_spartn_header_t;

const uint8_t sfe_ublox_u8Crc4Table[] = {
    0x00U, 0x0BU, 0x05U, 0x0EU, 0x0AU, 0x01U, 0x0FU, 0x04U,
    0x07U, 0x0CU, 0x02U, 0x09U, 0x0DU, 0x06U, 0x08U, 0x03U,
    0x0EU, 0x05U, 0x0BU, 0x00U, 0x04U, 0x0FU, 0x01U, 0x0AU,
    0x09U, 0x02U, 0x0CU, 0x07U, 0x03U, 0x08U, 0x06U, 0x0DU,
    0x0FU, 0x04U, 0x0AU, 0x01U, 0x05U, 0x0EU, 0x00U, 0x0BU,
    0x08U, 0x03U, 0x0DU, 0x06U, 0x02U, 0x09U, 0x07U, 0x0CU,
    0x01U, 0x0AU, 0x04U, 0x0FU, 0x0BU, 0x00U, 0x0EU, 0x05U,
    0x06U, 0x0DU, 0x03U, 0x08U, 0x0CU, 0x07U, 0x09U, 0x02U,
    0x0DU, 0x06U, 0x08U, 0x03U, 0x07U, 0x0CU, 0x02U, 0x09U,
    0x0AU, 0x01U, 0x0FU, 0x04U, 0x00U, 0x0BU, 0x05U, 0x0EU,
    0x03U, 0x08U, 0x06U, 0x0DU, 0x09U, 0x02U, 0x0CU, 0x07U,
    0x04U, 0x0FU, 0x01U, 0x0AU, 0x0EU, 0x05U, 0x0BU, 0x00U,
    0x02U, 0x09U, 0x07U, 0x0CU, 0x08U, 0x03U, 0x0DU, 0x06U,
    0x05U, 0x0EU, 0x00U, 0x0BU, 0x0FU, 0x04U, 0x0AU, 0x01U,
    0x0CU, 0x07U, 0x09U, 0x02U, 0x06U, 0x0DU, 0x03U, 0x08U,
    0x0BU, 0x00U, 0x0EU, 0x05U, 0x01U, 0x0AU, 0x04U, 0x0FU,
    0x09U, 0x02U, 0x0CU, 0x07U, 0x03U, 0x08U, 0x06U, 0x0DU,
    0x0EU, 0x05U, 0x0BU, 0x00U, 0x04U, 0x0FU, 0x01U, 0x0AU,
    0x07U, 0x0CU, 0x02U, 0x09U, 0x0DU, 0x06U, 0x08U, 0x03U,
    0x00U, 0x0BU, 0x05U, 0x0EU, 0x0AU, 0x01U, 0x0FU, 0x04U,
    0x06U, 0x0DU, 0x03U, 0x08U, 0x0CU, 0x07U, 0x09U, 0x02U,
    0x01U, 0x0AU, 0x04U, 0x0FU, 0x0BU, 0x00U, 0x0EU, 0x05U,
    0x08U, 0x03U, 0x0DU, 0x06U, 0x02U, 0x09U, 0x07U, 0x0CU,
    0x0FU, 0x04U, 0x0AU, 0x01U, 0x05U, 0x0EU, 0x00U, 0x0BU,
    0x04U, 0x0FU, 0x01U, 0x0AU, 0x0EU, 0x05U, 0x0BU, 0x00U,
    0x03U, 0x08U, 0x06U, 0x0DU, 0x09U, 0x02U, 0x0CU, 0x07U,
    0x0AU, 0x01U, 0x0FU, 0x04U, 0x00U, 0x0BU, 0x05U, 0x0EU,
    0x0DU, 0x06U, 0x08U, 0x03U, 0x07U, 0x0CU, 0x02U, 0x09U,
    0x0BU, 0x00U, 0x0EU, 0x05U, 0x01U, 0x0AU, 0x04U, 0x0FU,
    0x0CU, 0x07U, 0x09U, 0x02U, 0x06U, 0x0DU, 0x03U, 0x08U,
    0x05U, 0x0EU, 0x00U, 0x0BU, 0x0FU, 0x04U, 0x0AU, 0x01U,
    0x02U, 0x09U, 0x07U, 0x0CU, 0x08U, 0x03U, 0x0DU, 0x06U
};

const uint8_t sfe_ublox_u8Crc8Table[] = {
    0x00U, 0x07U, 0x0EU, 0x09U, 0x1CU, 0x1BU, 0x12U, 0x15U,
    0x38U, 0x3FU, 0x36U, 0x31U, 0x24U, 0x23U, 0x2AU, 0x2DU,
    0x70U, 0x77U, 0x7EU, 0x79U, 0x6CU, 0x6BU, 0x62U, 0x65U,
    0x48U, 0x4FU, 0x46U, 0x41U, 0x54U, 0x53U, 0x5AU, 0x5DU,
    0xE0U, 0xE7U, 0xEEU, 0xE9U, 0xFCU, 0xFBU, 0xF2U, 0xF5U,
    0xD8U, 0xDFU, 0xD6U, 0xD1U, 0xC4U, 0xC3U, 0xCAU, 0xCDU,
    0x90U, 0x97U, 0x9EU, 0x99U, 0x8CU, 0x8BU, 0x82U, 0x85U,
    0xA8U, 0xAFU, 0xA6U, 0xA1U, 0xB4U, 0xB3U, 0xBAU, 0xBDU,
    0xC7U, 0xC0U, 0xC9U, 0xCEU, 0xDBU, 0xDCU, 0xD5U, 0xD2U,
    0xFFU, 0xF8U, 0xF1U, 0xF6U, 0xE3U, 0xE4U, 0xEDU, 0xEAU,
    0xB7U, 0xB0U, 0xB9U, 0xBEU, 0xABU, 0xACU, 0xA5U, 0xA2U,
    0x8FU, 0x88U, 0x81U, 0x86U, 0x93U, 0x94U, 0x9DU, 0x9AU,
    0x27U, 0x20U, 0x29U, 0x2EU, 0x3BU, 0x3CU, 0x35U, 0x32U,
    0x1FU, 0x18U, 0x11U, 0x16U, 0x03U, 0x04U, 0x0DU, 0x0AU,
    0x57U, 0x50U, 0x59U, 0x5EU, 0x4BU, 0x4CU, 0x45U, 0x42U,
    0x6FU, 0x68U, 0x61U, 0x66U, 0x73U, 0x74U, 0x7DU, 0x7AU,
    0x89U, 0x8EU, 0x87U, 0x80U, 0x95U, 0x92U, 0x9BU, 0x9CU,
    0xB1U, 0xB6U, 0xBFU, 0xB8U, 0xADU, 0xAAU, 0xA3U, 0xA4U,
    0xF9U, 0xFEU, 0xF7U, 0xF0U, 0xE5U, 0xE2U, 0xEBU, 0xECU,
    0xC1U, 0xC6U, 0xCFU, 0xC8U, 0xDDU, 0xDAU, 0xD3U, 0xD4U,
    0x69U, 0x6EU, 0x67U, 0x60U, 0x75U, 0x72U, 0x7BU, 0x7CU,
    0x51U, 0x56U, 0x5FU, 0x58U, 0x4DU, 0x4AU, 0x43U, 0x44U,
    0x19U, 0x1EU, 0x17U, 0x10U, 0x05U, 0x02U, 0x0BU, 0x0CU,
    0x21U, 0x26U, 0x2FU, 0x28U, 0x3DU, 0x3AU, 0x33U, 0x34U,
    0x4EU, 0x49U, 0x40U, 0x47U, 0x52U, 0x55U, 0x5CU, 0x5BU,
    0x76U, 0x71U, 0x78U, 0x7FU, 0x6AU, 0x6DU, 0x64U, 0x63U,
    0x3EU, 0x39U, 0x30U, 0x37U, 0x22U, 0x25U, 0x2CU, 0x2BU,
    0x06U, 0x01U, 0x08U, 0x0FU, 0x1AU, 0x1DU, 0x14U, 0x13U,
    0xAEU, 0xA9U, 0xA0U, 0xA7U, 0xB2U, 0xB5U, 0xBCU, 0xBBU,
    0x96U, 0x91U, 0x98U, 0x9FU, 0x8AU, 0x8DU, 0x84U, 0x83U,
    0xDEU, 0xD9U, 0xD0U, 0xD7U, 0xC2U, 0xC5U, 0xCCU, 0xCBU,
    0xE6U, 0xE1U, 0xE8U, 0xEFU, 0xFAU, 0xFDU, 0xF4U, 0xF3U
};

const uint16_t sfe_ublox_u16Crc16Table[] = {
    0x0000U, 0x1021U, 0x2042U, 0x3063U, 0x4084U, 0x50A5U, 0x60C6U, 0x70E7U,
    0x8108U, 0x9129U, 0xA14AU, 0xB16BU, 0xC18CU, 0xD1ADU, 0xE1CEU, 0xF1EFU,
    0x1231U, 0x0210U, 0x3273U, 0x2252U, 0x52B5U, 0x4294U, 0x72F7U, 0x62D6U,
    0x9339U, 0x8318U, 0xB37BU, 0xA35AU, 0xD3BDU, 0xC39CU, 0xF3FFU, 0xE3DEU,
    0x2462U, 0x3443U, 0x0420U, 0x1401U, 0x64E6U, 0x74C7U, 0x44A4U, 0x5485U,
    0xA56AU, 0xB54BU, 0x8528U, 0x9509U, 0xE5EEU, 0xF5CFU, 0xC5ACU, 0xD58DU,
    0x3653U, 0x2672U, 0x1611U, 0x0630U, 0x76D7U, 0x66F6U, 0x5695U, 0x46B4U,
    0xB75BU, 0xA77AU, 0x9719U, 0x8738U, 0xF7DFU, 0xE7FEU, 0xD79DU, 0xC7BCU,
    0x48C4U, 0x58E5U, 0x6886U, 0x78A7U, 0x0840U, 0x1861U, 0x2802U, 0x3823U,
    0xC9CCU, 0xD9EDU, 0xE98EU, 0xF9AFU, 0x8948U, 0x9969U, 0xA90AU, 0xB92BU,
    0x5AF5U, 0x4AD4U, 0x7AB7U, 0x6A96U, 0x1A71U, 0x0A50U, 0x3A33U, 0x2A12U,
    0xDBFDU, 0xCBDCU, 0xFBBFU, 0xEB9EU, 0x9B79U, 0x8B58U, 0xBB3BU, 0xAB1AU,
    0x6CA6U, 0x7C87U, 0x4CE4U, 0x5CC5U, 0x2C22U, 0x3C03U, 0x0C60U, 0x1C41U,
    0xEDAEU, 0xFD8FU, 0xCDECU, 0xDDCDU, 0xAD2AU, 0xBD0BU, 0x8D68U, 0x9D49U,
    0x7E97U, 0x6EB6U, 0x5ED5U, 0x4EF4U, 0x3E13U, 0x2E32U, 0x1E51U, 0x0E70U,
    0xFF9FU, 0xEFBEU, 0xDFDDU, 0xCFFCU, 0xBF1BU, 0xAF3AU, 0x9F59U, 0x8F78U,
    0x9188U, 0x81A9U, 0xB1CAU, 0xA1EBU, 0xD10CU, 0xC12DU, 0xF14EU, 0xE16FU,
    0x1080U, 0x00A1U, 0x30C2U, 0x20E3U, 0x5004U, 0x4025U, 0x7046U, 0x6067U,
    0x83B9U, 0x9398U, 0xA3FBU, 0xB3DAU, 0xC33DU, 0xD31CU, 0xE37FU, 0xF35EU,
    0x02B1U, 0x1290U, 0x22F3U, 0x32D2U, 0x4235U, 0x5214U, 0x6277U, 0x7256U,
    0xB5EAU, 0xA5CBU, 0x95A8U, 0x8589U, 0xF56EU, 0xE54FU, 0xD52CU, 0xC50DU,
    0x34E2U, 0x24C3U, 0x14A0U, 0x0481U, 0x7466U, 0x6447U, 0x5424U, 0x4405U,
    0xA7DBU, 0xB7FAU, 0x8799U, 0x97B8U, 0xE75FU, 0xF77EU, 0xC71DU, 0xD73CU,
    0x26D3U, 0x36F2U, 0x0691U, 0x16B0U, 0x6657U, 0x7676U, 0x4615U, 0x5634U,
    0xD94CU, 0xC96DU, 0xF90EU, 0xE92FU, 0x99C8U, 0x89E9U, 0xB98AU, 0xA9ABU,
    0x5844U, 0x4865U, 0x7806U, 0x6827U, 0x18C0U, 0x08E1U, 0x3882U, 0x28A3U,
    0xCB7DU, 0xDB5CU, 0xEB3FU, 0xFB1EU, 0x8BF9U, 0x9BD8U, 0xABBBU, 0xBB9AU,
    0x4A75U, 0x5A54U, 0x6A37U, 0x7A16U, 0x0AF1U, 0x1AD0U, 0x2AB3U, 0x3A92U,
    0xFD2EU, 0xED0FU, 0xDD6CU, 0xCD4DU, 0xBDAAU, 0xAD8BU, 0x9DE8U, 0x8DC9U,
    0x7C26U, 0x6C07U, 0x5C64U, 0x4C45U, 0x3CA2U, 0x2C83U, 0x1CE0U, 0x0CC1U,
    0xEF1FU, 0xFF3EU, 0xCF5DU, 0xDF7CU, 0xAF9BU, 0xBFBAU, 0x8FD9U, 0x9FF8U,
    0x6E17U, 0x7E36U, 0x4E55U, 0x5E74U, 0x2E93U, 0x3EB2U, 0x0ED1U, 0x1EF0U
};

const uint32_t sfe_ublox_u32Crc24Table[] = {
    0x00000000U, 0x00864CFBU, 0x008AD50DU, 0x000C99F6U, 0x0093E6E1U, 0x0015AA1AU, 0x001933ECU, 0x009F7F17U,
    0x00A18139U, 0x0027CDC2U, 0x002B5434U, 0x00AD18CFU, 0x003267D8U, 0x00B42B23U, 0x00B8B2D5U, 0x003EFE2EU,
    0x00C54E89U, 0x00430272U, 0x004F9B84U, 0x00C9D77FU, 0x0056A868U, 0x00D0E493U, 0x00DC7D65U, 0x005A319EU,
    0x0064CFB0U, 0x00E2834BU, 0x00EE1ABDU, 0x00685646U, 0x00F72951U, 0x007165AAU, 0x007DFC5CU, 0x00FBB0A7U,
    0x000CD1E9U, 0x008A9D12U, 0x008604E4U, 0x0000481FU, 0x009F3708U, 0x00197BF3U, 0x0015E205U, 0x0093AEFEU,
    0x00AD50D0U, 0x002B1C2BU, 0x002785DDU, 0x00A1C926U, 0x003EB631U, 0x00B8FACAU, 0x00B4633CU, 0x00322FC7U,
    0x00C99F60U, 0x004FD39BU, 0x00434A6DU, 0x00C50696U, 0x005A7981U, 0x00DC357AU, 0x00D0AC8CU, 0x0056E077U,
    0x00681E59U, 0x00EE52A2U, 0x00E2CB54U, 0x006487AFU, 0x00FBF8B8U, 0x007DB443U, 0x00712DB5U, 0x00F7614EU,
    0x0019A3D2U, 0x009FEF29U, 0x009376DFU, 0x00153A24U, 0x008A4533U, 0x000C09C8U, 0x0000903EU, 0x0086DCC5U,
    0x00B822EBU, 0x003E6E10U, 0x0032F7E6U, 0x00B4BB1DU, 0x002BC40AU, 0x00AD88F1U, 0x00A11107U, 0x00275DFCU,
    0x00DCED5BU, 0x005AA1A0U, 0x00563856U, 0x00D074ADU, 0x004F0BBAU, 0x00C94741U, 0x00C5DEB7U, 0x0043924CU,
    0x007D6C62U, 0x00FB2099U, 0x00F7B96FU, 0x0071F594U, 0x00EE8A83U, 0x0068C678U, 0x00645F8EU, 0x00E21375U,
    0x0015723BU, 0x00933EC0U, 0x009FA736U, 0x0019EBCDU, 0x008694DAU, 0x0000D821U, 0x000C41D7U, 0x008A0D2CU,
    0x00B4F302U, 0x0032BFF9U, 0x003E260FU, 0x00B86AF4U, 0x002715E3U, 0x00A15918U, 0x00ADC0EEU, 0x002B8C15U,
    0x00D03CB2U, 0x00567049U, 0x005AE9BFU, 0x00DCA544U, 0x0043DA53U, 0x00C596A8U, 0x00C90F5EU, 0x004F43A5U,
    0x0071BD8BU, 0x00F7F170U, 0x00FB6886U, 0x007D247DU, 0x00E25B6AU, 0x00641791U, 0x00688E67U, 0x00EEC29CU,
    0x003347A4U, 0x00B50B5FU, 0x00B992A9U, 0x003FDE52U, 0x00A0A145U, 0x0026EDBEU, 0x002A7448U, 0x00AC38B3U,
    0x0092C69DU, 0x00148A66U, 0x00181390U, 0x009E5F6BU, 0x0001207CU, 0x00876C87U, 0x008BF571U, 0x000DB98AU,
    0x00F6092DU, 0x007045D6U, 0x007CDC20U, 0x00FA90DBU, 0x0065EFCCU, 0x00E3A337U, 0x00EF3AC1U, 0x0069763AU,
    0x00578814U, 0x00D1C4EFU, 0x00DD5D19U, 0x005B11E2U, 0x00C46EF5U, 0x0042220EU, 0x004EBBF8U, 0x00C8F703U,
    0x003F964DU, 0x00B9DAB6U, 0x00B54340U, 0x00330FBBU, 0x00AC70ACU, 0x002A3C57U, 0x0026A5A1U, 0x00A0E95AU,
    0x009E1774U, 0x00185B8FU, 0x0014C279U, 0x00928E82U, 0x000DF195U, 0x008BBD6EU, 0x00872498U, 0x00016863U,
    0x00FAD8C4U, 0x007C943FU, 0x00700DC9U, 0x00F64132U, 0x00693E25U, 0x00EF72DEU, 0x00E3EB28U, 0x0065A7D3U,
    0x005B59FDU, 0x00DD1506U, 0x00D18CF0U, 0x0057C00BU, 0x00C8BF1CU, 0x004EF3E7U, 0x00426A11U, 0x00C426EAU,
    0x002AE476U, 0x00ACA88DU, 0x00A0317BU, 0x00267D80U, 0x00B90297U, 0x003F4E6CU, 0x0033D79AU, 0x00B59B61U,
    0x008B654FU, 0x000D29B4U, 0x0001B042U, 0x0087FCB9U, 0x001883AEU, 0x009ECF55U, 0x009256A3U, 0x00141A58U,
    0x00EFAAFFU, 0x0069E604U, 0x00657FF2U, 0x00E33309U, 0x007C4C1EU, 0x00FA00E5U, 0x00F69913U, 0x0070D5E8U,
    0x004E2BC6U, 0x00C8673DU, 0x00C4FECBU, 0x0042B230U, 0x00DDCD27U, 0x005B81DCU, 0x0057182AU, 0x00D154D1U,
    0x0026359FU, 0x00A07964U, 0x00ACE092U, 0x002AAC69U, 0x00B5D37EU, 0x00339F85U, 0x003F0673U, 0x00B94A88U,
    0x0087B4A6U, 0x0001F85DU, 0x000D61ABU, 0x008B2D50U, 0x00145247U, 0x00921EBCU, 0x009E874AU, 0x0018CBB1U,
    0x00E37B16U, 0x006537EDU, 0x0069AE1BU, 0x00EFE2E0U, 0x00709DF7U, 0x00F6D10CU, 0x00FA48FAU, 0x007C0401U,
    0x0042FA2FU, 0x00C4B6D4U, 0x00C82F22U, 0x004E63D9U, 0x00D11CCEU, 0x00575035U, 0x005BC9C3U, 0x00DD8538U
};

const uint32_t sfe_ublox_u32Crc32Table[] = {
    0x00000000U, 0x04C11DB7U, 0x09823B6EU, 0x0D4326D9U, 0x130476DCU, 0x17C56B6BU, 0x1A864DB2U, 0x1E475005U,
    0x2608EDB8U, 0x22C9F00FU, 0x2F8AD6D6U, 0x2B4BCB61U, 0x350C9B64U, 0x31CD86D3U, 0x3C8EA00AU, 0x384FBDBDU,
    0x4C11DB70U, 0x48D0C6C7U, 0x4593E01EU, 0x4152FDA9U, 0x5F15ADACU, 0x5BD4B01BU, 0x569796C2U, 0x52568B75U,
    0x6A1936C8U, 0x6ED82B7FU, 0x639B0DA6U, 0x675A1011U, 0x791D4014U, 0x7DDC5DA3U, 0x709F7B7AU, 0x745E66CDU,
    0x9823B6E0U, 0x9CE2AB57U, 0x91A18D8EU, 0x95609039U, 0x8B27C03CU, 0x8FE6DD8BU, 0x82A5FB52U, 0x8664E6E5U,
    0xBE2B5B58U, 0xBAEA46EFU, 0xB7A96036U, 0xB3687D81U, 0xAD2F2D84U, 0xA9EE3033U, 0xA4AD16EAU, 0xA06C0B5DU,
    0xD4326D90U, 0xD0F37027U, 0xDDB056FEU, 0xD9714B49U, 0xC7361B4CU, 0xC3F706FBU, 0xCEB42022U, 0xCA753D95U,
    0xF23A8028U, 0xF6FB9D9FU, 0xFBB8BB46U, 0xFF79A6F1U, 0xE13EF6F4U, 0xE5FFEB43U, 0xE8BCCD9AU, 0xEC7DD02DU,
    0x34867077U, 0x30476DC0U, 0x3D044B19U, 0x39C556AEU, 0x278206ABU, 0x23431B1CU, 0x2E003DC5U, 0x2AC12072U,
    0x128E9DCFU, 0x164F8078U, 0x1B0CA6A1U, 0x1FCDBB16U, 0x018AEB13U, 0x054BF6A4U, 0x0808D07DU, 0x0CC9CDCAU,
    0x7897AB07U, 0x7C56B6B0U, 0x71159069U, 0x75D48DDEU, 0x6B93DDDBU, 0x6F52C06CU, 0x6211E6B5U, 0x66D0FB02U,
    0x5E9F46BFU, 0x5A5E5B08U, 0x571D7DD1U, 0x53DC6066U, 0x4D9B3063U, 0x495A2DD4U, 0x44190B0DU, 0x40D816BAU,
    0xACA5C697U, 0xA864DB20U, 0xA527FDF9U, 0xA1E6E04EU, 0xBFA1B04BU, 0xBB60ADFCU, 0xB6238B25U, 0xB2E29692U,
    0x8AAD2B2FU, 0x8E6C3698U, 0x832F1041U, 0x87EE0DF6U, 0x99A95DF3U, 0x9D684044U, 0x902B669DU, 0x94EA7B2AU,
    0xE0B41DE7U, 0xE4750050U, 0xE9362689U, 0xEDF73B3EU, 0xF3B06B3BU, 0xF771768CU, 0xFA325055U, 0xFEF34DE2U,
    0xC6BCF05FU, 0xC27DEDE8U, 0xCF3ECB31U, 0xCBFFD686U, 0xD5B88683U, 0xD1799B34U, 0xDC3ABDEDU, 0xD8FBA05AU,
    0x690CE0EEU, 0x6DCDFD59U, 0x608EDB80U, 0x644FC637U, 0x7A089632U, 0x7EC98B85U, 0x738AAD5CU, 0x774BB0EBU,
    0x4F040D56U, 0x4BC510E1U, 0x46863638U, 0x42472B8FU, 0x5C007B8AU, 0x58C1663DU, 0x558240E4U, 0x51435D53U,
    0x251D3B9EU, 0x21DC2629U, 0x2C9F00F0U, 0x285E1D47U, 0x36194D42U, 0x32D850F5U, 0x3F9B762CU, 0x3B5A6B9BU,
    0x0315D626U, 0x07D4CB91U, 0x0A97ED48U, 0x0E56F0FFU, 0x1011A0FAU, 0x14D0BD4DU, 0x19939B94U, 0x1D528623U,
    0xF12F560EU, 0xF5EE4BB9U, 0xF8AD6D60U, 0xFC6C70D7U, 0xE22B20D2U, 0xE6EA3D65U, 0xEBA91BBCU, 0xEF68060BU,
    0xD727BBB6U, 0xD3E6A601U, 0xDEA580D8U, 0xDA649D6FU, 0xC423CD6AU, 0xC0E2D0DDU, 0xCDA1F604U, 0xC960EBB3U,
    0xBD3E8D7EU, 0xB9FF90C9U, 0xB4BCB610U, 0xB07DABA7U, 0xAE3AFBA2U, 0xAAFBE615U, 0xA7B8C0CCU, 0xA379DD7BU,
    0x9B3660C6U, 0x9FF77D71U, 0x92B45BA8U, 0x9675461FU, 0x8832161AU, 0x8CF30BADU, 0x81B02D74U, 0x857130C3U,
    0x5D8A9099U, 0x594B8D2EU, 0x5408ABF7U, 0x50C9B640U, 0x4E8EE645U, 0x4A4FFBF2U, 0x470CDD2BU, 0x43CDC09CU,
    0x7B827D21U, 0x7F436096U, 0x7200464FU, 0x76C15BF8U, 0x68860BFDU, 0x6C47164AU, 0x61043093U, 0x65C52D24U,
    0x119B4BE9U, 0x155A565EU, 0x18197087U, 0x1CD86D30U, 0x029F3D35U, 0x065E2082U, 0x0B1D065BU, 0x0FDC1BECU,
    0x3793A651U, 0x3352BBE6U, 0x3E119D3FU, 0x3AD08088U, 0x2497D08DU, 0x2056CD3AU, 0x2D15EBE3U, 0x29D4F654U,
    0xC5A92679U, 0xC1683BCEU, 0xCC2B1D17U, 0xC8EA00A0U, 0xD6AD50A5U, 0xD26C4D12U, 0xDF2F6BCBU, 0xDBEE767CU,
    0xE3A1CBC1U, 0xE760D676U, 0xEA23F0AFU, 0xEEE2ED18U, 0xF0A5BD1DU, 0xF464A0AAU, 0xF9278673U, 0xFDE69BC4U,
    0x89B8FD09U, 0x8D79E0BEU, 0x803AC667U, 0x84FBDBD0U, 0x9ABC8BD5U, 0x9E7D9662U, 0x933EB0BBU, 0x97FFAD0CU,
    0xAFB010B1U, 0xAB710D06U, 0xA6322BDFU, 0xA2F33668U, 0xBCB4666DU, 0xB8757BDAU, 0xB5365D03U, 0xB1F740B4U
};

