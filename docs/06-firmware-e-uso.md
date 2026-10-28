# 6. Firmware e uso

## Caricare il firmware

Non servono librerie esterne: basta il core **Arduino-ESP32 3.x** di Espressif.

### Con PlatformIO (consigliato)

1. Installa Visual Studio Code e l'estensione **PlatformIO IDE**.
2. Apri la cartella `firmware/controller`.
3. Collega l'ESP32 via USB e premi **Upload** (la freccia nella barra in basso).
   La prima compilazione scarica la piattaforma e richiede qualche minuto.
4. **Serial Monitor** a 115200 baud per vedere i messaggi.

Da riga di comando:

```bash
pio run -d firmware/controller -t upload
```

### Con Arduino IDE 2.x

1. *File → Preferenze → URL aggiuntive per il Gestore schede*:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. *Gestore schede*: installa **esp32 di Espressif Systems**, versione 3.x.
3. Apri `firmware/controller/controller.ino` (il codice è nella sottocartella `src`).
4. Scheda **ESP32 Dev Module**, Partition Scheme **Minimal SPIFFS (1.9MB APP with OTA)**.
5. Carica.

### Il nodo a batteria

Stessa procedura con la cartella `firmware/sensor_node` (ambiente PlatformIO
`xiao_esp32c6`, oppure scheda **XIAO_ESP32C6** in Arduino IDE). Pin, tipo di sonda e
tempi sono in `firmware/sensor_node/src/config.h`.

Il nodo dorme quasi sempre, quindi la porta USB sparisce dopo pochi istanti. Per
riprogrammarlo: tieni premuto **BOOT**, premi e rilascia **RESET**, rilascia BOOT, poi
carica. Alla fine premi RESET.

### File già compilati

Ogni modifica al repository viene compilata dalla CI di GitHub (scheda *Actions*): tra
gli "artifacts" ci sono i `firmware.bin` pronti per l'aggiornamento dalla pagina web.
Il primo caricamento va fatto comunque via USB.

## Prima accensione

1. La centralina crea la rete Wi-Fi **Termosifone-XXXX** (password `termosifone`).
   Collegati con il telefono e apri **http://192.168.4.1**.
2. Nella sezione **Wi-Fi e accesso** scrivi nome e password della rete di casa, il nome
   del dispositivo e, se vuoi, una password per la pagina. Premi **Salva e riavvia**.
3. Da quel momento la pagina è su **http://termosifone.local** (o sull'indirizzo IP che
   trovi nel router o sul monitor seriale; alcuni telefoni Android non risolvono i nomi
   `.local`).

La rete di configurazione ricompare da sola se la rete di casa non risponde per un
minuto, oppure tenendo premuto il tasto BOOT per 10 secondi. Mentre è attiva, la
centralina riprova la rete di casa ogni 5 minuti (cercarla in continuazione renderebbe
instabile la rete di configurazione). La password della rete di configurazione si
cambia in `firmware/controller/src/config.h` (`AP_PASSWORD`).

Poi, nella sezione **Impostazioni**, scegli sorgente della temperatura, attuatore e
soglie, e premi **Salva**.

## La pagina web

* **Stato**: temperatura del termosifone, sorgente, uscita (*riscaldamento consentito*
  o *bloccato*), modalità, fase del ciclo, timer in corso, ultimo evento, avvisi.
* **Comandi**: Automatico, Solo termostato, Spento forzato, Riaccendi ora, Acceso
  forzato per N minuti. Con il servo "a pulsante" compare anche *Il termostato adesso
  è: Acceso / Spento* per riallinearlo.
* **Grafico delle ultime 24 ore** con le soglie e le fasce in cui il riscaldamento era
  bloccato (passa il dito o il mouse sul grafico per leggere i valori), più una
  **tabella oraria** con minimo, media, massimo e minuti di blocco.
* **Nodi sensore a batteria**: temperatura, tensione della batteria, segnale, ultimo
  dato.
* **Impostazioni**, **Calibrazione infrarossi**, **Wi-Fi e accesso**, **Eventi**.
* In fondo: **Aggiorna firmware**, **Riavvia**, **Ripristino di fabbrica**.

La regolazione non dipende dalla pagina né dal Wi-Fi: se il router si spegne, la
centralina continua a lavorare.

## Parametri

| Nella pagina | Default | Significato |
|---|---|---|
| Spegni quando il termosifone arriva a | 50 °C | soglia di spegnimento |
| Ritardo dopo la soglia | 0 min | riscaldamento ancora consentito dopo aver raggiunto la soglia (0 = spegne subito) |
| Riaccensione | quando scende sotto la soglia | temperatura, timer, timer o soglia (il primo), timer e soglia (entrambi), mai |
| Riaccendi sotto | 30 °C | soglia bassa |
| Timer di pausa | 90 min | durata della pausa nelle modalità a timer |
| Limite di sicurezza | 75 °C | sopra questo valore blocca sempre |
| Sorgente | infrarossi | infrarossi MLX90614, sonda DS18B20 a filo, nodo a batteria |
| Emissività efficace | 0,95 | solo infrarossi; la imposta la calibrazione |
| Correzione | 0 °C | sommata alla lettura (per tarare le sonde) |
| Conferma soglia | 20 s | per quanto una soglia deve restare superata prima di agire |
| Se il sensore si guasta | lascia comandare il termostato | oppure blocca il riscaldamento |
| Guasto dopo (sensore locale / nodo) | 120 s / 900 s | tempo senza letture valide |
| Misura del nodo vicino alle soglie / a riposo | 30 s / 120 s | intervalli suggeriti al nodo a batteria |
| Tipo di attuatore | relè NC | relè NC, relè NO, servo a due posizioni, servo a pulsante |
| Modulo relè attivo basso | no | solo per moduli relè "low level trigger" |
| Angoli e durata del servo | 30°/120°, 90°/60°, 400 ms | posizioni "consenti"/"blocca"; riposo/pressione |

