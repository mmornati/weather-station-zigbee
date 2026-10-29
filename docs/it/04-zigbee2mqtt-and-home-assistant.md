# 04 · Zigbee2MQTT & Home Assistant

🌐 [English](../04-zigbee2mqtt-and-home-assistant.md) · [Français](../fr/04-zigbee2mqtt-and-home-assistant.md) · **Italiano**

## Zigbee2MQTT

1. Copia [`zigbee2mqtt/weather_station.mjs`](../../zigbee2mqtt/weather_station.mjs) in `<dati z2m>/external_converters/` e riavvia Zigbee2MQTT.
2. Apri *Permit join*, premi **RESET** sulla stazione e aspetta che l'interview finisca (resta sveglia 2 minuti).
3. Rinominala **`weather_station`**: il package di Home Assistant si aspetta questo nome.

Vengono esposti solo i sensori che la stazione ha davvero: un modulo opzionale compare appena esiste il suo endpoint.

La stazione è un end device a batteria: il controllo di disponibilità di Zigbee2MQTT la segna offline solo dopo molte ore. Il package qui sotto ha il suo avviso «stazione muta» a 30 minuti.

## Package Home Assistant

Copia [`homeassistant/weather_station.yaml`](../../homeassistant/weather_station.yaml) in `/config/packages/` e riavvia. Aggiunge:

| Entità | Cosa |
|---|---|
| `sensor.weather_wind_speed`, `sensor.weather_wind_gust` | km/h con la device class `wind_speed` |
| `sensor.weather_wind_direction` | gradi, attributo `cardinal` (N, NNE…) |
| `sensor.weather_beaufort` | Forza Beaufort |
| `sensor.weather_rain_total` | mm, `total_increasing` |
| `sensor.weather_rain_today`, `_this_month`, `_this_year` | utility meter |
| `sensor.weather_rain_last_15_min`, `sensor.weather_rain_last_24_h` | totali mobili |
| `sensor.weather_rain_rate` | mm/h su 15 minuti |
| `binary_sensor.weather_raining` | una basculata negli ultimi 15 minuti |
| `sensor.weather_dew_point`, `sensor.weather_feels_like` | punto di rugiada (Magnus); wind chill / indice di calore |
| `sensor.weather_sea_level_pressure` | usa `input_number.weather_station_altitude` |
| `binary_sensor.weather_frost` | sonda a livello dell'erba ≤ 0 °C, o aria ≤ 2 °C senza di essa |
| `binary_sensor.weather_station_silent` | nessun invio da 30 minuti |

Avvisi (sostituisci `notify.notify`): raffica forte (soglia regolabile), fulmini a meno di N km, gelo, stazione muta, batteria sotto il 20 %.

Imposta **Weather station - altitude** alla tua altitudine in metri, così la pressione al livello del mare corrisponde alle previsioni.

## Sostituire una Netatmo (o un'altra stazione)

1. Tienile entrambe in funzione per 2–3 settimane e confronta temperatura, pioggia giornaliera e vento su una dashboard.
2. Fai puntare le tue automazioni alle entità `weather_*` qui sopra invece che a quelle della vecchia stazione.
3. Rimuovi la vecchia integrazione quando nessuno la legge più.

Avanti: [Risoluzione problemi](05-troubleshooting.md).
