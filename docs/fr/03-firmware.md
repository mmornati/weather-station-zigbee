# 03 · Firmware

🌐 [English](../03-firmware.md) · **Français** · [Italiano](../it/03-firmware.md)

## Compiler et flasher

### Depuis le navigateur (le plus simple)

Ouvrez la [section flash du site](https://mmornati.github.io/weather-station-zigbee/?lang=fr#flash) dans Chrome ou Edge sur un ordinateur, branchez le XIAO en USB-C (accu débranché) et cliquez sur **Connecter & installer**. Choisissez **Erase device** à la première installation seulement. La page propose toujours le firmware compilé depuis le dernier `main`.

### PlatformIO

```bash
cd firmware
pio run -t erase && pio run -t upload
pio device monitor
```

N'effacez qu'au premier flash (cela efface aussi le cumul de pluie enregistré). Mises à jour suivantes : `pio run -t upload`.

### IDE Arduino

1. Installez le core **esp32** d'Espressif, 3.3.0 ou plus récent (gestionnaire de cartes).
2. Installez ces bibliothèques (gestionnaire de bibliothèques), même pour les modules que vous ne montez pas, car le sketch les inclut : **Adafruit BME280 Library**, **Adafruit SHT4x Library**, **Adafruit LTR390 Library**, **OneWire**, **DallasTemperature**, **SparkFun AS3935 Lightning Detector Arduino Library**.
3. Ouvrez `firmware/weather_station/weather_station.ino` et réglez le menu **Outils** :

| Réglage | Valeur |
|---|---|
| Carte | XIAO_ESP32C6 |
| Zigbee mode | Zigbee ED (end device) |
| Partition Scheme | Zigbee 4MB with spiffs |
| Erase All Flash Before Sketch Upload | Enabled (premier flash seulement) |

## Fonctionnement

La station est un **sleepy end device** Zigbee : elle passe ~94 % de sa vie en sommeil profond et se réveille pour trois raisons.

| Réveil | Ce qu'elle fait | Durée |
|---|---|---|
| L'auget bascule (reed sur `D1`) | Compte le basculement en mémoire RTC, se rendort | ~0,3 s |
| Toutes les 60 s | Lit la girouette, compte les impulsions de l'anémomètre pendant 3 s en sommeil léger (et la pluie) | ~3,3 s |
| Toutes les 5 minutes, ou à la minute suivante s'il y a de la nouvelle pluie | Lit tous les capteurs, démarre le Zigbee, envoie tout, attend les acquittements | ~2 s |

Chaque envoi contient :

- **vitesse du vent** : moyenne des cinq fenêtres de 3 s depuis le dernier envoi ;
- **rafale** : la meilleure fenêtre de 3 s ;
- **direction** : moyenne vectorielle pondérée par la vitesse (350° et 10° donnent 0°, pas 180° ; par vent nul, la dernière direction est gardée) ;
- **pluie** : cumul depuis l'installation en mm, enregistré en flash quand il change, donc il survit aux changements d'accu.

Le vent est *échantillonné* : 3 s toutes les 60. Cela donne une bonne image sur 5 minutes mais peut manquer une rafale brève entre deux fenêtres. Le comptage continu sur le cœur basse consommation du C6 est prévu pour une v2.

### Après une mise sous tension ou un appui sur RESET

1. Les modules optionnels sont détectés (affichés sur le moniteur série).
2. La LED clignote lentement pendant que la station cherche un réseau, jusqu'à 3 minutes : ouvrez *Permit join* dans Zigbee2MQTT.
3. Une fois connectée, elle envoie ses mesures et **reste éveillée 2 minutes**, en renvoyant toutes les 30 s, pour que Zigbee2MQTT puisse l'interviewer et la configurer. C'est aussi le moment de lire les logs, car l'USB disparaît en sommeil profond.
4. Puis le cycle normal commence.

**Quitter le réseau** : appuyez sur RESET, puis maintenez **BOOT** 3 s pendant que la LED clignote (clignotement rapide = relâchez). Le cumul de pluie est conservé.

**Module ajouté après coup ?** Appuyez sur RESET pour qu'il soit détecté, puis dans Zigbee2MQTT lancez *Interview* et *Reconfigure* pendant les 2 minutes d'éveil, pour que Z2M découvre le nouvel endpoint.

### Endpoints Zigbee

| EP | Cluster | Propriété Z2M | Unité | Quand |
|---|---|---|---|---|
| 1 | Analog Input | `wind_speed` | km/h | toujours |
| 1 | Power Configuration | `battery` | % | toujours |
| 2 | Analog Input | `wind_gust` | km/h | toujours |
| 3 | Analog Input | `wind_direction` | ° | toujours |
| 4 | Analog Input | `rain` | mm | toujours |
| 5 | Analog Input | `uv_index` | | LTR390 |
| 6 | Analog Input | `lightning_distance` | km | AS3935 |
| 7 | Analog Input | `lightning_count` | | AS3935 |
| 10 | Temperature + Humidity | `temperature`, `humidity` | °C, % | SHT45 ou BME280 |
| 11 | Pressure | `pressure` | hPa (au niveau de la station) | BME280 |
| 12 | Illuminance | `illuminance` | lx | LTR390 |
| 13 | Temperature | `ground_temperature` | °C | DS18B20 sur `D10` |
| 14 | Temperature | `soil_temperature` | °C | DS18B20 sur `D4` |
| 15 | Temperature | `box_temperature` | °C | SHT45 **et** BME280 |

## Réglages

Tous dans le bloc `Behaviour` en haut du sketch.

| Define | Défaut | Rôle |
|---|---|---|
| `TICK_MS` | 60000 | Temps entre deux fenêtres de vent |
| `WIND_WINDOW_MS` | 3000 | Durée d'une fenêtre de vent |
| `REPORT_EVERY_TICKS` | 5 | Ticks entre deux envois (5 = toutes les 5 min) |
| `UV_WINDOW_FACTOR` | 1.0 | À augmenter si le disque PTFE masque une partie des UV (comparez avec une prévision UV par temps clair) |
| `HUMIDITY_HEATER_RH` | 95 | Au-dessus, une impulsion de chauffage de 1 s sèche le SHT45 après la mesure, au plus une fois par heure |
| `JOIN_TIMEOUT_PAIRING_MS` | 180000 | Durée de recherche d'un réseau après un reset |

Des envois plus fréquents coûtent de la batterie : l'envoi Zigbee est le plus gros poste du [bilan énergétique](01-hardware-and-wiring.md#alimentation).

L'étalonnage (décalage du nord de la girouette, altitude pour la pression au niveau de la mer) reste dans Home Assistant : la station dort, elle ne peut pas recevoir de réglages de façon fiable.

## Tests

`pio test -e native` lance les tests unitaires de `weather_logic.h` sur l'hôte : décodage de la girouette, moyenne/rafale/direction du vent, anti-rebond du pluviomètre, horloge à travers le sommeil profond, courbe de batterie, conversions d'éclairement et d'UV.

Suite : [Zigbee2MQTT & Home Assistant](04-zigbee2mqtt-and-home-assistant.md).