Le soglie si riferiscono alla temperatura **misurata**: con la sonda sul tubo di
mandata o con il sensore IR non tarato possono essere diverse dalla temperatura media
del termosifone. Regolale guardando il grafico dei primi giorni.

## Esempi

| Vuoi | Imposta |
|---|---|
| Spegnere a 50 °C e riaccendere sotto 30 °C | spegni a **50**, ritardo **0**, riaccensione **quando scende sotto la soglia**, riaccendi sotto **30** |
| A 50 °C un timer di 15 minuti e poi spegne; riaccende dopo 1 h 30 | spegni a **50**, ritardo **15**, riaccensione **dopo il timer**, timer **90** |
| Come sopra, ma riaccende prima se il termosifone è già sotto 30 °C | riaccensione **timer o soglia (il primo)** |
| Pausa di almeno 1 h 30 e comunque non prima che scenda sotto 30 °C | riaccensione **timer e soglia (entrambi)** |
| Non riaccendere più finché non lo dico io | riaccensione **mai**; per ripartire: *Riaccendi ora* nella pagina o pressione breve del tasto BOOT |
| Togliere di mezzo la centralina per un po' | comando **Solo termostato** (o tasto BOOT per 3 s) |

## Tasto e LED della centralina

| Tasto BOOT | Effetto |
|---|---|
| pressione breve | **Riaccendi ora** (oppure torna in automatico se eri in un'altra modalità) |
| 3 secondi | alterna **Automatico** e **Solo termostato** |
| 10 secondi | apre la rete di configurazione |

| LED | Significato |
|---|---|
| acceso fisso | riscaldamento consentito |
| un lampo ogni 2 secondi | riscaldamento bloccato (pausa, blocco o spento forzato) |
| lampeggio rapido | sensore guasto o assente |
| breve inversione ogni 4 secondi | rete di configurazione attiva |

## Taratura del sensore a infrarossi

1. Aspetta che il termosifone sia **caldo e stabile** (per esempio verso la fine di un
   ciclo di riscaldamento, intorno ai 45-55 °C).
2. Misura la sua temperatura **nel punto inquadrato** con un termometro a contatto: un
   termometro da cucina a sonda con la punta premuta sulla ghisa e coperta con un
   panno, oppure la sonda DS18B20 appoggiata per qualche minuto.
3. Scrivi il valore in **Calibrazione infrarossi** e premi **Calibra**. La centralina
   calcola l'emissività efficace e azzera la correzione.

Il termosifone deve essere almeno 8 °C più caldo della stanza, altrimenti la
calibrazione viene rifiutata perché sarebbe imprecisa. Se preferisci, puoi anche non
tarare e impostare direttamente come soglie le temperature che il sensore legge quando
il termosifone è dove lo vuoi.

## Aggiornare il firmware senza cavo

Pagina **/update** (link in fondo alla pagina principale): scegli il file
`firmware.bin`.

* PlatformIO: `firmware/controller/.pio/build/esp32dev/firmware.bin`.
* Arduino IDE: *Sketch → Esporta binario compilato*, poi il file `.bin` senza
  "bootloader", "partitions" o "merged" nel nome.

Se hai impostato la password della pagina, l'aggiornamento la richiede (utente `admin`).
Se qualcosa va storto, resta attivo il firmware precedente.

## Più impianti vicini

Nodo e centralina si riconoscono con un identificativo d'impianto (`RP_DEFAULT_NET_ID`
in `radiator_packet.h`, presente in entrambe le cartelle del firmware). Se un vicino
usa lo stesso progetto, cambialo in tutte e due le copie del file.

## Problemi comuni

| Sintomo | Causa probabile | Cosa fare |
|---|---|---|
| "MLX90614 non risponde" | SDA e SCL invertiti, manca la 3,3 V, cavo troppo lungo | controlla i 4 fili; cavo sotto 1 m |
| Lettura IR troppo bassa o instabile | sensore BAA (90°) invece di DCI, puntamento sul bordo, vernice metallizzata, tenda davanti | verifica il modello, ripunta, metti il nastro nero, tara |
| "DS18B20 non risponde" | manca la resistenza da 4,7 kΩ, fili scambiati | controlla colori e pull-up |
| Errori CRC del DS18B20 | cavo lungo vicino a cavi a 230 V, contatti incerti | allontana i cavi, aggiungi 100 Ω e 100 nF |
| Il relè non scatta | piedini del transistor invertiti, manca il 5 V | controlla C-B-E e il pin VIN |
| Con la centralina spenta la caldaia non parte più | filo sul contatto NO invece che NC | sposta il filo su NC (morsetto 12 del Finder) |
| Il nodo lampeggia 10 volte all'accensione | centralina spenta o lontana, identificativo diverso | avvicina, controlla `RP_DEFAULT_NET_ID` |
| "nessun nodo recente" | batteria scarica, nodo troppo lontano | controlla la tensione nella tabella nodi |
| Non trovo la pagina | rete sbagliata, nome `.local` non supportato | usa l'IP mostrato dal router o dal monitor seriale |
