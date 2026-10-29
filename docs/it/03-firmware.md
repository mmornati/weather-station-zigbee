# 03 · Firmware

🌐 [English](../03-firmware.md) · [Français](../fr/03-firmware.md) · **Italiano**

## Compilare e flashare

### Dal browser (il modo più semplice)

Apri la [sezione flash del sito](https://mmornati.github.io/weather-station-zigbee/?lang=it#flash) con Chrome o Edge su un computer, collega lo XIAO in USB-C (batteria scollegata) e clicca **Collega & installa**. Scegli **Erase device** solo alla prima installazione. La pagina offre sempre il firmware compilato dall'ultimo `main`.

### PlatformIO

```bash
cd firmware
pio run -t erase && pio run -t upload
pio device monitor
```

Cancella solo al primo flash (cancella anche il totale di pioggia salvato). Aggiornamenti successivi: `pio run -t upload`.

### IDE Arduino

1. Installa il core **esp32** di Espressif, 3.3.0 o successivo (gestore schede).
2. Installa queste librerie (gestore librerie), anche per i moduli che non monti, perché lo sketch le include: **Adafruit BME280 Library**, **Adafruit SHT4x Library**, **Adafruit LTR390 Library**, **OneWire**, **DallasTemperature**, **SparkFun AS3935 Lightning Detector Arduino Library**.
3. Apri `firmware/weather_station/weather_station.ino` e imposta il menu **Strumenti**:

| Impostazione | Valore |
|---|---|
| Scheda | XIAO_ESP32C6 |
| Zigbee mode | Zigbee ED (end device) |
| Partition Scheme | Zigbee 4MB with spiffs |
| Erase All Flash Before Sketch Upload | Enabled (solo al primo flash) |

## Come funziona

La stazione è uno **sleepy end device** Zigbee: passa ~94 % della sua vita in deep sleep e si sveglia per tre motivi.

| Risveglio | Cosa fa | Durata |
|---|---|---|
| La vaschetta bascula (reed su `D1`) | Conta la basculata nella memoria RTC, torna a dormire | ~0,3 s |
| Ogni 60 s | Legge la banderuola, conta gli impulsi dell'anemometro per 3 s in light sleep (anche la pioggia) | ~3,3 s |
| Ogni 5 minuti, o al minuto successivo se c'è nuova pioggia | Legge tutti i sensori, avvia lo Zigbee, invia tutto, aspetta le conferme | ~2 s |

Ogni invio contiene:

- **velocità del vento**: media delle cinque finestre da 3 s dall'ultimo invio;
- **raffica**: la finestra da 3 s migliore;
- **direzione**: media vettoriale pesata sulla velocità (350° e 10° danno 0°, non 180°; con calma di vento resta l'ultima direzione);
- **pioggia**: totale dall'installazione in mm, salvato in flash quando cambia, quindi sopravvive ai cambi di batteria.

Il vento è *campionato*: 3 s ogni 60. Dà una buona immagine su 5 minuti ma può perdere una raffica breve tra due finestre. Il conteggio continuo sul core a basso consumo del C6 è previsto per una v2.

### Dopo l'accensione o una pressione su RESET

1. I moduli opzionali vengono rilevati (scritti sul monitor seriale).
2. Il LED lampeggia lentamente mentre la stazione cerca una rete, fino a 3 minuti: apri *Permit join* in Zigbee2MQTT.
3. Una volta entrata, invia i dati e **resta sveglia 2 minuti**, reinviando ogni 30 s, così Zigbee2MQTT può fare l'interview e configurarla. È anche il momento di leggere i log, perché l'USB sparisce in deep sleep.
4. Poi parte il ciclo normale.

**Uscire dalla rete**: premi RESET, poi tieni premuto **BOOT** per 3 s mentre il LED lampeggia (lampeggio veloce = rilascia). Il totale di pioggia resta.

**Hai aggiunto un modulo dopo?** Premi RESET perché venga rilevato, poi in Zigbee2MQTT lancia *Interview* e *Reconfigure* durante i 2 minuti di veglia, così Z2M scopre il nuovo endpoint.

### Endpoint Zigbee

| EP | Cluster | Proprietà Z2M | Unità | Quando |
|---|---|---|---|---|
| 1 | Analog Input | `wind_speed` | km/h | sempre |
| 1 | Power Configuration | `battery` | % | sempre |
| 2 | Analog Input | `wind_gust` | km/h | sempre |
| 3 | Analog Input | `wind_direction` | ° | sempre |
| 4 | Analog Input | `rain` | mm | sempre |
| 5 | Analog Input | `uv_index` | | LTR390 |
| 6 | Analog Input | `lightning_distance` | km | AS3935 |
| 7 | Analog Input | `lightning_count` | | AS3935 |
| 10 | Temperature + Humidity | `temperature`, `humidity` | °C, % | SHT45 o BME280 |
| 11 | Pressure | `pressure` | hPa (al livello della stazione) | BME280 |
| 12 | Illuminance | `illuminance` | lx | LTR390 |
| 13 | Temperature | `ground_temperature` | °C | DS18B20 su `D10` |
| 14 | Temperature | `soil_temperature` | °C | DS18B20 su `D4` |
| 15 | Temperature | `box_temperature` | °C | SHT45 **e** BME280 |

## Impostazioni

Tutte nel blocco `Behaviour` in cima allo sketch.

| Define | Predefinito | Cosa |
|---|---|---|
| `TICK_MS` | 60000 | Tempo tra due finestre del vento |
| `WIND_WINDOW_MS` | 3000 | Durata di una finestra del vento |
| `REPORT_EVERY_TICKS` | 5 | Tick tra due invii (5 = ogni 5 min) |
| `UV_WINDOW_FACTOR` | 1.0 | Aumentalo se il disco in PTFE nasconde parte degli UV (confronta con una previsione UV in una giornata limpida) |
| `HUMIDITY_HEATER_RH` | 95 | Sopra questo valore, un impulso di riscaldamento da 1 s asciuga l'SHT45 dopo la lettura, al massimo una volta all'ora |
| `JOIN_TIMEOUT_PAIRING_MS` | 180000 | Per quanto cerca una rete dopo un reset |

Invii più frequenti costano batteria: l'invio Zigbee è la voce più grande del [bilancio energetico](01-hardware-and-wiring.md#alimentazione).

La calibrazione (scostamento del nord della banderuola, altitudine per la pressione al livello del mare) resta in Home Assistant: la stazione dorme, quindi non può ricevere impostazioni in modo affidabile.

## Test

`pio test -e native` esegue i test unitari di `weather_logic.h` sull'host: decodifica della banderuola, media/raffica/direzione del vento, antirimbalzo del pluviometro, orologio attraverso il deep sleep, curva della batteria, conversioni di illuminamento e UV.

Avanti: [Zigbee2MQTT & Home Assistant](04-zigbee2mqtt-and-home-assistant.md).
