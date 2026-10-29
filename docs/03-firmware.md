# 03 · Firmware

🌐 **English** · [Français](fr/03-firmware.md) · [Italiano](it/03-firmware.md)

## Build and flash

### From your browser (easiest)

Open the [website's flash section](https://mmornati.github.io/weather-station-zigbee/#flash) in Chrome or Edge on a computer, plug the XIAO in with USB-C (battery disconnected) and click **Connect & install**. Choose **Erase device** on the first install only. The page always carries the firmware built from the latest `main`.

### PlatformIO

```bash
cd firmware
pio run -t erase && pio run -t upload
pio device monitor
```

Erase only on the first flash (it also wipes the stored rain total). Later updates: `pio run -t upload`.

### Arduino IDE

1. Install the **esp32** core by Espressif, 3.3.0 or newer (Boards Manager).
2. Install these libraries (Library Manager), even for modules you don't fit, because the sketch includes them: **Adafruit BME280 Library**, **Adafruit SHT4x Library**, **Adafruit LTR390 Library**, **OneWire**, **DallasTemperature**, **SparkFun AS3935 Lightning Detector Arduino Library**.
3. Open `firmware/weather_station/weather_station.ino` and set the **Tools** menu:

| Setting | Value |
|---|---|
| Board | XIAO_ESP32C6 |
| Zigbee mode | Zigbee ED (end device) |
| Partition Scheme | Zigbee 4MB with spiffs |
| Erase All Flash Before Sketch Upload | Enabled (first flash only) |

## How it works

The station is a Zigbee **sleepy end device**: it spends ~94 % of its life in deep sleep and wakes for three reasons.

| Wake-up | What it does | How long |
|---|---|---|
| Rain bucket tips (reed on `D1`) | Counts the tip in RTC memory, sleeps again | ~0.3 s |
| Every 60 s | Reads the vane, counts anemometer pulses for 3 s in light sleep (rain tips too) | ~3.3 s |
| Every 5th minute, or the next minute when new rain is waiting | Reads every sensor, starts Zigbee, sends everything, waits for the acknowledgements | ~2 s |

Each report carries:

- **wind speed**: average of the five 3 s windows since the last report;
- **gust**: the best 3 s window;
- **direction**: vector average weighted by speed (350° and 10° give 0°, not 180°; a calm keeps the last direction);
- **rain**: total since installation in mm, saved to flash when it changes, so it survives battery swaps.

Wind is *sampled*: 3 s out of every 60. It gives a good 5-minute picture but can miss a short gust between two windows. Continuous counting on the C6's low-power core is planned as a v2.

### After a power-on or a press on RESET

1. Optional modules are detected (logged on the serial monitor).
2. The LED blinks slowly while the station looks for a network, for up to 3 minutes: open *Permit join* in Zigbee2MQTT.
3. Once joined, it reports and **stays awake for 2 minutes**, re-sending every 30 s, so Zigbee2MQTT can interview and configure it. This is also the time to read logs, because USB disappears in deep sleep.
4. Then the normal cycle starts.

**Leave the network**: press RESET, then hold **BOOT** for 3 s while the LED blinks (fast blink = release). The rain total is kept.

**Added a module later?** Press RESET so it is detected, then in Zigbee2MQTT run *Interview* and *Reconfigure* during the 2 awake minutes, so Z2M learns the new endpoint.

### Zigbee endpoints

| EP | Cluster | Z2M property | Unit | When |
|---|---|---|---|---|
| 1 | Analog Input | `wind_speed` | km/h | always |
| 1 | Power Configuration | `battery` | % | always |
| 2 | Analog Input | `wind_gust` | km/h | always |
| 3 | Analog Input | `wind_direction` | ° | always |
| 4 | Analog Input | `rain` | mm | always |
| 5 | Analog Input | `uv_index` | | LTR390 |
| 6 | Analog Input | `lightning_distance` | km | AS3935 |
| 7 | Analog Input | `lightning_count` | | AS3935 |
| 10 | Temperature + Humidity | `temperature`, `humidity` | °C, % | SHT45 or BME280 |
| 11 | Pressure | `pressure` | hPa (station level) | BME280 |
| 12 | Illuminance | `illuminance` | lx | LTR390 |
| 13 | Temperature | `ground_temperature` | °C | DS18B20 on `D10` |
| 14 | Temperature | `soil_temperature` | °C | DS18B20 on `D4` |
| 15 | Temperature | `box_temperature` | °C | SHT45 **and** BME280 |

## Settings

All in the `Behaviour` block at the top of the sketch.

| Define | Default | What |
|---|---|---|
| `TICK_MS` | 60000 | Time between two wind windows |
| `WIND_WINDOW_MS` | 3000 | Length of a wind window |
| `REPORT_EVERY_TICKS` | 5 | Ticks between two reports (5 = every 5 min) |
| `UV_WINDOW_FACTOR` | 1.0 | Raise it if the PTFE disc hides some UV (compare with a UV forecast on a clear day) |
| `HUMIDITY_HEATER_RH` | 95 | Above this, a 1 s heater pulse dries the SHT45 after the reading, at most once an hour |
| `JOIN_TIMEOUT_PAIRING_MS` | 180000 | How long it searches for a network after a reset |

Shorter reports cost battery: the Zigbee report is the biggest item in the [power budget](01-hardware-and-wiring.md#power).

Calibration (vane north offset, altitude for sea-level pressure) stays in Home Assistant: the station sleeps, so it cannot reliably receive settings.

## Tests

`pio test -e native` runs the host unit tests of `weather_logic.h`: vane decoding, wind average/gust/direction, rain bounce guard, the tick clock across deep sleep, battery curve, illuminance and UV conversions.

Next: [Zigbee2MQTT & Home Assistant](04-zigbee2mqtt-and-home-assistant.md).
