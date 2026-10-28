# 5. Montaggio

## La centralina

* Sulla parete **accanto al termostato**, entro 0,5-1 m, perché il relè va sul filo che
  dal termostato va alla caldaia.
* **Di fianco** al termostato, ad almeno 20-30 cm, e non subito sotto: l'ESP32 e
  l'alimentatore scaldano un poco (mezzo watt o più) e l'aria calda che sale falserebbe
  la temperatura letta dal termostato.
* Lontana da sole diretto e altre fonti di calore, dove arriva il Wi-Fi, con la presa
  USB raggiungibile.
* Fissala con due tasselli o con biadesivo forte. I fori di ventilazione non servono.

## Sensore a infrarossi (configurazione A)

```
                   paraluce: tubo Ø 16-20 mm, lungo 3-5 cm,
   scatola         interno nero opaco
  ┌─────────┐    ┌───────────┐
  │         ├────┤ MLX90614  │ ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ►  centro del corpo
  │  ESP32  │    └───────────┘          cono di 5°                       del termosifone
  │  relè   │                           (a 2 m: zona di ~17 cm)
  └─────────┘
```

* **Vista libera** sul termosifone, a 1-3 m, possibilmente di fronte. Punta al **centro
  del corpo** del termosifone, non al bordo superiore (lì il cono vedrebbe anche il muro).
* **Paraluce**: il sensore sta in fondo a un corto tubo con l'interno annerito. Riduce
  l'infrarosso che arriva di lato e le correnti d'aria sul sensore, che lo
  disturbano.
* **Niente davanti al sensore**: plastica, vetro e plexiglas sono opachi all'infrarosso
  termico. Il tubo esce da un foro della scatola.
* Il MLX90614 non ama i cambi di temperatura del proprio corpo: nella scatola mettilo in
  basso e lontano dall'alimentatore, oppure in una scatoletta a parte collegata con un
  cavetto a 4 poli (massimo 1 m).
* Per orientarlo bene usa un piccolo **snodo a sfera** (quelli per action cam con vite
  da 1/4"), oppure fissalo con colla a caldo dopo averlo puntato.

Per puntarlo:

1. con il termosifone caldo, apri la pagina web e guarda la **lettura grezza**;
2. muovi piano il sensore: la posizione giusta è quella con la lettura **più alta e
   stabile**;
3. in alternativa fissa per un momento un puntatore laser parallelo al tubo e punta il
   centro del termosifone.

Se il termosifone è verniciato con **vernice all'alluminio o argentata**, o se la
lettura oscilla, incolla un quadrato di **nastro isolante nero opaco** di circa
20×20 cm nel punto inquadrato. Evita di avere nel cono finestre, lampade, la tenda o
altre fonti di calore.

Poi **tara** il sensore: vedi [firmware e uso](06-firmware-e-uso.md#taratura-del-sensore-a-infrarossi).

## Sonda a contatto (configurazioni B e C)

**Dove:** nella parte alta del termosifone, dal lato dove entra l'acqua calda (è il
punto più caldo). Sul **tubo di mandata** la sonda reagisce prima ma legge di più: in
quel caso alza un po' le soglie.

**Metodo consigliato, con il magnete** (la ghisa è magnetica):

```
     termosifone (ghisa)
  ══════════════════════════
     ▓ pasta termoconduttiva
    ┌───────────────┐
    │ sonda DS18B20 │  ← tenuta contro la ghisa dal magnete
    └───────────────┘
   ┌─────────────────┐
   │ magnete ferrite │
   └─────────────────┘
   ░░░ neoprene o gommapiuma sopra tutto, per isolare dall'aria della stanza
```

* Una goccia di pasta termoconduttiva tra puntale e ghisa.
* Magnete a tazza in **ferrite**: quelli al neodimio normali si smagnetizzano sopra 80 °C.
* Copri sonda e magnete con un pezzetto di neoprene o gommapiuma: senza, la sonda
  legge una via di mezzo tra termosifone e aria.
* Si toglie in un secondo e non lascia segni.

In alternativa: sonda stesa lungo un elemento, fissata con due o tre giri di **nastro di
alluminio** adesivo, e sopra un pezzetto di isolante.

Il cavo: fissalo con clip o fascette senza lasciarlo appoggiato alla ghisa più del
necessario. Un cavo in PVC regge 70-80 °C; meglio la sonda con **cavo in silicone**.

**Con l'asticella di legno**, se preferisci non attaccare niente al termosifone o vuoi
tutta l'elettronica sulla parete:

```
   parete                                    termosifone
     │                                         ┌──┐
     ├────┐ staffa a L                         │  │
     │    │ molla                              │  │
     │    ├~~~~┬══════════════════════════════►│  │  punta in sughero o legno con
     │    │    │  tondino di faggio Ø 10-12 mm │  │  una scanalatura per la sonda
     ├────┘    (non conduce calore)            │  │  + una goccia di pasta termica
     │                                         └──┘
```

* Tondino di faggio da 10-12 mm, lungo quanto serve, infilato in un foro della staffa.
* Una piccola **molla a compressione** dietro (o la flessione del legno stesso) spinge la
  punta contro il termosifone e mantiene il contatto anche quando la ghisa si dilata.
* Il legno conduce pochissimo il calore (circa 0,15 W/m·K, contro gli oltre 40 della
  ghisa): l'elettronica sulla parete resta a temperatura ambiente.

## Nodo a batteria (configurazione B)

```
            ┌───────┐
            │ nodo  │ ◄── sulla parete, DI FIANCO al termosifone (almeno 20-30 cm),
            └───┬───┘     a metà altezza o più in basso
                │ cavo della sonda 0,5-1 m
     ┌──┬──┬──┬─┴┬──┬──┐
     │  │  │  │ ●│  │  │ ◄── sonda con magnete nella parte alta
     │  │  │  │  │  │  │
     └──┴──┴──┴──┴──┴──┘
```

* **Mai sopra il termosifone**: l'aria calda che sale da un termosifone a 60 °C supera
  facilmente i 40 °C, troppo per una batteria al litio. Mai sul termosifone.
* Niente scatole metalliche: schermano la radio. Fino a 10 m dalla centralina, anche
  con un muro in mezzo, l'ESP-NOW arriva senza problemi.
* Alla prima accensione il LED della XIAO lampeggia **3 volte lentamente** se la
  centralina ha risposto, **10 volte velocemente** se non l'ha trovata (centralina spenta,
  troppo lontana o con un identificativo d'impianto diverso).

## Temperature massime delle parti

| Parte | Limite | Dove sta |
|---|---|---|
| ESP32, XIAO | -40…85 °C | parete |
| MLX90614 (versione ESF) | -40…85 °C | centralina |
| DS18B20 | -55…125 °C | sul termosifone |
| Cavo in PVC | 70-80 °C | — |
| Cavo in silicone | 180-200 °C | — |
| Batteria Li-ion | carica 0…45 °C, uso -20…60 °C | parete, di fianco |
| Magnete in ferrite | ~250 °C | sul termosifone |
| Magnete al neodimio normale (grado N) | 80 °C | da evitare |

Un termosifone in ghisa con una caldaia tradizionale arriva di solito a 60-75 °C.
