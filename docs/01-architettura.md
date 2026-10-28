# 1. Architettura e scelte

## Che cosa fa

Il termostato ambiente (BPT TH/124.01 BB) conosce solo la temperatura della stanza:
quando chiede calore, la caldaia scalda l'acqua e i termosifoni in ghisa salgono di
temperatura. La ghisa ha molta inerzia: continua a scaldare la stanza a lungo anche
dopo che la caldaia si è fermata. La centralina aggiunge una seconda regola,
basata sulla **temperatura del termosifone**:

* quando il termosifone arriva alla **soglia di spegnimento** (es. 50 °C) il
  riscaldamento viene **bloccato**, subito oppure dopo un **ritardo** (es. 15 min);
* il blocco finisce secondo la **modalità di riaccensione** scelta:
  * **temperatura**: quando il termosifone scende sotto la soglia bassa (es. 30 °C);
  * **timer**: dopo un tempo fisso (es. 90 min = 1 h 30 min);
  * **timer o temperatura**: la prima delle due condizioni;
  * **timer e temperatura**: entrambe (pausa minima e termosifone freddo);
  * **mai**: resta bloccato finché non premi "Riaccendi ora" (pagina web o tasto).

"Riscaldamento consentito" non vuol dire che la caldaia è accesa: vuol dire che
decide il termostato, come se la centralina non ci fosse. Se la stanza è già calda,
il termostato non chiede calore e la caldaia resta spenta.

```
                       T >= soglia di spegnimento (per "conferma" secondi)
  ┌────────────────────┐ ── ritardo = 0 ──────────────────────────► ┌─────────────────────┐
  │ RISCALDAMENTO      │                                            │ PAUSA               │
  │ CONSENTITO         │ ── ritardo > 0 ──► ┌──────────────────┐    │ riscaldamento       │
  │ (comanda il        │                    │ POST-SOGLIA      │──► │ bloccato dal relè   │
  │  termostato)       │                    │ ancora consentito│    └──────────┬──────────┘
  └─────────▲──────────┘                    │ per "ritardo" min│               │
            │                               └──────────────────┘               │
            │  riaccensione: T <= soglia bassa / fine timer / il primo / entrambi
            └──────────────────────────────────────────────────────────────────┤
                                                                               │ modalità "mai"
                                                                    ┌──────────▼──────────┐
                                     "Riaccendi ora" ◄───────────── │ BLOCCATO fino a     │
                                                                    │ sblocco manuale     │
                                                                    └─────────────────────┘
```

Altre regole sempre attive:

* **Conferma soglia** (default 20 s): una soglia deve restare superata per un po'
  prima che la centralina agisca. Filtra i disturbi, per esempio una persona che
  passa davanti al sensore a infrarossi.
* **Limite di sicurezza** (default 75 °C): sopra questo valore blocca sempre,
  anche durante il post-soglia e l'"acceso forzato".
* Se il timer di pausa finisce ma il termosifone è **ancora sopra la soglia di
  spegnimento**, la pausa si prolunga (riaccendere lo farebbe rispegnere subito).

Comandi manuali: **Automatico**, **Solo termostato** (la centralina non interviene),
**Spento forzato**, **Acceso forzato per N minuti** (poi torna in automatico),
**Riaccendi ora**.

## Perché gli infrarossi (configurazione A)

Avevi chiesto un sensore sul termosifone con un'uscita meccanica o a infrarossi,
leggibile a un paio di metri, al buio e alla luce, possibilmente senza alimentazione.
Il termosifone **è già** una sorgente di infrarossi: ogni oggetto emette radiazione
termica nella banda 8-14 µm, tanto più forte quanto è caldo. Un termometro a
termopila come il **Melexis MLX90614** la misura a distanza:

* **sul termosifone non si installa niente**: niente batterie, niente fili, niente
  elettronica al caldo;
* la misura **non dipende dalla luce**: il filtro del sensore lascia passare solo
  l'infrarosso termico, quindi buio, lampade o sole sul sensore non cambiano nulla;
* la centralina sta vicino al termostato, dove deve stare comunque il relè.

### Serve la versione a campo stretto

Il sensore vede un cono. La versione economica (GY-906-**BAA**) ha un cono di 90°:
da 2 m vedrebbe mezzo muro. Serve la **MLX90614ESF-DCI** (cono di 5°) o, se il
termosifone è vicino e grande, la **ESF-BCF** (10°).

| Distanza | Diametro della zona vista, 5° (DCI) | 10° (BCF) | 90° (BAA) |
|---|---|---|---|
| 1 m | 9 cm | 17 cm | 2 m |
| 2 m | 17 cm | 35 cm | 4 m |
| 3 m | 26 cm | 52 cm | 6 m |

Il cono è misurato a metà sensibilità: il termosifone deve coprire almeno il doppio
del diametro in tabella. Un termosifone in ghisa di 60-90 cm di altezza va benissimo
fino a 2-3 m con la DCI.

### Emissività e taratura

Il sensore esce di fabbrica tarato per un corpo nero (emissività 1,0). Una vernice
normale, di qualsiasi colore, bianco compreso, ha emissività 0,90-0,95 nell'infrarosso
termico; tra un elemento e l'altro il sensore vede anche un po' di muro. Il risultato
è che legge qualche grado in meno del vero. La centralina corregge con
un'**emissività efficace**, calcolata automaticamente dalla funzione **Calibra**:
misuri il termosifone caldo una volta con un termometro a contatto e scrivi il valore
nella pagina web.

