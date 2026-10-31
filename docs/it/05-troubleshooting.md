# 05 · Risoluzione problemi

🌐 [English](../05-troubleshooting.md) · [Français](../fr/05-troubleshooting.md) · **Italiano**

Parti dal monitor seriale subito dopo una pressione su **RESET**: nei primi 2 minuti la stazione scrive i moduli trovati, l'ingresso in rete e ogni invio. Dopo è in deep sleep e l'USB sparisce.

## Il LED lampeggia per 3 minuti, poi si ferma

La stazione non ha trovato una rete.

- Apri *Permit join* in Zigbee2MQTT **prima** di premere RESET.
- Era abbinata a un'altra rete: premi RESET, tieni premuto BOOT per 3 s, poi abbinala di nuovo.
- Portata: uno sleepy end device ha bisogno di un **router** (qualsiasi dispositivo Zigbee alimentato a rete) a portata. Collega l'antenna U.FL e tienila lontana dal metallo.

## Entrata in rete, ma nessun valore in Zigbee2MQTT

- Il converter non è caricato: controlla che Z2M mostri il modello `C6WeatherStation` come supportato.
- L'interview non è finita prima che si addormentasse: premi RESET e lancia *Reconfigure* in Z2M durante i 2 minuti di veglia.

## Vento sempre a 0

- Coppia RJ11 sbagliata: trova con il multimetro la coppia dell'anemometro (un bip per giro).
- Manca il pull-up su `D3`.

## Direzione del vento bloccata o assente

- `wind_direction` non compare mai: la banderuola risulta aperta o in corto. Controlla la 10 kΩ verso `D9` e la coppia (da 688 Ω a 120 kΩ tra i due fili).
- Sbagliata di un angolo fisso: il segno **N** della banderuola non punta a nord.

## La pioggia conta troppo o troppo poco

- Qualche basculata in più nei giorni di vento: la vaschetta non è in bolla. Usa la bolla della piastra.
- Confronta con un pluviometro manuale per qualche settimana; gli 0,2794 mm per basculata del kit sono un valore nominale. Se serve, cambia `MM_PER_TIP` in `weather_logic.h`.

## Temperatura troppo alta al sole

- L'SHT45 deve stare nella gabbia dello schermo, non contro un piatto, e lo schermo deve essere bianco.
- Tieni lo schermo ad almeno 1 m dalla scatola, dal pannello e da qualsiasi muro.

## La batteria si scarica

- Controlla che il bq24074 carichi (il suo LED CHG al sole) e che il pannello guardi a sud.
- Sotto 0 °C il bq24074 non carica, di proposito. La cella porta la stazione attraverso i periodi di freddo.
- Misura la corrente in sonno: più di ~100 µA vuol dire che qualcosa resta alimentato (il LED di un modulo, un pull-up verso un reed chiuso).

## Modulo opzionale non trovato

I moduli vengono rilevati solo all'accensione: premi RESET dopo averne collegato uno. Controlla l'indirizzo: SHT45 0x44, BME280 0x76/0x77, LTR390 0x53, AS3935 0x03 (entrambi i pin di indirizzo alti).

## Le parti stampate non combaciano

- La staffa non appoggia bene: il tuo palo non ha il `pole_d` di [`common.scad`](../../hardware/3d/common.scad). Cambialo e ristampa.
- Gli inserti entrano troppo facilmente o non entrano: regola `m4_insert_d` in `common.scad` per i tuoi inserti.
- Il pannello non si aggancia nel telaio: metti le sue misure reali in `panel` di [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad).
