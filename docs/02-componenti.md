# 2. Componenti con modelli precisi

Prezzi indicativi (2026, negozi online come Amazon, AliExpress, Mouser, RS, Farnell).
Per le parti a 230 V e per il sensore a infrarossi conviene un venditore affidabile:
circolano molte copie.

## Configurazione A: centralina con sensore a infrarossi (consigliata)

| # | Componente | Modello | Q.tà | Note | Prezzo |
|---|---|---|---|---|---|
| 1 | Microcontrollore | **ESP32 DevKit** (ESP32-WROOM-32/32D/32E, versione da 30 o 38 pin) | 1 | quello che hai già | — |
| 2 | Sensore a infrarossi | modulo **GY-906-DCI** con **Melexis MLX90614ESF-DCI** (cono di 5°, versione da 3 V) | 1 | a 1-1,5 m va bene anche la **MLX90614ESF-BCF** (10°). Da evitare per questo uso: GY-906-**BAA** (90°) | 15-30 € |
| 3 | Relè | **Finder 34.51.7.005.0010**: 1 scambio, 6 A 250 V~, bobina 5 V DC 130 Ω (170 mW), isolamento bobina-contatti 6 kV | 1 | adatto anche se il circuito della caldaia è a 230 V | 5-8 € |
| 4 | Transistor NPN | **BC337-40** (oppure 2N2222A) | 1 | pilota la bobina del relè | 0,10 € |
| 5 | Diodo | **1N4148** (oppure 1N4007) | 1 | sulla bobina, contro i picchi di tensione | 0,05 € |
| 6 | Resistenze 1/4 W | 1 kΩ ×2, 10 kΩ ×1 | 3 | base del transistor, LED, tenuta a riposo | 0,10 € |
| 7 | LED 3 mm | rosso | 1 | facoltativo: acceso = relè eccitato = riscaldamento bloccato | 0,05 € |
| 8 | Condensatori | elettrolitico **470 µF 10 V**, ceramici **100 nF** ×2 | 3 | sul 5 V e vicino ai sensori | 0,30 € |
| 9 | Morsettiere a vite | 2 poli, passo 5,08 mm, **300 V** | 2 | linea del termostato (entrata/uscita) | 0,50 € |
| 10 | Basetta millefori | 5×7 cm | 1 | per relè e transistor | 1 € |
| 11 | Scatola | plastica ABS circa 100×70×40 mm | 1 | se c'è il 230 V dentro: scatola chiusa con pressacavi | 3-8 € |
| 12 | Paraluce del sensore IR | spezzone di tubo in PVC o alluminio Ø 16-20 mm, lungo 3-5 cm | 1 | protegge il sensore da correnti d'aria e infrarossi laterali | — |
| 13 | Alimentazione | alimentatore USB **5 V ≥ 1 A** marcato CE + cavo USB della scheda (micro-USB o USB-C) | 1 | la soluzione più sicura; per i 230 V vedi sotto | 5-10 € |
| 14 | Facoltativi | nastro isolante nero opaco (bersaglio IR se la vernice è metallizzata), puntatore laser (per mirare), termometro a contatto (per tarare) | | | |

Totale indicativo: **35-60 €**, ESP32 escluso.

## Configurazione C: sonda a contatto con filo

