// Configurazione di compilazione della centralina.
// I parametri di funzionamento (soglie, timer, sorgente temperatura, attuatore,
// Wi-Fi...) NON stanno qui: si impostano dalla pagina web e restano in memoria.
#pragma once

#define FW_VERSION "1.0.0"

// ---- Pin per ESP32 "classico" (ESP32-WROOM-32, DevKit V1 / DevKitC) ----
// Per altre schede cambia i numeri qui sotto (o passali con -D... da platformio.ini).
#ifndef PIN_I2C_SDA
#define PIN_I2C_SDA 21  // MLX90614 SDA
#endif
#ifndef PIN_I2C_SCL
#define PIN_I2C_SCL 22  // MLX90614 SCL
#endif
#ifndef PIN_ONEWIRE
#define PIN_ONEWIRE 4  // DS18B20 dati, con pull-up 4.7 kOhm verso 3V3
#endif
#ifndef PIN_RELAY
#define PIN_RELAY 26  // base del transistor del rele' (tramite 1 kOhm) o ingresso IN del modulo rele'
#endif
#ifndef PIN_SERVO
#define PIN_SERVO 25  // segnale del servo (solo se usi il "ditino")
#endif
#ifndef PIN_SERVO_POWER
#define PIN_SERVO_POWER -1  // MOSFET che alimenta il servo; -1 = servo sempre alimentato
#endif
#ifndef PIN_BUTTON
#define PIN_BUTTON 0  // tasto BOOT della scheda (chiude verso GND)
#endif
#ifndef PIN_LED
#define PIN_LED 2  // LED della scheda; -1 = nessun LED
#endif
#ifndef LED_ACTIVE_HIGH
#define LED_ACTIVE_HIGH 1
#endif

// ---- Wi-Fi di configurazione ----
// Rete "Termosifone-XXXX" creata al primo avvio o se la rete di casa non risponde.
#ifndef AP_PASSWORD
#define AP_PASSWORD "termosifone"  // almeno 8 caratteri
#endif
#ifndef AP_CHANNEL
#define AP_CHANNEL 1
#endif
#define DEFAULT_HOSTNAME "termosifone"

// ---- Storico ed eventi ----
#define HISTORY_INTERVAL_S 60
#define HISTORY_SIZE 1440  // 24 ore a un punto al minuto
#define EVENT_LOG_SIZE 24
