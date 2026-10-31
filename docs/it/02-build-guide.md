# 02 · Montaggio passo passo

🌐 [English](../02-build-guide.md) · [Français](../fr/02-build-guide.md) · **Italiano**

Segui questi passi in ordine. Ogni parte viene provata sul banco prima di andare all'aperto, così un problema si vede alla scrivania e non in cima a un palo.

<p align="center"><img src="../../site/img/station.png" alt="La stazione finita: un palo da 3 m con il braccio del vento in cima, lo schermo solare, il pannello e la scatola, e il pluviometro sul suo paletto" width="380"> <img src="../../site/img/head.png" alt="Primo piano dello schermo sul suo braccio, del pannello sul supporto inclinabile e della scatola con i pressacavi" width="300"></p>

## Cosa ti serve

- Tutto quello che c'è nell'[elenco dei componenti](01-hardware-and-wiring.md#componenti) (circa 170–200 € con palo e filamento).
- Una stampante 3D con piano da 220 × 220 mm che stampi PETG, un saldatore con punta per inserti a caldo, un multimetro.
- Un cavo USB-C, una breadboard e qualche cavetto per la prova al banco.
- Facoltativo ma utile: un misuratore USB o un Nordic PPK2 per verificare la corrente in sonno, vernice protettiva per la scheda.

I file che userai:

| Cosa | File |
|---|---|
| Firmware | [`firmware/weather_station/weather_station.ino`](../../firmware/weather_station/weather_station.ino), compilato con [`firmware/platformio.ini`](../../firmware/platformio.ini) |
| Parti 3D | [`hardware/3d/`](../../hardware/3d): un file `.scad` per parte, [`build_stl.sh`](../../hardware/3d/build_stl.sh) genera tutti gli STL |
| Converter Zigbee2MQTT | [`zigbee2mqtt/weather_station.mjs`](../../zigbee2mqtt/weather_station.mjs) |
| Package Home Assistant | [`homeassistant/weather_station.yaml`](../../homeassistant/weather_station.yaml) |

## 1. Controlla i cavi del kit

I kit meteo cambiano da un lotto all'altro: misura prima di saldare qualsiasi cosa.

1. Cavo del vento (quello dell'anemometro, con la banderuola collegata): multimetro in continuità su ogni coppia di fili della spina RJ11, e gira lentamente le coppette. La **coppia dell'anemometro** fa un bip a ogni giro.
2. Sempre sul cavo del vento, passa in ohm e gira la banderuola. La **coppia della banderuola** legge tra circa 688 Ω e 120 kΩ a seconda della direzione.
3. Cavo della pioggia: la **coppia del pluviometro** fa bip ogni volta che fai basculare la vaschetta a mano.
4. Annota le tre coppie. Ti serviranno per le prese RJ11 sulla scheda.

## 2. Controlla il caricatore

1. Collega il bq24074 alla 18650 (BAT) e al pannello solare (IN). Fissa l'NTC da 10 kΩ alla cella con del nastro e collegalo a TS.
2. Metti il pannello al sole. Il LED CHG si accende e la tensione della cella sale lentamente.
3. Solo allora prosegui: il caricatore è l'unico pezzo difficile da diagnosticare una volta cablato tutto.

## 3. Prova al banco

1. Flasha il firmware con lo XIAO su USB, dal [sito web](https://mmornati.github.io/weather-station-zigbee/?lang=it#flash) o con PlatformIO ([come](03-firmware.md#compilare-e-flashare)). Scegli la cancellazione al primo flash.
2. Su una breadboard collega il kit, l'SHT45 e il BME280 come nello [schema di collegamento](01-hardware-and-wiring.md#schema-di-collegamento). Lascia fuori il caricatore: l'USB alimenta tutto.
3. Copia il [converter](../../zigbee2mqtt/weather_station.mjs) in `external_converters/` di Zigbee2MQTT e riavvia Z2M. Apri *Permit join*.
4. Apri il monitor seriale (`pio device monitor`) e premi **RESET**. Devi vedere i moduli rilevati, la stazione che entra in Zigbee2MQTT e un primo invio.
5. Durante i 2 minuti in cui la stazione resta sveglia dopo un reset, gira le coppette e fai basculare la vaschetta: l'invio successivo mostra vento e pioggia.
6. Gira la banderuola in ogni direzione e controlla `wind_direction` in Zigbee2MQTT.

Se manca qualcosa, guarda la [Risoluzione problemi](05-troubleshooting.md) prima di saldare.

## 4. Stampa le parti

Tutte le parti sono file OpenSCAD in [`hardware/3d`](../../hardware/3d). I valori predefiniti vanno bene per un palo da 40 mm e un pannello da 136 × 110 mm. Per cambiarli, modifica `pole_d` in [`common.scad`](../../hardware/3d/common.scad) o `panel` in [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad), poi lancia `hardware/3d/build_stl.sh`. Gli STL pronti sono sul [sito web](https://mmornati.github.io/weather-station-zigbee/?lang=it#parts) e in ogni release.

| Parte | File (`part=`) | Q.tà | Orientamento | Impostazioni |
|---|---|---|---|---|
| <img src="../../site/img/part_shield_plate.png" width="90" alt=""> Piatto dello schermo | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`plate`) | 9 | Come esportato (anello piatto sotto, gonna a 45° in alto) | Bianco, 2 perimetri, 15 % |
| <img src="../../site/img/part_shield_cap.png" width="90" alt=""> Cappello dello schermo | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`cap`) | 1 | Come esportato | Bianco, 2 perimetri, 15 % |
| <img src="../../site/img/part_shield_base.png" width="90" alt=""> Base + braccio | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`base`) | 1 | In piano | 4 perimetri, 30 % |
| <img src="../../site/img/part_shield_spacer.png" width="50" alt=""> Distanziale | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`spacer`) | 30 | In piedi | 100 % |
| <img src="../../site/img/part_box.png" width="90" alt=""> Scatola elettronica | [`electronics_box.scad`](../../hardware/3d/electronics_box.scad) (`box`) | 1 | Sul dorso | 4 perimetri, 30 % |
| <img src="../../site/img/part_box_lid.png" width="90" alt=""> Coperchio | [`electronics_box.scad`](../../hardware/3d/electronics_box.scad) (`lid`) | 1 | In piano | 4 perimetri, 30 % |
| <img src="../../site/img/part_panel_frame.png" width="90" alt=""> Telaio del pannello | [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad) (`frame`) | 1 | In piano | 4 perimetri, 30 % |
| <img src="../../site/img/part_panel_cheeks.png" width="90" alt=""> Fianchi del pannello | [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad) (`cheeks`) | 1 | Piastra di base sotto | 4 perimetri, 40 % |
| <img src="../../site/img/part_pole_saddle.png" width="90" alt=""> Staffa per il palo | [`pole_saddle.scad`](../../hardware/3d/pole_saddle.scad) | 4 | Faccia piana sotto | 6 perimetri, 40 % |
| <img src="../../site/img/part_angle_bracket.png" width="80" alt=""> Squadretta | [`angle_bracket.scad`](../../hardware/3d/angle_bracket.scad) | 2 | Di fianco | 6 perimetri, 50 % |
| <img src="../../site/img/part_rain_plate_top.png" width="90" alt=""> Piastra pluviometro, sopra | [`rain_gauge_plate.scad`](../../hardware/3d/rain_gauge_plate.scad) (`top`) | 1 | In piano | 4 perimetri, 40 % |
| <img src="../../site/img/part_rain_plate_base.png" width="90" alt=""> Piastra pluviometro, base | [`rain_gauge_plate.scad`](../../hardware/3d/rain_gauge_plate.scad) (`base`) | 1 | In piano | 4 perimetri, 40 % |

