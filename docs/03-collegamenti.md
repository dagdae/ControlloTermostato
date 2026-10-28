# 3. Collegamenti e schemi

## Prima di tutto: misura la tensione del circuito TA

Il termostato chiude un contatto pulito (senza tensione propria) collegato con due fili
ai morsetti **TA** della caldaia. Su quei due fili può esserci bassa tensione (il caso
più comune con le caldaie moderne: da pochi volt a 24 V) oppure **230 V** (impianti
vecchi o con valvole di zona). Va saputo prima di mettere mano a qualsiasi cosa.

1. Metti il termostato in modo che **non chieda calore** (temperatura impostata bassa):
   il contatto è aperto.
2. Con il tester in **V~** (alternata) e poi in **V=** (continua) misura tra i due
   morsetti del termostato a cui arrivano i fili della caldaia.
3. Leggi il risultato:
   * da 3 a 30 V circa: **bassa tensione**. Il lavoro è semplice, ma spegni comunque
     la caldaia prima di scollegare i fili.
   * circa **230 V~**: il circuito è **in tensione di rete**. Stacca l'interruttore
     della caldaia, usa il relè Finder indicato e una scatola chiusa, tieni i fili a
     230 V separati dalla parte a 5 V. Se non sei pratico, fai fare il collegamento a
     un elettricista.
   * 0 V in entrambe le misure: probabilmente il termostato sta chiedendo calore
     (contatto chiuso). Abbassa la temperatura impostata e rimisura.

Fai una **foto dei morsetti** prima di scollegare qualunque filo.

## Il relè in serie al termostato (contatto NC, a prova di guasto)

Si interrompe **uno** dei due fili tra caldaia e termostato e lo si fa passare dal
contatto **COM-NC** del relè della centralina.

```
 PRIMA

  caldaia, morsetti TA                     termostato TH/124
  ┌──────────┐                             ┌──────────┐
  │  TA ●────┼─────────────────────────────┼────● C   │   C  = comune del relè del termostato
  │  TA ●────┼─────────────────────────────┼────● NA  │   NA = contatto che si chiude quando
  └──────────┘                             └──────────┘        chiede calore

 DOPO

  caldaia, morsetti TA                     termostato TH/124
  ┌──────────┐                             ┌──────────┐
  │  TA ●────┼─────────────────────────────┼────● C   │
  │  TA ●────┼──────┐             ┌────────┼────● NA  │
  └──────────┘      │             │        └──────────┘
                    │  centralina │
               ┌────┴─────────────┴────┐
               │ TA-IN          TA-OUT │   morsettiera della centralina
               │   │               │   │
               │  11 COM ●═════● 12 NC │   relè Finder 34.51: COM-NC chiuso a riposo
               │                       │   (il contatto 14 NO resta libero)
               └───────────────────────┘
```

* **Relè a riposo** (centralina spenta, guasta o in "riscaldamento consentito"):
  COM-NC è chiuso, il filo è come se non fosse stato interrotto, il termostato comanda
  la caldaia come prima.
* **Relè eccitato** (centralina in "pausa" o "bloccato"): COM-NC si apre, la caldaia
  non riceve la richiesta di calore qualunque cosa faccia il termostato.

Passi:

1. Spegni la caldaia (spina o interruttore).
2. Scollega dal termostato **uno** dei due fili che arrivano dalla caldaia (per esempio
   quello sul morsetto NA) e portalo al morsetto **TA-IN** della centralina.
3. Con un filo nuovo dello stesso tipo collega il morsetto **TA-OUT** della centralina
   al morsetto del termostato che hai appena liberato.
4. Il contatto **NO** (14) del relè resta libero.
5. Riaccendi la caldaia **con la centralina scollegata dall'alimentazione**: il
   riscaldamento deve funzionare esattamente come prima (prova ad alzare la temperatura
   sul termostato). Poi alimenta la centralina.

Cavo: se il circuito è a bassa tensione basta un cavetto bipolare da 0,5 mm²; se è a
230 V usa un cavo a doppio isolamento (per esempio H05VV-F 2×0,75 mm²) e tieni la
centralina a pochi decimetri dal termostato.

Non mettere condensatori o reti RC (snubber) ai capi del contatto: la piccola
corrente che lasciano passare può bastare a far credere alla caldaia che il termostato
chieda calore.

