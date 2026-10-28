# ControlloTermostato

Spegne il riscaldamento quando i termosifoni in ghisa arrivano a una temperatura
(es. **50 °C**) e lo riaccende quando scendono sotto un'altra (es. **30 °C**),
oppure dopo un **timer** (es. 1 h 30 min), oppure **mai**, finché non lo riaccendi tu.
Si può aggiungere un **ritardo dopo la soglia**: a 50 °C parte un timer, per esempio
di 15 minuti, e solo dopo spegne.

Funziona insieme al cronotermostato che hai già (**BPT TH/124.01 BB**), senza
aprirlo e senza toccarne l'elettronica: un relè della centralina va **in serie** al
suo contatto verso la caldaia. Se la centralina si spegne o si guasta, il relè torna
a riposo e **il termostato funziona esattamente come prima**.

```
   termosifone in ghisa                    parete vicino al termostato
  ┌──┬──┬──┬──┬──┬──┐                     ┌──────────────────────────┐
  │  │  │  │  │  │  │    infrarossi         │ CENTRALINA ESP32         │      ┌─────────┐
  │  │  │  │  │  │  │ ◄── cono di 5° ───────│ MLX90614-DCI (sensore IR)│      │ TH/124  │
  │  │  │  │  │  │  │    1-3 m, buio o luce │ relè Finder 34.51        ├─NC───┤ termost.│
  └──┴──┴──┴──┴──┴──┘                       │ pagina web via Wi-Fi     │      └────┬────┘
   niente da installare                     │ alimentatore USB 5 V     │           │
   sul termosifone                          └──────────────────────────┘    2 fili verso la
                                                                            caldaia (morsetti TA)
```

## Le tre configurazioni

| | Sensore | Sul termosifone | Pro | Contro |
|---|---|---|---|---|
| **A (consigliata)** | MLX90614**ESF-DCI**, infrarossi con cono di 5°, montato sulla centralina | **niente** | niente fili, niente batterie, nessuna elettronica al caldo, funziona al buio e alla luce | serve la vista libera sul termosifone; va tarato una volta |
| **B** | sonda DS18B20 a contatto + nodo a batteria **XIAO ESP32-C6** (ESP-NOW) | sonda con magnete, nodo a parete di fianco | precisione del contatto, nessun filo attraverso la stanza | una batteria da ricaricare (circa un anno con una 18650) |
| **C** | sonda DS18B20 a contatto collegata con un cavetto | sonda con magnete | la più semplice ed economica | un cavetto lungo il battiscopa |

La sorgente della temperatura si sceglie dalla pagina web: puoi partire con A e
passare a B o C senza riprogrammare niente.

Il comando verso il termostato può essere il **relè in serie** (consigliato) oppure
un **servo** che sposta un cursore o preme un pulsante (il "ditino"). Il firmware li
gestisce entrambi, ma con il TH/124 il relè è molto più affidabile
([perché](docs/01-architettura.md#relè-o-ditino)).

## Cosa comprare (configurazione A)

| Cosa | Modello preciso |
|---|---|
| Microcontrollore | il tuo **ESP32** (DevKit V1 / DevKitC con ESP32-WROOM-32) |
| Sensore a infrarossi | modulo **GY-906-DCI** con **Melexis MLX90614ESF-DCI** (5°, 3 V). **Non** la GY-906-BAA da 90° |
| Relè | **Finder 34.51.7.005.0010** (bobina 5 V, 6 A 250 V, isolamento 6 kV) + transistor BC337, diodo 1N4148, resistenze da 1 kΩ e 10 kΩ |
| Alimentazione | alimentatore USB **5 V 1 A** marcato CE con il suo cavo (oppure Hi-Link **HLK-PM01** dai fili a 230 V, con fusibile e varistore) |
| Scatola | scatola in plastica di circa 100×70×40 mm, morsetti a vite passo 5 mm |

La lista completa, con alternative, prezzi indicativi e i pezzi delle configurazioni
B e C, è in **[docs/02-componenti.md](docs/02-componenti.md)**.

## Documentazione

1. [Architettura e scelte](docs/01-architettura.md): perché gli infrarossi, perché il relè, alternative valutate (ditino, bimetallo, circuiti analogici o con ATtiny)
2. [Componenti con modelli precisi](docs/02-componenti.md)
3. [Collegamenti e schemi](docs/03-collegamenti.md): relè in serie al TH/124, sensori, servo, nodo a batteria
4. [Alimentazione](docs/04-alimentazione.md): USB, 230 V con protezioni, i tuoi mini buck, durata della batteria
5. [Montaggio](docs/05-montaggio.md): puntamento del sensore IR, sonda con magnete o asticella di legno, calore
6. [Firmware e uso](docs/06-firmware-e-uso.md): caricamento, prima accensione, pagina web, parametri, taratura
7. [Sicurezza e protezioni](docs/07-sicurezza.md)

## Firmware

```
firmware/
  controller/    centralina (ESP32): sensori, logica, relè o servo, pagina web, ricezione ESP-NOW
  sensor_node/   nodo a batteria opzionale (XIAO ESP32-C6/C3 o ESP32): DS18B20 o NTC, deep sleep
  test/          test della logica sul PC:  make -C firmware/test
```

Non servono librerie esterne: basta il core **Arduino-ESP32 3.x** (PlatformIO o
Arduino IDE). La logica di controllo è coperta da test che girano sul PC e la CI di
GitHub compila i firmware veri con PlatformIO a ogni modifica.

> **Sicurezza.** Il contatto del termostato verso la caldaia può essere a bassa
> tensione oppure a **230 V**: misuralo prima di tutto
> ([come](docs/03-collegamenti.md#prima-di-tutto-misura-la-tensione-del-circuito-ta)).
> Nuovi collegamenti fissi a 230 V vanno fatti da un installatore abilitato.
> Questo dispositivo non sostituisce le sicurezze della caldaia.
