# 04 · Zigbee2MQTT & Home Assistant

🌐 [English](../04-zigbee2mqtt-and-home-assistant.md) · **Français** · [Italiano](../it/04-zigbee2mqtt-and-home-assistant.md)

## Zigbee2MQTT

1. Copiez [`zigbee2mqtt/weather_station.mjs`](../../zigbee2mqtt/weather_station.mjs) dans `<données z2m>/external_converters/` et redémarrez Zigbee2MQTT.
2. Ouvrez *Permit join*, appuyez sur **RESET** sur la station et attendez la fin de l'interview (elle reste éveillée 2 minutes).
3. Renommez-la **`weather_station`** : le package Home Assistant attend ce nom.

Seuls les capteurs réellement présents sont exposés : un module optionnel apparaît dès que son endpoint existe.

La station est un end device sur batterie : la vérification de disponibilité de Zigbee2MQTT ne la marque hors ligne qu'au bout de plusieurs heures. Le package ci-dessous a sa propre alerte « station muette » à 30 minutes.

## Package Home Assistant

Copiez [`homeassistant/weather_station.yaml`](../../homeassistant/weather_station.yaml) dans `/config/packages/` et redémarrez. Il ajoute :

| Entité | Quoi |
|---|---|
| `sensor.weather_wind_speed`, `sensor.weather_wind_gust` | km/h avec la device class `wind_speed` |
| `sensor.weather_wind_direction` | degrés, attribut `cardinal` (N, NNE…) |
| `sensor.weather_beaufort` | Force Beaufort |
| `sensor.weather_rain_total` | mm, `total_increasing` |
| `sensor.weather_rain_today`, `_this_month`, `_this_year` | utility meters |
| `sensor.weather_rain_last_15_min`, `sensor.weather_rain_last_24_h` | cumuls glissants |
| `sensor.weather_rain_rate` | mm/h sur 15 minutes |
| `binary_sensor.weather_raining` | un basculement dans les 15 dernières minutes |
| `sensor.weather_dew_point`, `sensor.weather_feels_like` | point de rosée (Magnus) ; refroidissement éolien / indice de chaleur |
| `sensor.weather_sea_level_pressure` | utilise `input_number.weather_station_altitude` |
| `binary_sensor.weather_frost` | sonde au ras de l'herbe ≤ 0 °C, ou air ≤ 2 °C sans elle |
| `binary_sensor.weather_station_silent` | aucun envoi depuis 30 minutes |

Alertes (remplacez `notify.notify`) : forte rafale (seuil réglable), foudre à moins de N km, gel, station muette, batterie sous 20 %.

Réglez **Weather station - altitude** sur votre altitude en mètres pour que la pression au niveau de la mer corresponde aux prévisions.

## Remplacer une Netatmo (ou une autre station)

1. Faites tourner les deux côte à côte pendant 2–3 semaines et comparez température, pluie journalière et vent sur un tableau de bord.
2. Faites pointer vos automatisations vers les entités `weather_*` ci-dessus au lieu de celles de l'ancienne station.
3. Supprimez l'ancienne intégration quand plus rien ne la lit.

Suite : [Dépannage](05-troubleshooting.md).
