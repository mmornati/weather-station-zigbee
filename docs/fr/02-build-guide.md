# 02 · Montage pas à pas

🌐 [English](../02-build-guide.md) · **Français** · [Italiano](../it/02-build-guide.md)

Suivez ces étapes dans l'ordre. Chaque partie est testée sur l'établi avant de partir dehors : un problème se voit au bureau, pas en haut d'un mât.

<p align="center"><img src="../../site/img/station.png" alt="La station terminée : un mât de 3 m avec le bras de vent en haut, l'abri anti-rayonnement, le panneau solaire et le boîtier, et le pluviomètre sur son propre piquet" width="380"> <img src="../../site/img/head.png" alt="Gros plan de l'abri sur son bras, du panneau sur son support inclinable et du boîtier avec ses presse-étoupes" width="300"></p>

## Ce qu'il vous faut

- Tout ce qui figure dans la [liste des pièces](01-hardware-and-wiring.md#pièces) (environ 170–200 € avec le mât et le filament).
- Une imprimante 3D avec un plateau de 220 × 220 mm qui imprime le PETG, un fer à souder avec une panne pour inserts à chaud, un multimètre.
- Un câble USB-C, une breadboard et quelques fils pour le test sur table.
- Facultatif mais utile : un wattmètre USB ou un Nordic PPK2 pour vérifier le courant en sommeil, un vernis de tropicalisation pour la carte.

Les fichiers utilisés :

| Quoi | Fichier |
|---|---|
| Firmware | [`firmware/weather_station/weather_station.ino`](../../firmware/weather_station/weather_station.ino), compilé avec [`firmware/platformio.ini`](../../firmware/platformio.ini) |
| Pièces 3D | [`hardware/3d/`](../../hardware/3d) : un fichier `.scad` par pièce, [`build_stl.sh`](../../hardware/3d/build_stl.sh) génère tous les STL |
| Convertisseur Zigbee2MQTT | [`zigbee2mqtt/weather_station.mjs`](../../zigbee2mqtt/weather_station.mjs) |
| Package Home Assistant | [`homeassistant/weather_station.yaml`](../../homeassistant/weather_station.yaml) |

## 1. Vérifier les câbles du kit

Les kits météo changent d'un lot à l'autre : mesurez avant de souder quoi que ce soit.

1. Câble du vent (celui de l'anémomètre, avec la girouette branchée dessus) : multimètre en mode continuité sur chaque paire de fils de la prise RJ11, et tournez lentement les coupelles. La **paire de l'anémomètre** bipe une fois par tour.
2. Toujours sur le câble du vent, passez en ohmmètre et tournez la girouette. La **paire de la girouette** lit entre environ 688 Ω et 120 kΩ selon la direction.
3. Câble de pluie : la **paire du pluviomètre** bipe à chaque fois que vous basculez l'auget à la main.
4. Notez les trois paires. Vous en aurez besoin pour les prises RJ11 de la carte.

## 2. Vérifier le chargeur

1. Branchez le bq24074 sur la 18650 (BAT) et le panneau solaire (IN). Scotchez la CTN 10 kΩ sur l'accu et reliez-la à TS.
2. Mettez le panneau au soleil. La LED CHG s'allume et la tension de l'accu monte doucement.
3. Seulement ensuite, continuez : le chargeur est la seule pièce difficile à dépanner une fois tout câblé.

## 3. Test sur table

1. Flashez le firmware avec le XIAO en USB, depuis le [site web](https://mmornati.github.io/weather-station-zigbee/?lang=fr#flash) ou avec PlatformIO ([comment](03-firmware.md#compiler-et-flasher)). Choisissez l'effacement au premier flash.
2. Sur une breadboard, câblez le kit, le SHT45 et le BME280 comme sur le [schéma de câblage](01-hardware-and-wiring.md#schéma-de-câblage). Laissez le chargeur de côté : l'USB alimente tout.
3. Copiez le [convertisseur](../../zigbee2mqtt/weather_station.mjs) dans `external_converters/` de Zigbee2MQTT et redémarrez Z2M. Ouvrez *Permit join*.
4. Ouvrez le moniteur série (`pio device monitor`) et appuyez sur **RESET**. Vous devez voir les modules détectés, la station rejoindre Zigbee2MQTT et un premier envoi.
5. Pendant les 2 minutes où la station reste éveillée après un reset, faites tourner les coupelles et basculer l'auget : l'envoi suivant montre le vent et la pluie.
6. Tournez la girouette dans chaque direction et vérifiez `wind_direction` dans Zigbee2MQTT.

S'il manque quelque chose, voyez le [Dépannage](05-troubleshooting.md) avant de souder.

## 4. Imprimer les pièces

Toutes les pièces sont des fichiers OpenSCAD dans [`hardware/3d`](../../hardware/3d). Les valeurs par défaut conviennent à un mât de 40 mm et un panneau de 136 × 110 mm. Pour les changer, modifiez `pole_d` dans [`common.scad`](../../hardware/3d/common.scad) ou `panel` dans [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad), puis lancez `hardware/3d/build_stl.sh`. Des STL prêts à imprimer sont sur le [site web](https://mmornati.github.io/weather-station-zigbee/?lang=fr#parts) et dans chaque version.

| Pièce | Fichier (`part=`) | Qté | Orientation | Réglages |
|---|---|---|---|---|
| <img src="../../site/img/part_shield_plate.png" width="90" alt=""> Assiette d'abri | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`plate`) | 9 | Telle qu'exportée (anneau plat en bas, jupe à 45° vers le haut) | Blanc, 2 parois, 15 % |
| <img src="../../site/img/part_shield_cap.png" width="90" alt=""> Chapeau d'abri | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`cap`) | 1 | Telle qu'exportée | Blanc, 2 parois, 15 % |
| <img src="../../site/img/part_shield_base.png" width="90" alt=""> Base + bras d'abri | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`base`) | 1 | À plat | 4 parois, 30 % |
| <img src="../../site/img/part_shield_spacer.png" width="50" alt=""> Entretoise | [`radiation_shield.scad`](../../hardware/3d/radiation_shield.scad) (`spacer`) | 30 | Debout | 100 % |
| <img src="../../site/img/part_box.png" width="90" alt=""> Boîtier électronique | [`electronics_box.scad`](../../hardware/3d/electronics_box.scad) (`box`) | 1 | Sur le dos | 4 parois, 30 % |
| <img src="../../site/img/part_box_lid.png" width="90" alt=""> Couvercle | [`electronics_box.scad`](../../hardware/3d/electronics_box.scad) (`lid`) | 1 | À plat | 4 parois, 30 % |
| <img src="../../site/img/part_panel_frame.png" width="90" alt=""> Cadre de panneau | [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad) (`frame`) | 1 | À plat | 4 parois, 30 % |
| <img src="../../site/img/part_panel_cheeks.png" width="90" alt=""> Joues de panneau | [`panel_bracket.scad`](../../hardware/3d/panel_bracket.scad) (`cheeks`) | 1 | Plaque de base en bas | 4 parois, 40 % |
| <img src="../../site/img/part_pole_saddle.png" width="90" alt=""> Collier de mât | [`pole_saddle.scad`](../../hardware/3d/pole_saddle.scad) | 4 | Face plate en bas | 6 parois, 40 % |
| <img src="../../site/img/part_angle_bracket.png" width="80" alt=""> Équerre | [`angle_bracket.scad`](../../hardware/3d/angle_bracket.scad) | 2 | Sur le côté | 6 parois, 50 % |
| <img src="../../site/img/part_rain_plate_top.png" width="90" alt=""> Platine de pluviomètre, dessus | [`rain_gauge_plate.scad`](../../hardware/3d/rain_gauge_plate.scad) (`top`) | 1 | À plat | 4 parois, 40 % |
| <img src="../../site/img/part_rain_plate_base.png" width="90" alt=""> Platine de pluviomètre, base | [`rain_gauge_plate.scad`](../../hardware/3d/rain_gauge_plate.scad) (`base`) | 1 | À plat | 4 parois, 40 % |

- **Matière** : PETG (235–245 °C, plateau 75–80 °C) de couleur claire, ou ASA si votre buse tient ~250 °C (la buse d'origine de l'Ender 3 V2 Neo a un tube PTFE). Pas de PLA : il ramollit au soleil d'été. Toutes les pièces tiennent sur un plateau de 220 × 220 mm, en couches de 0,2 mm, sans supports.
- **Imprimez un test d'abord** : un collier et le couvercle. Vérifiez que le collier repose bien à plat sur votre mât et que vos inserts entrent dans les trous.
- **Inserts à chaud** : M4 × 6 dans les colliers et les bossages du couvercle, M3 dans les plots de la carte du boîtier, M5 (ou des écrous) dans la base de la platine de pluviomètre.

## 5. Souder la carte

Tout ce qui n'est pas un module va sur une plaque à pastilles de 70 × 50 mm avec le XIAO. Suivez le [schéma de câblage](01-hardware-and-wiring.md#schéma-de-câblage) et le [tableau des broches](01-hardware-and-wiring.md#broches-utilisées-par-le-firmware).

1. Soudez le XIAO sur des connecteurs femelles, pour pouvoir le retirer et le flasher au bureau.
2. Soudez les deux prises RJ11 le long d'un bord, face aux presse-étoupes : RJ11 n° 1 pour le câble du vent, n° 2 pour celui de la pluie, reliées aux paires trouvées à l'étape 1.
3. Ajoutez les composants autour du XIAO :
   - 100 kΩ de `3V3` à `D1` et 100 nF de `D1` à `GND` (pluie) ;
   - 100 kΩ de `3V3` à `D3` et 10 nF de `D3` à `GND` (anémomètre) ;
   - 10 kΩ de `D9` à `D2` (girouette ; l'autre fil de la girouette va à `GND`) ;
   - 1 MΩ du `+` de la batterie à `D0`, 1 MΩ de `D0` à `GND`, et 100 nF en parallèle sur celle du bas (tension de batterie).
4. Ajoutez un connecteur 4 broches pour le bus I2C (`3V3`, `GND`, `D7` SDA, `D8` SCL). Le câble du SHT45 vers l'abri et le BME280 s'y branchent ; le LTR390 et l'AS3935 optionnels vont sur le même bus.
5. Sondes DS18B20 optionnelles : data sur `D10` (ras de l'herbe) et `D4` (sol), chacune avec une résistance de tirage de 4,7 kΩ vers `3V3`.
6. Soudez deux fils sur les pastilles **BAT+** et **BAT−** du XIAO, sous la carte, pour l'accu.
7. Vérifiez au multimètre que `3V3` et `GND` ne sont pas en court-circuit, puis branchez l'USB et refaites le test sur table avec la carte.
8. Facultatif : vaporisez du vernis de tropicalisation sur la carte, en évitant les prises RJ11 et les connecteurs.

## 6. Monter le boîtier

<img src="../../site/img/part_box.png" alt="Le boîtier imprimé avec quatre trous de presse-étoupes et un trou d'aérateur sur la face du bas" width="320" align="right">

1. Posez les inserts M3 dans les quatre plots de la carte et les inserts M4 dans les quatre bossages du couvercle.
2. Vissez quatre **presse-étoupes PG7** et l'**aérateur M12** sur la face du bas. Presse-étoupes, de gauche à droite : vent, pluie, capteurs de l'abri, panneau solaire.
3. Vissez la carte sur ses plots, prises RJ11 côté presse-étoupes.
4. Placez le support de 18650 dans le berceau en haut, le bq24074 et le BME280 à côté. Scotchez la CTN sur l'accu.
5. Reliez la sortie BAT du bq24074 au support d'accu et aux pastilles BAT du XIAO. Laissez l'accu de côté pour l'instant.
6. Passez les câbles dans les presse-étoupes : les câbles RJ11 du kit se branchent directement dans les prises, le câble 4 fils de l'abri va au connecteur I2C, le panneau sur l'entrée IN du chargeur.
7. Enfoncez le cordon silicone de 2 mm dans la gorge du bord, sur tout le tour, et coupez-le pour que les extrémités se touchent juste.
8. Placez l'antenne à l'intérieur, côté couvercle, loin de l'accu.

Laissez le couvercle ouvert jusqu'à ce que la station soit sur le mât : il faudra appuyer sur RESET une fois en place.

## 7. Assembler l'abri anti-rayonnement

<img src="../../site/img/shield.png" alt="Vue en coupe de l'abri : neuf assiettes sur trois tiges avec entretoises, le SHT45 dans une cage ajourée au milieu de la pile" width="360" align="right">

1. Passez trois **tiges filetées M5 inox** (environ 200 mm) dans la base et bloquez-les par un écrou et une rondelle en dessous.
2. Enfilez une **entretoise**, puis une **assiette** (jupe vers le bas), sur les trois tiges. Recommencez neuf fois.
3. Terminez par le **chapeau**. Si vous montez le LTR390, vissez-le d'abord sous la fenêtre du chapeau et collez le disque PTFE de 25 mm dans le logement au-dessus.
4. Serrez un écrou sur chaque tige au-dessus du chapeau. N'écrasez pas les assiettes : un serrage léger suffit.
5. Passez le SHT45, sur un court câble 4 fils, par le trou de la base jusque dans la cage ajourée, pour qu'il pende au milieu de la pile sans toucher les parois.

La jupe de chaque assiette couvre l'espace en dessous : ni le soleil direct ni la pluie n'atteignent le capteur, et l'air circule librement.

## 8. Installer la station

Choisissez un endroit dégagé du jardin, à au moins 3 m des murs, des haies et des dallages, et loin de l'air chaud de la maison.

| Hauteur | Quoi | Comment |
|---|---|---|
| 3 m | Bras de vent (du kit) | Avec le collier du kit, en haut du mât. Orientez le repère **N** de la girouette vers le nord (la boussole d'un téléphone suffit) |
| 1,5 m | Abri anti-rayonnement | Sur une équerre, sur un collier, côté **nord** du mât |
| 1,3 m | Panneau solaire | Sur un collier, plein **sud**, incliné d'environ **60°** |
| 1,2 m | Boîtier électronique | Sur un collier, presse-étoupes **vers le bas** |
| ~1 m | Pluviomètre | Sur son propre petit piquet à environ 1 m, sur une équerre et la platine de mise à niveau |

1. Plantez le mât bien vertical (un pieu de sol ou un bloc de béton). Vérifiez au niveau : un mât penché fait dériver la girouette.
2. Fixez les colliers avec deux colliers de serrage inox chacun, aux hauteurs ci-dessus.
3. **Abri** : boulonnez une équerre sur son collier avec 4 × M4 × 12 dans les inserts, puis le bras de l'abri sur la tablette de l'équerre avec 4 × M4 × 20 et des écrous nylstop.
4. **Panneau** : boulonnez les joues sur leur collier, placez le cadre entre elles avec un boulon M4 de pivot et un boulon M4 dans la lumière en arc, réglez environ 60° et serrez.
5. **Boîtier** : boulonnez-le sur son collier depuis l'intérieur avec 4 × M4 × 12.
6. **Pluviomètre** : sur son piquet, un collier, une équerre, puis la base de la platine sur la tablette. Vissez le pluviomètre sur la platine du dessus, mettez les trois vis M5 et leurs ressorts entre les deux platines et ajustez-les jusqu'à ce que la bulle soit centrée.
7. Sondes optionnelles : une DS18B20 à 5 cm au-dessus de l'herbe, hors de l'abri ; l'autre à 10 cm de profondeur dans le sol.
8. Branchez les câbles, mettez l'accu dans son support et appuyez sur **RESET** avec *Permit join* ouvert dans Zigbee2MQTT (la station garde l'appairage fait sur table, elle se reconnecte simplement).
9. Fermez le couvercle avec 4 × M4 × 12. Serrez les presse-étoupes autour des câbles.

## 9. Vérifications finales

| Faites ceci | Attendez-vous à |
|---|---|
| Surveillez Zigbee2MQTT pendant 10 minutes | Un envoi toutes les 5 minutes, une `battery` cohérente |
| Faites tourner les coupelles à la main | `wind_speed` et `wind_gust` au-dessus de 0 à l'envoi suivant |
| Versez lentement un verre d'eau dans le pluviomètre | `rain` augmente de 0,28 mm par basculement |
| Comparez `temperature` avec une référence à l'ombre | À environ 0,5 °C près |
| Regardez la carte de Z2M | La station est un **end device** avec un routeur parent |
| Revenez après une journée ensoleillée | La batterie reste à 100 % ou presque |

Ajoutez ensuite le [package Home Assistant](04-zigbee2mqtt-and-home-assistant.md#package-home-assistant) et réglez votre altitude.

Suite : [Firmware](03-firmware.md).
