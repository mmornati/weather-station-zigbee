# 05 · Troubleshooting

Start with the serial monitor right after a press on **RESET**: during the first 2 minutes the station logs the modules it found, the join and each report. After that it is in deep sleep and USB disappears.

## The LED blinks for 3 minutes, then stops

The station did not find a network.

- Open *Permit join* in Zigbee2MQTT **before** pressing RESET.
- It was paired to another network: press RESET, hold BOOT 3 s, then pair again.
- Range: a sleepy end device needs a **router** (any mains Zigbee device) within reach. Plug the U.FL antenna in and keep it out of the metal.

## Joined, but no values in Zigbee2MQTT

- The converter is not loaded: check that Z2M shows the model `C6WeatherStation` as supported.
- The interview did not finish before it went to sleep: press RESET and run *Reconfigure* in Z2M during the 2 awake minutes.

## Wind always 0

- Wrong RJ11 pair: find the anemometer pair with a multimeter (it beeps once per turn).
- No pull-up on `D3`.

## Wind direction stuck or missing

- `wind_direction` never appears: the vane reads as open or short. Check the 10 kΩ to `D9` and the pair (688 Ω to 120 kΩ between them).
- Wrong by a fixed angle: the vane's **N** mark is not facing north.

## Rain counts too much or too little

- A few extra tips on windy days: the bucket is not level. Use the bubble on the rain plate.
- Compare with a manual gauge over a few weeks; the kit's 0.2794 mm per tip is a nominal value. Change `MM_PER_TIP` in `weather_logic.h` if needed.

## Temperature too high in the sun

- The SHT45 must sit inside the shield's cage, not against a plate, and the shield must be white.
- Keep the shield at least 1 m from the box, the panel and any wall.

## Battery drains

- Check the bq24074 charges (its CHG LED in sun) and that the panel faces south.
- Below 0 °C the bq24074 refuses to charge, on purpose. The cell carries the station through cold spells.
- Measure the sleep current: more than ~100 µA means something stays powered (a module's LED, a pull-up to a closed reed).

## Optional module not found

Modules are detected at power-on only: press RESET after connecting one. Check its address: SHT45 0x44, BME280 0x76/0x77, LTR390 0x53, AS3935 0x03 (both address pins high).