- **Materiale**: PETG (235–245 °C, piano 75–80 °C) di colore chiaro, oppure ASA se l'hotend regge ~250 °C (l'hotend originale della Ender 3 V2 Neo ha il tubo in PTFE). Niente PLA: al sole d'estate si ammorbidisce. Tutte le parti stanno su un piano da 220 × 220 mm, con layer da 0,2 mm e senza supporti.
- **Stampa prima una prova**: una staffa e il coperchio. Verifica che la staffa appoggi bene sul tuo palo e che gli inserti entrino nei fori.
- **Inserti a caldo**: M4 × 6 nelle staffe e nelle torrette del coperchio, M3 nelle torrette della scheda, M5 (o dadi) nella base della piastra del pluviometro.

## 5. Salda la scheda

Tutto quello che non è un modulo va su una millefori da 70 × 50 mm insieme allo XIAO. Segui lo [schema di collegamento](01-hardware-and-wiring.md#schema-di-collegamento) e la [tabella dei pin](01-hardware-and-wiring.md#pin-usati-dal-firmware).

1. Salda lo XIAO su strip femmina, così puoi toglierlo e flasharlo alla scrivania.
2. Salda le due prese RJ11 lungo un bordo, verso i pressacavi: RJ11 n. 1 per il cavo del vento, n. 2 per quello della pioggia, collegate alle coppie trovate al passo 1.
3. Aggiungi i componenti attorno allo XIAO:
   - 100 kΩ da `3V3` a `D1` e 100 nF da `D1` a `GND` (pioggia);
   - 100 kΩ da `3V3` a `D3` e 10 nF da `D3` a `GND` (anemometro);
   - 10 kΩ da `D9` a `D2` (banderuola; l'altro filo della banderuola va a `GND`);
   - 1 MΩ dal `+` della batteria a `D0`, 1 MΩ da `D0` a `GND`, e 100 nF in parallelo a quella in basso (tensione della batteria).
4. Aggiungi un connettore a 4 pin per il bus I2C (`3V3`, `GND`, `D7` SDA, `D8` SCL). Ci si collegano il cavo dell'SHT45 verso lo schermo e il BME280; l'LTR390 e l'AS3935 opzionali vanno sullo stesso bus.
5. Sonde DS18B20 opzionali: data su `D10` (livello dell'erba) e `D4` (suolo), ciascuna con un pull-up da 4,7 kΩ verso `3V3`.
6. Salda due fili sui pad **BAT+** e **BAT−** dello XIAO, sotto la scheda, per la cella.
7. Verifica col multimetro che `3V3` e `GND` non siano in corto, poi collega l'USB e ripeti la prova al banco con la scheda.
8. Facoltativo: spruzza vernice protettiva sulla scheda, evitando le prese RJ11 e i connettori.

## 6. Monta la scatola

<img src="../../site/img/part_box.png" alt="La scatola stampata con quattro fori per i pressacavi e un foro per lo sfiato sulla faccia inferiore" width="320" align="right">

1. Inserisci a caldo gli inserti M3 nelle quattro torrette della scheda e gli M4 nelle quattro torrette del coperchio.
2. Avvita quattro **pressacavi PG7** e il **tappo di sfiato M12** sulla faccia inferiore. Pressacavi, da sinistra a destra: vento, pioggia, sensori dello schermo, pannello solare.
3. Avvita la scheda sulle torrette, con le prese RJ11 verso i pressacavi.
4. Metti il portabatteria 18650 nella culla in alto, il bq24074 e il BME280 accanto. Fissa l'NTC alla cella con il nastro.
5. Collega l'uscita BAT del bq24074 al portabatteria e ai pad BAT dello XIAO. Per ora lascia fuori la cella.
6. Fai passare i cavi nei pressacavi: i cavi RJ11 del kit si infilano direttamente nelle prese, il cavo a 4 fili dello schermo va al connettore I2C, il pannello all'ingresso IN del caricatore.
7. Premi il cordino in silicone da 2 mm nella gola del bordo, tutto attorno, e taglialo in modo che le estremità si tocchino appena.
8. Sistema l'antenna all'interno, dal lato del coperchio, lontano dalla batteria.

Lascia il coperchio aperto finché la stazione non è sul palo: dovrai premere RESET una volta installata.

## 7. Monta lo schermo solare

<img src="../../site/img/shield.png" alt="Vista in sezione dello schermo: nove piatti su tre barre con distanziali, l'SHT45 in una gabbia forata al centro della pila" width="360" align="right">

1. Infila tre **barre filettate M5 inox** (circa 200 mm) nella base e bloccale sotto con dado e rondella.
2. Infila un **distanziale**, poi un **piatto** (gonna verso il basso), su tutte e tre le barre. Ripeti nove volte.
3. Chiudi con il **cappello**. Se monti l'LTR390, avvitalo prima sotto la finestra del cappello e incolla il disco in PTFE da 25 mm nella sede sopra.
4. Stringi un dado su ogni barra sopra il cappello. Non schiacciare i piatti: basta un serraggio leggero.
5. Fai passare l'SHT45, su un corto cavo a 4 fili, dal foro della base fino alla gabbia forata, in modo che resti al centro della pila senza toccare le pareti.

La gonna di ogni piatto copre lo spazio sotto: né il sole diretto né la pioggia raggiungono il sensore, mentre l'aria circola liberamente.

## 8. Installala

Scegli un punto aperto del giardino, ad almeno 3 m da muri, siepi e pavimentazioni, e lontano dall'aria calda della casa.

| Altezza | Cosa | Come |
|---|---|---|
| 3 m | Braccio del vento (del kit) | Con il morsetto del kit, in cima al palo. Punta il segno **N** della banderuola verso nord (basta la bussola del telefono) |
| 1,5 m | Schermo solare | Su una squadretta, su una staffa, sul lato **nord** del palo |
| 1,3 m | Pannello solare | Su una staffa, verso **sud**, inclinato di circa **60°** |
| 1,2 m | Scatola elettronica | Su una staffa, pressacavi **in basso** |
| ~1 m | Pluviometro | Sul suo paletto a circa 1 m, su una squadretta e la piastra di livellamento |

1. Pianta il palo in verticale (un picchetto da terra o un blocco di cemento). Controllalo con una livella: un palo storto fa sbagliare la banderuola.
2. Monta le staffe con due fascette inox ciascuna, alle altezze sopra.
3. **Schermo**: imbullona una squadretta alla sua staffa con 4 × M4 × 12 negli inserti, poi il braccio dello schermo sul ripiano della squadretta con 4 × M4 × 20 e dadi autobloccanti.
4. **Pannello**: imbullona i fianchi alla loro staffa, metti il telaio tra i due con un bullone M4 come perno e un bullone M4 nell'asola ad arco, regola a circa 60° e stringi.
5. **Scatola**: imbullonala alla sua staffa dall'interno con 4 × M4 × 12.
6. **Pluviometro**: sul suo paletto, staffa, squadretta, poi la base della piastra sul ripiano. Avvita il pluviometro alla piastra superiore, metti le tre viti M5 con le molle tra le due piastre e regolale finché la bolla è al centro.
7. Sonde opzionali: una DS18B20 5 cm sopra l'erba, fuori dallo schermo; l'altra 10 cm sotto terra.
8. Collega i cavi, metti la cella nel portabatteria e premi **RESET** con *Permit join* aperto in Zigbee2MQTT (la stazione conserva l'abbinamento fatto al banco, quindi rientra semplicemente).
9. Chiudi il coperchio con 4 × M4 × 12. Stringi i pressacavi attorno ai cavi.

## 9. Controlli finali

| Fai questo | Ti aspetti |
|---|---|
| Guarda Zigbee2MQTT per 10 minuti | Un invio ogni 5 minuti, `battery` sensata |
| Gira le coppette a mano | `wind_speed` e `wind_gust` sopra 0 nell'invio successivo |
| Versa lentamente un bicchiere d'acqua nel pluviometro | `rain` sale di 0,28 mm per basculata |
| Confronta `temperature` con un riferimento all'ombra | Entro circa 0,5 °C |
| Guarda la mappa di Z2M | La stazione è un **end device** con un router padre |
| Ritorna dopo una giornata di sole | La batteria resta al 100 % o quasi |

Poi aggiungi il [package Home Assistant](04-zigbee2mqtt-and-home-assistant.md#package-home-assistant) e imposta la tua altitudine.

Avanti: [Firmware](03-firmware.md).