Al posto del sensore a infrarossi (oppure in aggiunta, per tarare l'IR):

| Componente | Modello | Q.tà | Note | Prezzo |
|---|---|---|---|---|
| Sonda di temperatura | **DS18B20** impermeabile, puntale inox Ø 6×50 mm, 3 fili, cavo 2-3 m, meglio con **cavo in silicone** | 1 | i fili di solito sono rosso = VDD, nero = GND, giallo o bianco = dati (controlla sul tuo) | 2-5 € |
| Resistenza | 4,7 kΩ | 1 | pull-up della linea dati (spesso inclusa nei kit) | 0,05 € |
| Protezione cavo lungo | 100 Ω + 100 nF | 1+1 | facoltative, utili con più di 3 m di cavo | 0,10 € |
| Magnete | magnete **a tazza in ferrite** Ø 20-25 mm con foro (la ghisa è magnetica) | 1 | evita il neodimio normale (grado N): si smagnetizza sopra 80 °C | 2-4 € |
| Fissaggio | una goccia di pasta termoconduttiva, nastro di alluminio adesivo, un pezzetto di neoprene o gommapiuma | | isolano la sonda dall'aria della stanza | 3 € |

## Configurazione B: nodo a batteria sul termosifone

| Componente | Modello | Q.tà | Note | Prezzo |
|---|---|---|---|---|
| Microcontrollore | **Seeed Studio XIAO ESP32-C6** | 1 | ~15 µA in deep sleep, caricabatterie Li-ion integrato (USB-C), piazzole BAT+/BAT- | 6-9 € |
| (alternative) | Seeed XIAO ESP32-C3 (~44 µA) oppure DFRobot FireBeetle 2 ESP32-E (DFR0654) | | da evitare: le DevKit ESP32 generiche, che in deep sleep consumano alcuni mA per regolatore e LED | |
| Sonda | **DS18B20** come sopra, cavo 0,5-1 m + 4,7 kΩ; oppure termistore **NTC 10 kΩ B3950** a sonda + resistenza 10 kΩ 1% | 1 | l'NTC risparmia un po' di batteria ma è meno preciso | 2-5 € |
| Batteria | Li-ion **18650 3,7 V protetta** (circuito di protezione PCM) 2500-3500 mAh di marca: Samsung INR18650-35E, LG INR18650-MJ1, Panasonic NCR18650B | 1 | oppure LiPo 3,7 V 1000-2000 mAh **con protezione** e connettore JST. Niente celle "9900 mAh" senza marca | 6-12 € |
| Portabatteria | 18650 singolo con fili | 1 | così la cella si può caricare anche in un caricatore esterno | 1-2 € |
| Partitore batteria | resistenze **1 MΩ ×2**, condensatore **100 nF** | 3 | per misurare la tensione della batteria | 0,10 € |
| Fusibile | PTC ripristinabile 0,5 A (es. Bourns MF-R050) | 1 | facoltativo, in serie al + della batteria | 0,30 € |
| Scatoletta | plastica circa 70×40×30 mm + biadesivo o viti | 1 | | 2-4 € |
| Magnete e fissaggio sonda | come nella configurazione C | | | |

Totale indicativo del nodo: **20-35 €**.

## Ditino (servo), facoltativo

Solo se non vuoi o non puoi usare il relè (con il TH/124 il relè è consigliato).

| Componente | Modello | Note |
|---|---|---|
| Servo | **MG90S** (ingranaggi metallici) o SG90 | |
| Condensatore | 470-1000 µF 10 V | ai capi dell'alimentazione del servo |
| Alimentazione | 5 V ≥ 2 A (USB) oppure **Hi-Link HLK-5M05** (5 V 1 A) da 230 V | il servo assorbe picchi di 0,5-0,8 A |
| Braccetto | stampato in 3D o in legno, punta in gomma | |
| Fissaggio | biadesivo **3M VHB** | si toglie senza rovinare il muro |
| Facoltativo | MOSFET logic level (**AO3400A** su basetta, o **IRLZ44N**) | per togliere del tutto alimentazione al servo tra un movimento e l'altro |

## Alimentazione da 230 V, se tiri i due fili dal muro

| Componente | Modello | Note |
|---|---|---|
| Alimentatore AC/DC | **Hi-Link HLK-PM01** (100-240 V~ → 5 V 0,6 A, 3 W) | **HLK-5M05** (5 V 1 A, 5 W) se usi il servo |
| Fusibile | ritardato **T 500 mA 250 V**, 5×20 mm, con portafusibile | sulla fase |
| Varistore | **S10K275** (TDK/EPCOS) oppure **10D471K** | tra fase e neutro, dopo il fusibile |
| Morsetti | a vite, 2 poli, 250 V o più | |
| Scatola | chiusa, con pressacavo per i fili a 230 V | i fili a 230 V non devono poter uscire dai morsetti |

Valuta prima l'alternativa più semplice e sicura: con quei due fili un elettricista
monta una **presa** (anche una presa con uscita USB integrata) e la centralina si
alimenta con un normale alimentatore USB. Dettagli in [alimentazione](04-alimentazione.md).

## I tuoi mini buck

Da come li descrivi (circa 3×2 cm, uscita fissa 3,3, 5, 9 o 12 V a seconda del
modello, tutti uguali tra loro) sono quasi certamente dei **MINI560**: 5 A, ingresso
fino a circa 20 V, e l'ingresso deve stare almeno 2 V sopra l'uscita. Per
riconoscerli, alimentali con 9-12 V, misura l'uscita con il tester e scrivi la
tensione sopra ognuno con un pennarello. Alcuni venditori indicano la versione con la sigla
della resistenza SMD vicino al chip (per esempio "16C" per la versione da 9 V), ma la
misura è più sicura.

In questo progetto servono solo in un caso: se alimenti tutto da un alimentatore a
9-12 V (MINI560 da **5 V** → pin VIN/5V dell'ESP32). Non servono con l'alimentatore
USB o con l'HLK-PM01, che danno già 5 V. Non vanno bene con una batteria Li-ion
singola (3,0-4,2 V non bastano per stare 2 V sopra l'uscita) e il loro consumo a vuoto,
che i venditori non dichiarano, per moduli di questo tipo è dell'ordine del milliampere:
decine di volte quello di un ESP32 in deep sleep.

## Da non comprare per questo progetto

* **GY-906-BAA** (90°) da usare a distanza: va bene solo a pochi centimetri.
* Moduli relè economici **senza fessure di isolamento** sulla basetta se il circuito
  della caldaia è a 230 V.
* Magneti al **neodimio** normali (N35, N42, N52) a contatto con il termosifone: oltre
  80 °C perdono magnetismo in modo permanente. Ferrite, oppure neodimio di grado "H"/"SH".
* Celle 18650 senza marca o **non protette**.
* Una **ESP32 DevKit** come nodo a batteria: regolatore AMS1117, LED di
  alimentazione e chip USB consumano alcuni mA anche in deep sleep (batteria scarica in
  pochi giorni).
