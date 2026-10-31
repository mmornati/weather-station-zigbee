# 05 · Dépannage

🌐 [English](../05-troubleshooting.md) · **Français** · [Italiano](../it/05-troubleshooting.md)

Commencez par le moniteur série juste après un appui sur **RESET** : pendant les 2 premières minutes, la station affiche les modules trouvés, la connexion et chaque envoi. Ensuite elle est en sommeil profond et l'USB disparaît.

## La LED clignote 3 minutes, puis s'arrête

La station n'a pas trouvé de réseau.

- Ouvrez *Permit join* dans Zigbee2MQTT **avant** d'appuyer sur RESET.
- Elle était appairée à un autre réseau : appuyez sur RESET, maintenez BOOT 3 s, puis appairez à nouveau.
- Portée : un sleepy end device a besoin d'un **routeur** (n'importe quel appareil Zigbee sur secteur) à portée. Branchez l'antenne U.FL et gardez-la loin du métal.

## Connectée, mais aucune valeur dans Zigbee2MQTT

- Le convertisseur n'est pas chargé : vérifiez que Z2M affiche le modèle `C6WeatherStation` comme pris en charge.
- L'interview n'a pas fini avant l'endormissement : appuyez sur RESET et lancez *Reconfigure* dans Z2M pendant les 2 minutes d'éveil.

## Vent toujours à 0

- Mauvaise paire RJ11 : trouvez la paire de l'anémomètre au multimètre (un bip par tour).
- Pas de résistance de tirage sur `D3`.

## Direction du vent figée ou absente

- `wind_direction` n'apparaît jamais : la girouette est lue ouverte ou en court-circuit. Vérifiez la 10 kΩ vers `D9` et la paire (688 Ω à 120 kΩ entre ses fils).
- Décalée d'un angle fixe : le repère **N** de la girouette n'est pas orienté au nord.

## La pluie compte trop ou pas assez

- Quelques basculements en trop par jour de vent : l'auget n'est pas de niveau. Utilisez la bulle de la platine.
- Comparez avec un pluviomètre manuel sur quelques semaines ; les 0,2794 mm par basculement du kit sont une valeur nominale. Modifiez `MM_PER_TIP` dans `weather_logic.h` si besoin.

## Température trop haute au soleil

- Le SHT45 doit être dans la cage de l'abri, pas contre une assiette, et l'abri doit être blanc.
- Gardez l'abri à au moins 1 m du boîtier, du panneau et de tout mur.

## La batterie se vide

- Vérifiez que le bq24074 charge (sa LED CHG au soleil) et que le panneau est orienté au sud.
- Sous 0 °C le bq24074 refuse de charger, exprès. L'accu fait tenir la station pendant les vagues de froid.
- Mesurez le courant en sommeil : plus de ~100 µA signifie que quelque chose reste alimenté (la LED d'un module, une résistance de tirage vers un reed fermé).

## Module optionnel non trouvé

Les modules ne sont détectés qu'à la mise sous tension : appuyez sur RESET après en avoir branché un. Vérifiez son adresse : SHT45 0x44, BME280 0x76/0x77, LTR390 0x53, AS3935 0x03 (les deux broches d'adresse à l'état haut).

## Les pièces imprimées ne s'ajustent pas

- Le collier ne repose pas à plat : votre mât n'a pas le diamètre `pole_d` de [`common.scad`](../../hardware/3d/common.scad). Changez-le et réimprimez.
- Les inserts entrent trop facilement ou pas du tout : ajustez `m4_insert_d` dans `common.scad` à vos inserts.
- Le panneau ne se clipse pas dans le cadre : mettez ses vraies dimensions dans `panel` de [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad).