La logica alternativa "eccitato = consenti" (contatto **COM-NO**, attuatore "Relè NO"
nella pagina) blocca il riscaldamento quando la centralina è spenta. Usala solo se è
proprio quello che vuoi.

## Comando del relè dalla ESP32

L'ESP32 lavora a 3,3 V e non può alimentare direttamente la bobina del relè (5 V,
circa 38 mA): serve un transistor.

```
                        +5 V  (pin VIN / 5V della DevKit)
                          │
             ┌────────────┼────────────┐
             │            │            │
          1N4148 ▲      bobina       [1 kΩ]
       (la riga, cioè   relè          │
        il catodo, va   A1-A2       LED ▼   (facoltativo: acceso =
        verso +5 V)       │            │     riscaldamento bloccato)
             │            │            │
             └────────────┼────────────┘
                          │
                          C (collettore)
  GPIO26 ──[1 kΩ]──┬──── B   BC337
                   │      E (emettitore)
                [10 kΩ]   │
                   │      │
  GND ─────────────┴──────┴───────────────── GND
```

* La resistenza da **10 kΩ** tiene il relè a riposo mentre l'ESP32 si accende o si
  riprogramma.
* Il **diodo** assorbe il picco di tensione della bobina quando si spegne: senza, il
  transistor si rompe.
* Piedini: BC337 con il lato piatto verso di te e i piedini in basso = **C B E**.
  Attenzione, il 2N2222A in TO-92 è al contrario (**E B C**).
* Relè Finder 34.51: bobina **A1-A2** (polarità indifferente), contatti **11** = comune,
  **12** = NC, **14** = NO.
* Sulla millefori tieni almeno **6 mm** tra le piste dei contatti del relè e tutto il
  resto, se il circuito TA è a 230 V.

### Con un modulo relè già pronto

Va bene per circuiti TA a **bassa tensione**. Scegli un modulo a 1 canale da 5 V con
ponticello di selezione **High/Low trigger** e mettilo su **High**:
VCC → 5 V, GND → GND, IN → GPIO26; contatti COM e NC come sopra.

I moduli solo "low level trigger" con fotoaccoppiatore a 5 V spesso **non si
diseccitano** con i 3,3 V dell'ESP32. Se ne hai uno, attiva "Modulo relè attivo basso"
nella pagina e verifica con il tester che il contatto si apra e si chiuda davvero; se
non succede, usa il circuito a transistor.

## Sensore a infrarossi MLX90614 (configurazione A)

```
  modulo GY-906-DCI          ESP32 DevKit
  VIN ────────────────────── 3V3
  GND ────────────────────── GND
  SCL ────────────────────── GPIO22
  SDA ────────────────────── GPIO21
```

* Alimentalo a **3,3 V** (la DCI è una versione a 3 V; i moduli con regolatore
  funzionano anche così).
* Il modulo ha già le resistenze di pull-up sul bus I2C.
* Cavo al massimo di 1 m (bus I2C a 100 kHz); se il sensore sta in un contenitore a parte
  usa un cavetto a 4 poli e un condensatore da 100 nF tra VIN e GND vicino al sensore.

## Sonda DS18B20 a filo (configurazione C)

```
                3V3
                 │
              [4,7 kΩ]
                 │
  GPIO4 ─────────┴──[100 Ω]──────────── DATI (giallo o bianco)
  3V3 ───────────────────────────────── VDD  (rosso)
  GND ───────────────────────────────── GND  (nero)
                          100 nF tra VDD e GND vicino alla sonda (facoltativo)
```

* Usa i **tre fili**, non il collegamento "parassita" a due fili.
* Con la 4,7 kΩ il cavo può essere lungo 5-10 m (per esempio cavo telefonico o da
  allarme a 4 poli lungo il battiscopa). Tienilo lontano dai cavi a 230 V.
* La 100 Ω e il 100 nF sono facoltativi: proteggono l'ingresso da scariche e disturbi
  sui cavi lunghi.

## Servo, il "ditino" (facoltativo)

```
  servo MG90S / SG90
  marrone  ───── GND
  rosso    ───── +5 V   (mai dal pin 3V3)       470-1000 µF tra +5 V e GND
  arancio  ───── GPIO25
```

