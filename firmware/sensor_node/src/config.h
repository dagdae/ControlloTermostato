// Configurazione del nodo sensore a batteria (sul termosifone).
#pragma once

#define NODE_FW_VERSION "1.0.0"

// ---- Sensore ----
#define NODE_SENSOR_DS18B20 1  // sonda digitale (consigliata, nessuna taratura)
#define NODE_SENSOR_NTC 2      // termistore NTC 10k B3950 (misura istantanea, consuma meno)
#ifndef NODE_SENSOR
#define NODE_SENSOR NODE_SENSOR_DS18B20
#endif
#define DS18B20_BITS 10  // 0.25 C, 188 ms di conversione

// NTC: partitore 3V3(PIN_SENSOR_POWER) -- R fissa -- (PIN_SENSOR_DATA) -- NTC -- GND
#define NTC_R_FIXED_OHM 10000.0f
#define NTC_R25_OHM 10000.0f
#define NTC_BETA 3950.0f
#define NTC_SUPPLY_MV 3300.0f
#define NTC_SAMPLES 8

// ---- Pin ----
#if defined(ARDUINO_XIAO_ESP32C6) || defined(ARDUINO_XIAO_ESP32C3) || defined(ARDUINO_XIAO_ESP32S3)
#define PIN_BATT_ADC A0      // partitore 1M + 1M dal + della batteria (con 100 nF verso GND)
#define PIN_SENSOR_POWER D1  // alimenta il sensore solo durante la misura
#define PIN_SENSOR_DATA D2   // DS18B20 DQ (4.7k verso D1) oppure centro del partitore NTC
#else                        // ESP32 generico
#define PIN_BATT_ADC 35
#define PIN_SENSOR_POWER 26
#define PIN_SENSOR_DATA 32
#endif

#if defined(ARDUINO_XIAO_ESP32C6)
#define PIN_STATUS_LED 15  // LED giallo utente, acceso a livello basso
#define STATUS_LED_ON LOW
#else
#define PIN_STATUS_LED -1
#define STATUS_LED_ON HIGH
#endif

// ---- Batteria (Li-ion 1S, misurata con il partitore 1:2) ----
#define BATT_MEASURE 1         // 0 se il partitore non e' montato
#define BATT_DIVIDER 2.0f
#define BATT_LOW_MV 3500       // segnala "batteria bassa" alla centralina
#define BATT_CRITICAL_MV 3300  // sotto: una misura all'ora per proteggere la cella

// ---- Tempi ----
#define NODE_DEFAULT_INTERVAL_S 60  // finche' la centralina non ne suggerisce un altro
#define NODE_MIN_INTERVAL_S 10
#define NODE_MAX_INTERVAL_S 1800
#define NODE_HEARTBEAT_S 240        // trasmette almeno ogni 4 minuti anche se la temperatura non cambia
#define NODE_DELTA_CX100 30         // ...oppure se cambia di almeno 0.30 C
#define NODE_ACK_WAIT_MS 40         // attesa della conferma per ogni tentativo

// 1 = messaggi sulla seriale (consuma di piu': solo per le prove)
#ifndef NODE_DEBUG
#define NODE_DEBUG 0
#endif
