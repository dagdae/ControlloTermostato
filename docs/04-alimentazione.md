# 4. Alimentazione

## Quanto consuma la centralina

| Parte | Corrente a 5 V |
|---|---|
| ESP32 con Wi-Fi sempre acceso | 80-150 mA, picchi fino a ~400 mA |
| Bobina del relè Finder 34.51 (solo quando blocca) | 38 mA |
| MLX90614 | ~1,5 mA |
| Servo (solo se lo usi, mentre si muove) | picchi 0,5-0,8 A |

Basta un alimentatore da **5 V 1 A** (2 A con il servo). La centralina deve restare
sempre accesa: serve la pagina web, deve ricevere il nodo a batteria e comandare il
relè in qualsiasi momento ([perché non a batteria](01-architettura.md#perché-la-centralina-va-alimentata)).

## Opzione 1 (consigliata): alimentatore USB

Un normale alimentatore USB da 5 V 1 A **marcato CE** in una presa vicina, con il cavo
USB della scheda (micro-USB o USB-C). Il cavo può correre per 2-3 m lungo il
battiscopa. È la soluzione più sicura: non tocchi l'impianto elettrico e
l'isolamento dalla rete lo garantisce l'alimentatore certificato.

## Opzione 2: i due fili a 230 V dal vecchio foro nel muro

Prima di tutto bisogna sapere **da dove vengono**: devono essere fase e neutro di un
circuito protetto da interruttore magnetotermico e differenziale (il "salvavita"),
e devi sapere quale interruttore del quadro li stacca. Fili di provenienza sconosciuta
non vanno usati.

**La strada migliore:** con quei fili un elettricista monta una scatola da incasso con
una **presa** (esistono prese con uscita USB integrata) e torni all'opzione 1. In Italia
le nuove prese e le derivazioni fisse a 230 V vanno fatte da un installatore
abilitato (DM 37/2008), che rilascia la dichiarazione di conformità.

**Se l'alimentatore va dentro la centralina** (HLK-PM01, 5 V 0,6 A):

```
  fase L ───[fusibile T 500 mA 250 V]───┬──────────── AC  HLK-PM01  +Vo ──┬──► VIN (5 V) ESP32
                                        │                                │
                                  [varistore                          [470 µF]
                                   S10K275]                              │
                                        │                                │
  neutro N ─────────────────────────────┴──────────── AC            -Vo ─┴──► GND ESP32
```

* **Fusibile ritardato T 500 mA** sulla fase, prima di tutto il resto: interviene in
  caso di guasto dell'alimentatore.
* **Varistore da 275 V~** (S10K275 o 10D471K) tra fase e neutro dopo il fusibile:
  assorbe le sovratensioni della rete.
* Scatola **chiusa**, con il coperchio avvitato; i fili a 230 V entrano da un
  **pressacavo** e sono fissati in modo che, se un filo si stacca, non possa toccare la
  parte a 5 V.
* Almeno **6 mm** tra qualsiasi parte a 230 V (morsetti, piste, fusibile) e la parte a
  bassa tensione. Il modulo Hi-Link ha già l'isolamento al suo interno: non accorciare le
  distanze attorno ai suoi piedini.
* Se usi il servo, al posto dell'HLK-PM01 metti un **HLK-5M05** (5 V 1 A).

Se il circuito TA della caldaia è anch'esso a 230 V, nella stessa scatola ci sono due
circuiti di rete: tieni separati i loro morsetti e cablaggi.

## Opzione 3: un alimentatore da 9-12 V e il tuo mini buck

Se hai già un alimentatore da 9-12 V (per esempio quello di un vecchio router):
alimentatore → **MINI560 da 5 V** → pin **VIN** (o 5V) e GND della DevKit. Rispetta la
polarità e misura l'uscita del modulo prima di collegare la scheda. I mini buck non
servono con le opzioni 1 e 2, che danno già 5 V.

## Avvertenza su USB e alimentazione esterna

Mentre programmi la scheda dal PC via USB, scollega l'alimentazione esterna su VIN
(o verifica che la tua DevKit abbia il diodo tra USB e VIN): due alimentatori collegati
insieme si possono danneggiare a vicenda.

## Il nodo a batteria (configurazione B)

Un ESP32 "consuma tantissimo" solo se resta sveglio con il Wi-Fi acceso. Il nodo invece:

* passa quasi tutto il tempo in **deep sleep** (~15 µA sulla XIAO ESP32-C6);
* si sveglia ogni 30-120 secondi (l'intervallo lo suggerisce la centralina: corto
  vicino alle soglie, lungo a riposo), misura in circa 0,3 s e torna a dormire;
* **trasmette solo se serve**: la temperatura è cambiata di almeno 0,3 °C oppure sono
  passati 4 minuti dall'ultimo invio;
* usa **ESP-NOW**: non si collega al router (niente DHCP, niente attese), manda un
  pacchetto di 22 byte e aspetta la conferma per pochi millisecondi.

### Durata della batteria del nodo

Stima con le impostazioni di fabbrica, XIAO ESP32-C6 e sonda DS18B20:

| Voce | Valore |
|---|---|
| Deep sleep (scheda + partitore della batteria) | ~17 µA → 0,4 mAh al giorno |
| Ogni risveglio con misura (~0,35 s a ~22 mA) | ~2,1 µAh |
| Ogni trasmissione con conferma (~0,15 s a ~90 mA) | ~3,8 µAh |
| Giornata tipo: 8 h di cicli (misura ogni 30 s) + 16 h a riposo (ogni 120 s) | 1440 risvegli, ~840 trasmissioni |
| **Totale** | **circa 6,6 mAh al giorno** |
| Con una 18650 da 2500 mAh utili | **circa 12 mesi** |

Sono stime: tempi e correnti cambiano con la scheda, la sonda e la distanza dalla
centralina. Per conoscere il valore vero misura con il tester in serie alla batteria
(scala µA in deep sleep, mA durante la trasmissione). Una stagione di riscaldamento dura
circa 6 mesi, quindi una ricarica all'anno basta con buon margine.

Per farla durare di più:

* nella pagina della centralina porta "Misura del nodo a riposo" a 300 s e "vicino alle
  soglie" a 60 s: circa 3,5 mAh al giorno, **quasi 2 anni**;
* usa l'NTC invece del DS18B20 (`-DNODE_SENSOR=2`): niente attesa di conversione,
  circa metà del consumo di ogni risveglio;
* d'estate togli la batteria. Se la lasci e la centralina è spenta, il nodo non trova
  risposta e allunga da solo le attese fino a un tentativo ogni 30 minuti (circa 1 mAh
  al giorno).

### Scelta e protezione della batteria

* **Li-ion 3,7 V protetta** (18650 con circuito PCM, oppure LiPo con protezione): il
  caricabatterie della XIAO la carica a 4,2 V dalla presa USB-C.
* **Non** collegare celle **LiFePO4** alla XIAO: il caricabatterie le porterebbe a 4,2 V
  (il massimo per una LiFePO4 è 3,6 V).
* Il firmware segnala alla centralina "batteria bassa" sotto **3,50 V** e sotto
  **3,30 V** fa una sola misura all'ora per proteggere la cella.
* Il fusibile PTC da 0,5 A sul + della batteria limita i danni in caso di corto.
* Le batterie al litio non amano il caldo: carica tra 0 e 45 °C, uso fino a 60 °C, e
  invecchiano più in fretta sopra i 35-40 °C. Per questo il nodo va **di fianco** al
  termosifone e non sopra (vedi [montaggio](05-montaggio.md#nodo-a-batteria-configurazione-b)).
