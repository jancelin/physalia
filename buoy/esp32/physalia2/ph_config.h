#pragma once
// =============================================================================
// ph_config.h – Configuration matérielle, macros de log, dépendances globales
// =============================================================================
// Ce fichier est inclus en PREMIER dans tous les .cpp et dans le .ino.
// Les #define TinyGSM DOIVENT précéder #include <TinyGsmClient.h>.
// =============================================================================

#include <Arduino.h>
#include <SPI.h>
#include <esp_adc_cal.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <time.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Bluetooth (désactivé en production)
#if defined(ARDUINO_ARCH_ESP32) && __has_include("esp32-hal-bt.h")
  #include "esp32-hal-bt.h"
#endif

// -------------------------------------------------------------------------------------------------
// TinyGSM – doit être défini AVANT l'include
// -------------------------------------------------------------------------------------------------
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_DEBUG Serial
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#include <TinyGsmClient.h>

// -------------------------------------------------------------------------------------------------
// Log macros  (LOG_LEVEL défini dans secrets.h : 0=rien, 1=minimal, 2=complet)
// -------------------------------------------------------------------------------------------------
#define LOGLN(lvl, msg)      do { if (LOG_LEVEL >= (lvl)) Serial.println(msg); } while(0)
#define LOG(lvl, msg)        do { if (LOG_LEVEL >= (lvl)) Serial.print(msg);   } while(0)
#define LOGF(lvl, fmt, ...)  do { if (LOG_LEVEL >= (lvl)) Serial.printf(fmt, ##__VA_ARGS__); } while(0)
#define LOGBLOCK(lvl)        if (LOG_LEVEL >= (lvl))

// -------------------------------------------------------------------------------------------------
// Mapping matériel – T-SIM7600G-H R2
// -------------------------------------------------------------------------------------------------

// GNSS (UM980 via Pololu 2810)
#define PIN_GNSS_EN  23
#define UM980_TX_PIN 22   // ESP32 TX → UM980 RXD2
#define UM980_RX_PIN 21   // ESP32 RX ← UM980 TXD2
#define UM980_BAUD   460800

// SIM7600 UART + contrôle alimentation
#define UART_BAUD 115200
#define PIN_TX    27
#define PIN_RX    26
#define PWR_PIN   4       // PWRKEY SIM7600 (hold LOW >1.5s = power-off)
#define LED_PIN   12
#define POWER_PIN 25      // DC-DC boost enable
#define IND_PIN   36
#define SerialAT  Serial1 // UART modem SIM7600

// ADC batterie – T-SIM7600G-H R2 : 18650 → diviseur 100kΩ/100kΩ → GPIO35 (ADC1_CH7)
// GPIO4 = PWR_PIN numérique → ne PEUT PAS être ADC simultanément.
// AT+CBC lit la tension régulée du modem (~4.1V fixe), inutile pour surveiller la décharge.
#define ADC_PIN 35

// -------------------------------------------------------------------------------------------------
// Constantes runtime figées
// -------------------------------------------------------------------------------------------------
static const uint32_t TASK_WDT_TIMEOUT_S      = 60;
static const uint8_t  FAILURE_REBOOT_THRESHOLD = 3;
static const size_t   GNSS_LINE_BUFFER_SIZE    = 1024;
static const uint16_t PVT_QUEUE_DEPTH          = 240;  // 48 s à 5 Hz pendant setupGsm() bloquant
static const unsigned long MAX_HANGUP_MS       = 10000UL;
