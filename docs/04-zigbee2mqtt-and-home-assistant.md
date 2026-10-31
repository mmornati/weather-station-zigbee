# 04 · Zigbee2MQTT & Home Assistant

🌐 **English** · [Français](fr/04-zigbee2mqtt-and-home-assistant.md) · [Italiano](it/04-zigbee2mqtt-and-home-assistant.md)

## Zigbee2MQTT

1. Copy [`zigbee2mqtt/weather_station.mjs`](../zigbee2mqtt/weather_station.mjs) to `<z2m data>/external_converters/` and restart Zigbee2MQTT.
2. Open *Permit join*, press **RESET** on the station, wait for the interview to finish (it stays awake 2 minutes).
3. Rename it **`weather_station`**: the Home Assistant package expects that name.

Only the sensors the station really has are exposed: an optional module appears once its endpoint exists.

The station is a battery end device: Zigbee2MQTT's availability check only marks it offline after many hours. The package below has its own 30-minute "station silent" alert.

## Home Assistant package

Copy [`homeassistant/weather_station.yaml`](../homeassistant/weather_station.yaml) to `/config/packages/` and restart. It adds:

| Entity | What |
|---|---|
| `sensor.weather_wind_speed`, `sensor.weather_wind_gust` | km/h with the `wind_speed` device class |
| `sensor.weather_wind_direction` | degrees, `cardinal` attribute (N, NNE…) |
| `sensor.weather_beaufort` | Beaufort force |
| `sensor.weather_rain_total` | mm, `total_increasing` |
| `sensor.weather_rain_today`, `_this_month`, `_this_year` | utility meters |
| `sensor.weather_rain_last_15_min`, `sensor.weather_rain_last_24_h` | rolling totals |
| `sensor.weather_rain_rate` | mm/h over 15 minutes |
| `binary_sensor.weather_raining` | a tip in the last 15 minutes |
| `sensor.weather_dew_point`, `sensor.weather_feels_like` | Magnus dew point; wind chill / heat index |
| `sensor.weather_sea_level_pressure` | uses `input_number.weather_station_altitude` |
| `binary_sensor.weather_frost` | grass-level probe ≤ 0 °C, or air ≤ 2 °C without it |
| `binary_sensor.weather_station_silent` | no report for 30 minutes |

Alerts (replace `notify.notify`): strong gust (threshold adjustable), lightning closer than N km, frost, station silent, battery below 20 %.

Set **Weather station - altitude** to your altitude in metres so the sea-level pressure matches the forecasts.

## Replacing a Netatmo (or any other station)

1. Run both side by side for 2–3 weeks and compare temperature, daily rain and wind on a dashboard.
2. Point your automations at the `weather_*` entities above instead of the old station's.
3. Remove the old integration once nothing reads it any more.

Next: [Troubleshooting](05-troubleshooting.md).
