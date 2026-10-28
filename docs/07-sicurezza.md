# 7. Sicurezza e protezioni

## Che cosa non è

La centralina **non è un dispositivo di sicurezza**: può solo togliere la richiesta di
calore del termostato, mai aggiungerla. Le sicurezze della caldaia (termostato di
sicurezza, pressostati, valvola di sicurezza) restano le uniche responsabili della
sicurezza dell'impianto. Non aprire la caldaia: il collegamento si fa sul filo del
termostato.

## Protezioni nel comportamento

| Protezione | Come funziona |
|---|---|
| **A prova di guasto** | il relè lavora sul contatto **NC**: senza alimentazione, con l'ESP32 guasto o durante un riavvio, il termostato comanda la caldaia come se la centralina non ci fosse |
| Avvio sicuro | la resistenza da 10 kΩ sulla base del transistor tiene il relè a riposo mentre l'ESP32 si accende; il firmware come prima cosa porta l'uscita su "consentito" |
| Watchdog | se il programma si blocca, l'ESP32 si riavvia da solo (e intanto il relè torna a riposo) |
| Guasto del sensore | senza letture valide per 2 min (15 min per il nodo) applica la scelta fatta nella pagina, per default "lascia comandare il termostato"; LED a lampeggio rapido e avviso |
| Limite di sicurezza | sopra 75 °C (regolabile) blocca sempre, anche durante il ritardo dopo la soglia e l'"acceso forzato" |
| Filtri sulle letture | mediana delle ultime 5 letture IR e conferma di 20 s prima di ogni azione: un disturbo breve non fa scattare il relè |
| Letture scartate | DS18B20: controllo del CRC, del registro di configurazione e del valore di accensione (85 °C); MLX90614: controllo del PEC (CRC) e del bit di errore; valori fuori da -30…120 °C |
| Niente cicli brevi | tra spegnimento e riaccensione c'è l'isteresi delle due soglie (o il timer); a fine timer, se il termosifone è ancora sopra la soglia di spegnimento, la pausa si prolunga |
| Dopo un blackout | riprende la modalità e lo stato del ciclo (pausa o blocco) salvati in memoria |
| Pagina protetta | password facoltativa (utente `admin`) per la pagina e per l'aggiornamento del firmware |

## Protezioni elettriche

**Lato caldaia (circuito TA)**

* Misura la tensione **prima** ([come](03-collegamenti.md#prima-di-tutto-misura-la-tensione-del-circuito-ta))
  e spegni la caldaia prima di scollegare i fili.
* Relè **Finder 34.51**: contatto 6 A 250 V~ e 6 kV di isolamento tra bobina e
  contatti, quindi adatto anche a un circuito TA a 230 V.
* Se il circuito TA è a 230 V: scatola chiusa, cavo a doppio isolamento, almeno 6 mm
  tra le parti a 230 V e quelle a 5 V, morsetti adatti alla tensione.
* **Niente snubber RC** ai capi del contatto: la corrente che lasciano passare può
  far credere alla caldaia che il termostato chieda calore.

**Lato comando**

* **Diodo 1N4148** sulla bobina del relè: senza, il picco allo spegnimento rompe il
  transistor.
* Transistor tra ESP32 e bobina: il pin dell'ESP32 non deve alimentare la bobina.
* Condensatore da 470 µF sul 5 V e 100 nF vicino ai sensori.
* Sonda su cavo lungo: 4,7 kΩ di pull-up, 100 Ω in serie e cavo lontano dai cavi a 230 V.

**Alimentazione a 230 V** (solo se non usi un alimentatore USB)

* Fusibile ritardato **T 500 mA** sulla fase, **varistore 275 V~** tra fase e neutro.
* Modulo Hi-Link in scatola chiusa, fili a 230 V con pressacavo, coperchio avvitato.
* Circuito protetto a monte da magnetotermico e **differenziale**.
* In Italia le nuove prese e derivazioni fisse a 230 V vanno fatte da un installatore
  abilitato (DM 37/2008). L'alternativa più semplice è una presa con un alimentatore
  USB certificato.

**Batteria del nodo**

* Solo celle Li-ion o LiPo **con circuito di protezione** e di marca.
* Fusibile PTC da 0,5 A sul +, attenzione alla polarità (la XIAO non ha protezione
  contro l'inversione).
* Il firmware riduce le misure a una all'ora sotto 3,3 V per non scaricare a fondo la
  cella.
* Nodo e batteria **di fianco** al termosifone, mai sopra né sopra altre fonti di calore.

**Calore**

* Nella configurazione A sul termosifone non c'è nulla.
* Nelle configurazioni B e C sul termosifone c'è solo la sonda (fino a 125 °C), meglio con
  cavo in silicone, fissata con un magnete in ferrite.