Per togliere del tutto l'alimentazione al servo tra un movimento e l'altro:
MOSFET N logic level (AO3400A) sul filo marrone: drain al marrone, source a GND, gate
al GPIO27 tramite 100 Ω, con 100 kΩ tra gate e GND; poi compila con
`-DPIN_SERVO_POWER=27` (vedi `firmware/controller/src/config.h`).

Gli angoli e la durata della pressione si regolano dalla pagina web.

## Tasto e LED

Si usano quelli già presenti sulla DevKit: tasto **BOOT** (GPIO0) e LED blu (GPIO2).
Puoi aggiungere un pulsante esterno in parallelo al BOOT (tra GPIO0 e GND), ma non
tenerlo premuto mentre colleghi l'alimentazione: l'ESP32 entrerebbe in modalità
programmazione.

## Riepilogo dei pin della centralina

| Funzione | GPIO | Note |
|---|---|---|
| MLX90614 SDA | 21 | |
| MLX90614 SCL | 22 | |
| DS18B20 dati | 4 | pull-up 4,7 kΩ verso 3V3 |
| Relè | 26 | 1 kΩ verso la base del BC337, 10 kΩ base-GND |
| Servo, segnale | 25 | |
| Servo, alimentazione | — | facoltativo, es. 27 |
| Tasto | 0 | BOOT |
| LED | 2 | LED della scheda |

Per un'altra scheda cambia i pin in `firmware/controller/src/config.h` o in
`platformio.ini` (c'è un esempio per ESP32-S3).

## Nodo a batteria (configurazione B)

```
            Seeed XIAO ESP32-C6, vista dall'alto (USB-C in alto)
                     ┌──────────────────────┐
  partitore batt. ── │ D0/A0            5V  │
  sonda VDD (rosso)─ │ D1               GND │ ── GND sonda, GND partitore
  sonda DATI ─────── │ D2               3V3 │
                     │ D3               D10 │
                     │ D4               D9  │
                     │ D5               D8  │
                     │ D6               D7  │
                     └──────────────────────┘
               sotto la scheda: piazzole BAT+ e BAT- ── batteria Li-ion protetta
```

Sonda DS18B20, alimentata solo durante la misura:

```
  D1 ──┬──────────────── VDD (rosso)
       │
    [4,7 kΩ]
       │
  D2 ──┴──────────────── DATI (giallo o bianco)
  GND ────────────────── GND (nero)
```

In alternativa termistore NTC 10 kΩ B3950 (compila con `-DNODE_SENSOR=2`):

```
  D1 ──[10 kΩ 1%]──┬── D2
                   │
                 NTC 10 kΩ
                   │
  GND ─────────────┘
```

Misura della batteria:

```
  BAT+ ──[1 MΩ]──┬──[1 MΩ]── GND
                 │
                 ├────────── D0/A0
                 │
              [100 nF]
                 │
                GND
```

* Batteria sulle piazzole **BAT+ / BAT-** sotto la scheda. **Controlla due volte la
  polarità**: non c'è protezione contro l'inversione. Il PTC da 0,5 A va in serie al
  filo del +.
* La batteria si ricarica dalla presa **USB-C** della XIAO (il LED rosso lampeggia
  durante la carica e si spegne a carica completa). La corrente di carica è bassa,
  circa 100 mA: una 18650 vuota richiede più di un giorno. Se hai un caricatore per
  18650 esterno, fai prima a togliere la cella dal portabatteria.
* Senza partitore metti `BATT_MEASURE` a 0 in `firmware/sensor_node/src/config.h`.

## Schema a blocchi della centralina

```
               ┌──────────────────────── scatola della centralina ────────────────────────┐
  USB 5 V ────►│ ESP32 DevKit                                                             │
               │   3V3, GND, GPIO21, GPIO22 ──── cavetto 4 poli ──► GY-906-DCI nel paraluce │
               │                                                     (foro verso il       │
               │   GPIO4 (+4,7 kΩ a 3V3) ──────────────────────────► DS18B20, solo C)     │
               │                                                                          │
               │   VIN 5 V ──► bobina relè ──► BC337 ◄──[1 kΩ]── GPIO26                    │
               │                                                                          │
               │   relè: 11 COM ── morsetto TA-IN        12 NC ── morsetto TA-OUT         │
               └──────────────────────────────────────────────────────────────────────────┘
                                       │                         │
                         filo dalla caldaia (TA)       filo verso il termostato
```