Attenzione alle **vernici metallizzate (alluminio, argento)**: hanno emissività bassa
(0,3-0,6) e rendono la misura instabile. In quel caso incolla sul termosifone, nel
punto inquadrato, un quadrato di nastro isolante nero opaco di circa 20×20 cm
(emissività ~0,95).

Limiti: il sensore deve **vedere** il termosifone. Tende, mobili o persone davanti
falsano la lettura finché restano lì (mediana e conferma eliminano i passaggi brevi).
Vetro e plastica sono opachi all'infrarosso termico: niente finestrelle davanti al
sensore, e non puntarlo verso una finestra.

Se la vista libera non c'è, usa la configurazione **B** (nodo a batteria) o **C**
(sonda a filo).

## Relè o ditino?

Il TH/124 è un cronotermostato giornaliero **a cursori**: il programma si imposta
con i cursori, e un pulsante alterna funzionamento **manuale e automatico**. Non c'è
un singolo pulsante "accendi/spegni riscaldamento" di cui si possa conoscere lo stato.
Un ditino che preme un pulsante ha due problemi:

* **perde il conto**: se qualcuno tocca il termostato, se una pressione non va a
  segno o se il termostato si riavvia (cambio batterie), la centralina crede acceso
  ciò che è spento e da lì in poi fa il contrario di quello che deve;
* è **meccanico**: va allineato, si sposta, si consuma, fa rumore.

Il **relè in serie** è assoluto (il contatto è chiuso o aperto, senza memoria), non
richiede di aprire o modificare il termostato e, collegato sul contatto **NC**, è
**a prova di guasto**: senza alimentazione il relè è a riposo e il termostato lavora
da solo come oggi.

Il firmware supporta comunque due modalità servo, utili con altri termostati:

* **servo a due posizioni**: sposta un cursore, una manopola o un interruttore tra la
  posizione "consenti" e "blocca" (è assoluto, quindi non perde il conto);
* **servo che preme un pulsante**: ogni pressione inverte lo stato; nella pagina c'è
  "Il termostato adesso è: Acceso / Spento" per riallinearlo a mano.

## Alternative valutate

| Idea | Come funzionerebbe | Valutazione |
|---|---|---|
| Termostato bimetallico a contatto (es. **KSD301** normalmente aperto, 50 °C) | disco bimetallico appoggiato al termosifone: chiude a ~50 °C, riapre a ~35-40 °C | completamente passivo, ma soglia e isteresi le decide il pezzo (non si regolano) e servono comunque due fili fino alla centralina |
| Indicatore meccanico letto da lontano | bandierina mossa da un bimetallo o da un attuatore a cera, letta da una fotocellula a riflessione | solo acceso/spento, delicato da allineare e da regolare |
| "Faro" a infrarossi | termistore NTC + comparatore o ATtiny, LED IR 940 nm a impulsi modulati a 38 kHz, ricevitore TSOP38238 sulla centralina | durerebbe anni con due stilo, ma serve la vista libera esattamente come il sensore IR, che però sul termosifone non richiede nulla |
| **Nodo ESP32 a batteria** (configurazione B) | sonda a contatto, deep sleep, invio ESP-NOW solo quando serve | scelto: una carica dura circa un anno e il segnale radio passa anche dietro ai mobili |
| **Sonda a filo** (configurazione C) | DS18B20 su 2-5 m di cavetto | scelto: il più semplice e affidabile, se il cavetto si può far passare |

## Perché la centralina va alimentata

Con il Wi-Fi acceso un ESP32 consuma intorno ai 100 mA: una batteria da 2500 mAh
durerebbe un giorno. La centralina deve restare sveglia per la pagina web, per
ricevere il nodo a batteria e per comandare il relè in qualsiasi momento.
Una centralina a batteria sarebbe possibile (deep sleep, relè bistabile, Wi-Fi solo su
richiesta) ma perderebbe la pagina web sempre disponibile e la ricezione del nodo;
non vale la pena se a pochi metri c'è una presa o se puoi portare i 230 V.
La batteria resta solo dove serve davvero: il nodo opzionale della configurazione B
(vedi [alimentazione](04-alimentazione.md#durata-della-batteria-del-nodo)).

## Che cosa succede se...

| Evento | Comportamento |
|---|---|
| La centralina si spegne, si blocca o si riavvia | il relè va a riposo: **il termostato funziona da solo**. Un watchdog riavvia l'ESP32 se il programma si blocca. |
| Il sensore smette di rispondere | dopo 2 min (sensore locale) o 15 min (nodo a batteria) scatta il "guasto sensore": per default **lascia comandare il termostato** (si può scegliere "blocca"). LED a lampeggio rapido e avviso nella pagina. |
| Manca il Wi-Fi di casa | la regolazione continua normalmente (non dipende dalla rete). Dopo 1 minuto si apre anche la rete di configurazione "Termosifone-XXXX". |
| Torna la corrente dopo un blackout | riprende la modalità e lo stato del ciclo (pausa o blocco). I timer in corso ripartono da zero. |
