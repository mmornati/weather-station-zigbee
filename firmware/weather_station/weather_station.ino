/**
 * Weather station — Seeed XIAO ESP32-C6, 18650 + solar, Zigbee sleepy end device.
 * Wind, rain, temperature, humidity and pressure, plus optional UV/light, lightning,
 * ground and soil probes, reported to Zigbee2MQTT:
 *
 *   EP 1  Analog Input            wind speed, km/h (5 min average) + battery (Power Config)
 *   EP 2  Analog Input            wind gust, km/h (best 3 s window)
 *   EP 3  Analog Input            wind direction, degrees (0 = north)
 *   EP 4  Analog Input            rain since installation, mm (kept in flash)
 *   EP 5  Analog Input            UV index                       (LTR390, optional)
 *   EP 6  Analog Input            distance of the last strike, km (AS3935, optional)
 *   EP 7  Analog Input            lightning strikes since power-on (AS3935, optional)
 *   EP 10 Temperature + Humidity  SHT45 in the shield (or the BME280 if there is no SHT45)
 *   EP 11 Pressure                BME280, hPa (station level)
 *   EP 12 Illuminance             LTR390, lux                    (optional)
 *   EP 13 Temperature             DS18B20 at grass level         (optional)
 *   EP 14 Temperature             DS18B20 in the soil            (optional)
 *   EP 15 Temperature             BME280 inside the box          (only with an SHT45 too)
 *
 * The station spends its life in deep sleep (~15 µA):
 *   - a rain bucket tip on D1 wakes it: count it, sleep again;
 *   - every 60 s: count anemometer pulses for 3 s in light sleep, read the vane;
 *   - every 5 min (every minute while it rains): read the sensors, start Zigbee, report.
 * After a power-on or a press on RESET it stays awake for 2 minutes once joined, so
 * Zigbee2MQTT can interview and configure it. Hold BOOT 3 s during that window to
 * leave the network.
 *
 * Optional modules are detected at power-on: plug them in, then press RESET.
 *
 * PlatformIO: all settings are in ../platformio.ini.
 * Arduino IDE, Tools menu:
 *   Board:            XIAO_ESP32C6
 *   Zigbee mode:      Zigbee ED (end device)
 *   Partition Scheme: Zigbee 4MB with spiffs
 *   Erase All Flash Before Sketch Upload: Enabled (first flash only)
 * Libraries (Library Manager): Adafruit BME280 Library, Adafruit SHT4x Library,
 *   Adafruit LTR390 Library, OneWire, DallasTemperature,
 *   SparkFun AS3935 Lightning Detector Arduino Library.
 *
 * Requires arduino-esp32 core 3.3.0 or newer.
 */

#include <Arduino.h>
#ifndef ZIGBEE_MODE_ED
#error "Select Tools -> Zigbee mode -> Zigbee ED (end device)"
#endif

#include "Zigbee.h"
#include <Preferences.h>
#include <Wire.h>
#include <sys/time.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include <Adafruit_BME280.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_LTR390.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SparkFun_AS3935.h>
#include "weather_logic.h"

/* ---------- Pins (XIAO ESP32-C6) ---------- */
#define BATTERY_ADC_PIN     0    // D0, 18650 through a 1 MΩ / 1 MΩ divider
#define RAIN_PIN            1    // D1, rain gauge reed to GND (deep-sleep wake source)
#define VANE_ADC_PIN        2    // D2, wind vane to GND, 10 kΩ to VANE_POWER_PIN
#define ANEMOMETER_PIN      21   // D3, anemometer reed to GND
#define SOIL_ONEWIRE_PIN    22   // D4, DS18B20 in the soil (optional)
#define I2C_SDA_PIN         17   // D7, SHT45 / BME280 / LTR390 / AS3935
#define I2C_SCL_PIN         19   // D8
#define VANE_POWER_PIN      20   // D9, powers the vane divider only while reading it
#define GROUND_ONEWIRE_PIN  18   // D10, DS18B20 at grass level (optional)
#define LED_PIN             15   // user LED, active LOW
#define BUTTON_PIN          BOOT_PIN  // GPIO9, BOOT button
#define RF_SWITCH_EN        3    // LOW = enable RF switch control
#define RF_ANT_SELECT       14   // HIGH = external U.FL antenna, LOW = ceramic

/* ---------- Endpoints ---------- */
#define EP_WIND_SPEED          1
#define EP_WIND_GUST           2
#define EP_WIND_DIRECTION      3
#define EP_RAIN                4
#define EP_UV                  5
#define EP_LIGHTNING_DISTANCE  6
#define EP_LIGHTNING_COUNT     7
#define EP_CLIMATE             10
#define EP_PRESSURE            11
#define EP_ILLUMINANCE         12
#define EP_GROUND_TEMPERATURE  13
#define EP_SOIL_TEMPERATURE    14
#define EP_BOX_TEMPERATURE     15

/* ---------- Behaviour ---------- */
#define TICK_MS                60000UL   // one wind window per minute
#define WIND_WINDOW_MS         3000UL    // anemometer counted for 3 s
#define REPORT_EVERY_TICKS     5         // report every 5 minutes (every tick while it rains)
#define ANEMOMETER_MIN_GAP_US  5000      // closures closer than 5 ms are reed bounce (> 400 km/h)
#define RAIN_RELEASE_WAIT_MS   500       // wait for the bucket's reed to open again before sleeping
#define BATTERY_DIVIDER        2.0f      // two equal resistors: battery+ -> D0 -> GND
#define BATTERY_MIN_VALID_V    2.5f      // below this the divider is not wired
#define VANE_SUPPLY_MV         3300
#define VANE_PULLUP_OHMS       10000.0f
#define UV_WINDOW_FACTOR       1.0f      // raise it if the PTFE disc in front of the LTR390 hides some UV
#define LIGHT_WINDOW_FACTOR    1.0f
#define HUMIDITY_HEATER_RH     95.0f     // dry the SHT45 with its heater after fog...
#define HEATER_INTERVAL_MS     (60UL * 60UL * 1000UL)  // ...at most once an hour
#define JOIN_TIMEOUT_PAIRING_MS (3UL * 60UL * 1000UL)  // after power-on: time to open "Permit join"
#define JOIN_TIMEOUT_MS        10000UL   // a normal wake: the network is known
#define AWAKE_AFTER_BOOT_MS    (2UL * 60UL * 1000UL)   // stay reachable for the Z2M interview
#define AWAKE_REPORT_MS        30000UL   // re-send everything while awake
#define REPORT_ACK_TIMEOUT_MS  3000UL
#define FACTORY_RESET_HOLD_MS  3000
#define RTC_MAGIC              0x57583031UL  // "WX01"

/* ---------- Optional modules, detected at power-on ---------- */
enum Module : uint8_t {
  MOD_SHT45 = 1 << 0,
  MOD_BME280 = 1 << 1,
  MOD_LTR390 = 1 << 2,
  MOD_AS3935 = 1 << 3,
  MOD_GROUND = 1 << 4,
  MOD_SOIL = 1 << 5,
};

/* ---------- State kept in RTC memory across deep sleep ---------- */
struct RtcState {
  uint32_t magic;
  uint8_t modules;
  uint8_t bmeAddress;
  bool heaterUsed;
  uint64_t lastHeaterMs;
  uint32_t savedTips;
  wx::WindAccumulator wind;
  wx::RainCounter rain;
  wx::LightningLog lightning;
  wx::TickClock clock;
  wx::ReportSchedule schedule;
};
RTC_DATA_ATTR RtcState rtc;

struct Readings {
  float batteryVolts = 0;
  bool climateOk = false;
  float temperature = 0;
  float humidity = 0;
  bool pressureOk = false;
  float pressureHpa = 0;
  bool boxOk = false;
  float boxTemperature = 0;
  bool lightOk = false;
  float uvIndex = 0;
  float lux = 0;
  bool groundOk = false;
  float groundTemperature = 0;
  bool soilOk = false;
  float soilTemperature = 0;
};

ZigbeeAnalog zbWindSpeed = ZigbeeAnalog(EP_WIND_SPEED);
ZigbeeAnalog zbWindGust = ZigbeeAnalog(EP_WIND_GUST);
ZigbeeAnalog zbWindDirection = ZigbeeAnalog(EP_WIND_DIRECTION);
ZigbeeAnalog zbRain = ZigbeeAnalog(EP_RAIN);
ZigbeeAnalog zbUv = ZigbeeAnalog(EP_UV);
ZigbeeAnalog zbLightningDistance = ZigbeeAnalog(EP_LIGHTNING_DISTANCE);
ZigbeeAnalog zbLightningCount = ZigbeeAnalog(EP_LIGHTNING_COUNT);
ZigbeeTempSensor zbClimate = ZigbeeTempSensor(EP_CLIMATE);
ZigbeePressureSensor zbPressure = ZigbeePressureSensor(EP_PRESSURE);
ZigbeeIlluminanceSensor zbIlluminance = ZigbeeIlluminanceSensor(EP_ILLUMINANCE);
ZigbeeTempSensor zbGround = ZigbeeTempSensor(EP_GROUND_TEMPERATURE);
ZigbeeTempSensor zbSoil = ZigbeeTempSensor(EP_SOIL_TEMPERATURE);
ZigbeeTempSensor zbBox = ZigbeeTempSensor(EP_BOX_TEMPERATURE);

Preferences preferences;
Adafruit_BME280 bme;
Adafruit_SHT4x sht;
Adafruit_LTR390 ltr;
SparkFun_AS3935 lightningSensor(0x03);  // AS3935 breakout default address (ADD0 and ADD1 high)
OneWire groundBus(GROUND_ONEWIRE_PIN);
OneWire soilBus(SOIL_ONEWIRE_PIN);
DallasTemperature groundProbe(&groundBus);
DallasTemperature soilProbe(&soilBus);

volatile int pendingReports = 0;
bool rainLevelHigh = true;

static inline bool has(uint8_t module) {
  return (rtc.modules & module) != 0;
}

static inline void ledOn()  { digitalWrite(LED_PIN, LOW); }
static inline void ledOff() { digitalWrite(LED_PIN, HIGH); }

// Milliseconds since power-on. The RTC keeps system time running through deep sleep.
uint64_t nowMs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)tv.tv_sec * 1000ULL + tv.tv_usec / 1000;
}

void useExternalAntenna() {
  pinMode(RF_SWITCH_EN, OUTPUT);
  digitalWrite(RF_SWITCH_EN, LOW);
  delay(100);
  pinMode(RF_ANT_SELECT, OUTPUT);
  digitalWrite(RF_ANT_SELECT, HIGH);
}

/* ---------- Rain ---------- */

uint32_t loadStoredTips() {
  preferences.begin("weather", true);
  uint32_t tips = preferences.getUInt("tips", 0);
  preferences.end();
  return tips;
}

void saveTipsIfChanged() {
  if (rtc.rain.tips == rtc.savedTips) {
    return;
  }
  preferences.begin("weather", false);
  preferences.putUInt("tips", rtc.rain.tips);
  preferences.end();
  rtc.savedTips = rtc.rain.tips;
}

// Called from every loop that keeps the station awake: a tip closes the reed for ~100 ms.
void pollRain() {
  bool high = digitalRead(RAIN_PIN) == HIGH;
  if (rainLevelHigh && !high) {
    rtc.rain.tip(nowMs());
  }
  rainLevelHigh = high;
}

void waitForRainRelease() {
  unsigned long start = millis();
  while (digitalRead(RAIN_PIN) == LOW && millis() - start < RAIN_RELEASE_WAIT_MS) {
    delay(10);
  }
  rainLevelHigh = digitalRead(RAIN_PIN) == HIGH;
}

/* ---------- Wind ---------- */

float readVaneDegrees() {
  digitalWrite(VANE_POWER_PIN, HIGH);
  delay(2);
  uint32_t millivolts = 0;
  for (int i = 0; i < 8; i++) {
    millivolts += analogReadMilliVolts(VANE_ADC_PIN);
  }
  digitalWrite(VANE_POWER_PIN, LOW);
  return wx::vaneDegrees(millivolts / 8, VANE_SUPPLY_MV, VANE_PULLUP_OHMS);
}

/**
 * Counts anemometer closures for WIND_WINDOW_MS in light sleep: the CPU wakes on each change
 * of the reed level, counts closures, and sleeps again. Rain tips are counted the same way.
 */
uint32_t countWindWindow() {
  uint32_t pulses = 0;
  int64_t start = esp_timer_get_time();
  int64_t end = start + (int64_t)WIND_WINDOW_MS * 1000;
  int64_t lastPulseUs = start - ANEMOMETER_MIN_GAP_US;
  bool windHigh = gpio_get_level((gpio_num_t)ANEMOMETER_PIN) == 1;
  rainLevelHigh = gpio_get_level((gpio_num_t)RAIN_PIN) == 1;

  esp_sleep_enable_gpio_wakeup();
  for (int64_t now = start; now < end; now = esp_timer_get_time()) {
    gpio_wakeup_enable((gpio_num_t)ANEMOMETER_PIN, windHigh ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable((gpio_num_t)RAIN_PIN, rainLevelHigh ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_timer_wakeup(end - now);
    esp_light_sleep_start();

    int64_t woke = esp_timer_get_time();
    bool high = gpio_get_level((gpio_num_t)ANEMOMETER_PIN) == 1;
    if (windHigh && !high && woke - lastPulseUs >= ANEMOMETER_MIN_GAP_US) {
      pulses++;
      lastPulseUs = woke;
    }
    windHigh = high;
    pollRain();
  }
  gpio_wakeup_disable((gpio_num_t)ANEMOMETER_PIN);
  gpio_wakeup_disable((gpio_num_t)RAIN_PIN);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  return pulses;
}

void pollLightning() {
  if (!has(MOD_AS3935) || !lightningSensor.begin(Wire)) {
    return;
  }
  uint8_t cause = lightningSensor.readInterruptReg();
  if (cause == wx::AS3935_INT_LIGHTNING) {
    rtc.lightning.add(cause, lightningSensor.distanceToStorm());
    Serial.printf("Lightning at %u km\n", rtc.lightning.lastDistanceKm);
  }
}

void runTick() {
  float direction = readVaneDegrees();
  uint32_t pulses = countWindWindow();
  rtc.wind.addWindow(pulses, WIND_WINDOW_MS, direction);
  pollLightning();
  Serial.printf("Wind window: %lu pulses, vane %.1f deg\n", (unsigned long)pulses, direction);
}

/* ---------- Sensors ---------- */

void detectModules() {
  rtc.modules = 0;
  if (sht.begin(&Wire)) {
    rtc.modules |= MOD_SHT45;
  }
  if (bme.begin(0x76, &Wire)) {
    rtc.modules |= MOD_BME280;
    rtc.bmeAddress = 0x76;
  } else if (bme.begin(0x77, &Wire)) {
    rtc.modules |= MOD_BME280;
    rtc.bmeAddress = 0x77;
  }
  if (ltr.begin(&Wire)) {
    rtc.modules |= MOD_LTR390;
    ltr.enable(false);
  }
  if (lightningSensor.begin(Wire)) {
    rtc.modules |= MOD_AS3935;
    lightningSensor.resetSettings();
    lightningSensor.setIndoorOutdoor(OUTDOOR);
    lightningSensor.maskDisturber(true);
  }
  groundProbe.begin();
  if (groundProbe.getDeviceCount() > 0) {
    rtc.modules |= MOD_GROUND;
  }
  soilProbe.begin();
  if (soilProbe.getDeviceCount() > 0) {
    rtc.modules |= MOD_SOIL;
  }
  Serial.printf("Modules: SHT45 %d, BME280 %d, LTR390 %d, AS3935 %d, ground probe %d, soil probe %d\n",
                has(MOD_SHT45), has(MOD_BME280), has(MOD_LTR390), has(MOD_AS3935), has(MOD_GROUND), has(MOD_SOIL));
}

float readBatteryVolts() {
  uint32_t millivolts = 0;
  for (int i = 0; i < 16; i++) {
    millivolts += analogReadMilliVolts(BATTERY_ADC_PIN);
  }
  return millivolts / 16.0f / 1000.0f * BATTERY_DIVIDER;
}

bool readProbe(DallasTemperature &probe, float &celsius) {
  celsius = probe.getTempCByIndex(0);
  return celsius != DEVICE_DISCONNECTED_C && celsius > -55.0f && celsius < 125.0f;
}

bool readLtr390(float &uvi, float &lux) {
  ltr.setResolution(LTR390_RESOLUTION_18BIT);  // 100 ms per measurement
  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_18);
  ltr.enable(true);
  unsigned long start = millis();
  while (!ltr.newDataAvailable() && millis() - start < 400) {
    delay(10);
  }
  uint32_t uvs = ltr.readUVS();

  // Gain 1 for light: full sun (~100 klx) would saturate a higher gain.
  ltr.setMode(LTR390_MODE_ALS);
  ltr.setGain(LTR390_GAIN_1);
  start = millis();
  while (!ltr.newDataAvailable() && millis() - start < 400) {
    delay(10);
  }
  uint32_t als = ltr.readALS();
  ltr.enable(false);

  uvi = wx::uvIndex(uvs, UV_WINDOW_FACTOR);
  lux = wx::luxFromAls(als, 1.0f, LIGHT_WINDOW_FACTOR);
  return millis() - start < 400;
}

Readings readSensors() {
  Readings r;
  r.batteryVolts = readBatteryVolts();

  // Start the slow DS18B20 conversions first, read them last.
  if (has(MOD_GROUND)) {
    groundProbe.begin();
    groundProbe.setResolution(11);  // 0.125 °C, 375 ms
    groundProbe.setWaitForConversion(false);
    groundProbe.requestTemperatures();
  }
  if (has(MOD_SOIL)) {
    soilProbe.begin();
    soilProbe.setResolution(11);
    soilProbe.setWaitForConversion(false);
    soilProbe.requestTemperatures();
  }
  unsigned long probesStarted = millis();

  if (has(MOD_SHT45) && sht.begin(&Wire)) {
    sht.setPrecision(SHT4X_HIGH_PRECISION);
    sht.setHeater(SHT4X_NO_HEATER);
    sensors_event_t humidity, temperature;
    if (sht.getEvent(&humidity, &temperature)) {
      r.climateOk = true;
      r.temperature = temperature.temperature;
      r.humidity = humidity.relative_humidity;
    }
  }

  if (has(MOD_BME280) && bme.begin(rtc.bmeAddress, &Wire)) {
    bme.setSampling(Adafruit_BME280::MODE_FORCED, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::FILTER_OFF);
    if (bme.takeForcedMeasurement()) {
      float temperature = bme.readTemperature();
      float humidity = bme.readHumidity();
      float pressure = bme.readPressure() / 100.0f;
      if (!isnan(pressure)) {
        r.pressureOk = true;
        r.pressureHpa = pressure;
      }
      if (!isnan(temperature)) {
        if (has(MOD_SHT45)) {
          r.boxOk = true;
          r.boxTemperature = temperature;
        } else if (!isnan(humidity)) {
          // No SHT45: the BME280 is the outdoor sensor (put it in the shield then).
          r.climateOk = true;
          r.temperature = temperature;
          r.humidity = humidity;
        }
      }
    }
  }

  if (has(MOD_LTR390) && ltr.begin(&Wire)) {
    r.lightOk = readLtr390(r.uvIndex, r.lux);
  }

  if (has(MOD_GROUND) || has(MOD_SOIL)) {
    while (millis() - probesStarted < 400) {
      pollRain();
      delay(10);
    }
    if (has(MOD_GROUND)) {
      r.groundOk = readProbe(groundProbe, r.groundTemperature);
    }
    if (has(MOD_SOIL)) {
      r.soilOk = readProbe(soilProbe, r.soilTemperature);
    }
  }

  return r;
}

// Fog and dew leave water on the SHT45: a short heater pulse dries it (after the reading).
void dryHumiditySensorIfNeeded(const Readings &r) {
  if (!has(MOD_SHT45) || !r.climateOk || r.humidity < HUMIDITY_HEATER_RH) {
    return;
  }
  uint64_t now = nowMs();
  if (rtc.heaterUsed && now - rtc.lastHeaterMs < HEATER_INTERVAL_MS) {
    return;
  }
  sht.setHeater(SHT4X_MED_HEATER_1S);
  sensors_event_t humidity, temperature;
  sht.getEvent(&humidity, &temperature);
  sht.setHeater(SHT4X_NO_HEATER);
  rtc.heaterUsed = true;
  rtc.lastHeaterMs = now;
  Serial.println("SHT45 heater pulse");
}

/* ---------- Zigbee ---------- */

void onGlobalResponse(zb_cmd_type_t command, esp_zb_zcl_status_t status, uint8_t endpoint, uint16_t cluster) {
  (void)status;
  (void)endpoint;
  (void)cluster;
  if (command == ZB_CMD_REPORT_ATTRIBUTE && pendingReports > 0) {
    pendingReports = pendingReports - 1;
  }
}

void setupAnalogInput(ZigbeeAnalog &ep, const char *description, float resolution) {
  ep.addAnalogInput();
  ep.setAnalogInputDescription(description);
  ep.setAnalogInputResolution(resolution);
  Zigbee.addEndpoint(&ep);
}

void setupTemperature(ZigbeeTempSensor &ep, float min, float max) {
  ep.setMinMaxValue(min, max);
  ep.setTolerance(0.1f);
  Zigbee.addEndpoint(&ep);
}

// The endpoint list must be the same on every wake: it only depends on rtc.modules.
void setupEndpoints(float batteryVolts) {
  zbWindSpeed.setManufacturerAndModel("DIY", "C6WeatherStation");
  zbWindSpeed.setPowerSource(ZB_POWER_SOURCE_BATTERY, wx::batteryPercent(batteryVolts), wx::zclBatteryVoltage(batteryVolts));
  setupAnalogInput(zbWindSpeed, "Wind km/h", 0.1f);
  setupAnalogInput(zbWindGust, "Gust km/h", 0.1f);
  setupAnalogInput(zbWindDirection, "Wind direction deg", 1.0f);
  setupAnalogInput(zbRain, "Rain mm", 0.01f);
  if (has(MOD_LTR390)) {
    setupAnalogInput(zbUv, "UV index", 0.1f);
  }
  if (has(MOD_AS3935)) {
    setupAnalogInput(zbLightningDistance, "Lightning km", 1.0f);
    setupAnalogInput(zbLightningCount, "Lightning strikes", 1.0f);
  }
  if (has(MOD_SHT45) || has(MOD_BME280)) {
    zbClimate.setMinMaxValue(-40, 85);
    zbClimate.setTolerance(0.1f);
    zbClimate.addHumiditySensor(0, 100, 1);
    Zigbee.addEndpoint(&zbClimate);
  }
  if (has(MOD_BME280)) {
    zbPressure.setMinMaxValue(300, 1100);
    Zigbee.addEndpoint(&zbPressure);
  }
  if (has(MOD_LTR390)) {
    zbIlluminance.setMinMaxValue(0, 50001);  // 0 to 100 000 lux on the log scale
    Zigbee.addEndpoint(&zbIlluminance);
  }
  if (has(MOD_GROUND)) {
    setupTemperature(zbGround, -40, 85);
  }
  if (has(MOD_SOIL)) {
    setupTemperature(zbSoil, -40, 85);
  }
  if (has(MOD_SHT45) && has(MOD_BME280)) {
    setupTemperature(zbBox, -40, 85);
  }
}

static inline void count(bool sent, int reports = 1) {
  if (sent) {
    pendingReports = pendingReports + reports;
  }
}

void sendReadings(const Readings &r, const wx::WindReport &wind) {
  pendingReports = 0;

  zbWindSpeed.setAnalogInput(wind.averageKmh);
  count(zbWindSpeed.reportAnalogInput());
  zbWindGust.setAnalogInput(wind.gustKmh);
  count(zbWindGust.reportAnalogInput());
  if (wind.directionDeg >= 0) {
    zbWindDirection.setAnalogInput(wind.directionDeg);
    count(zbWindDirection.reportAnalogInput());
  }
  zbRain.setAnalogInput(rtc.rain.totalMm());
  count(zbRain.reportAnalogInput());

  if (r.batteryVolts >= BATTERY_MIN_VALID_V) {
    zbWindSpeed.setBatteryVoltage(wx::zclBatteryVoltage(r.batteryVolts));
    zbWindSpeed.setBatteryPercentage(wx::batteryPercent(r.batteryVolts));
    count(zbWindSpeed.reportBatteryPercentage());
  }
  if (r.climateOk) {
    zbClimate.setTemperature(r.temperature);
    zbClimate.setHumidity(r.humidity);
    count(zbClimate.report(), 2);
  }
  if (r.pressureOk) {
    zbPressure.setPressure((int16_t)lroundf(r.pressureHpa));
    count(zbPressure.report());
  }
  if (r.boxOk) {
    zbBox.setTemperature(r.boxTemperature);
    count(zbBox.reportTemperature());
  }
  if (r.lightOk) {
    zbUv.setAnalogInput(r.uvIndex);
    count(zbUv.reportAnalogInput());
    zbIlluminance.setIlluminance(wx::zclIlluminance(r.lux));
    count(zbIlluminance.report());
  }
  if (r.groundOk) {
    zbGround.setTemperature(r.groundTemperature);
    count(zbGround.reportTemperature());
  }
  if (r.soilOk) {
    zbSoil.setTemperature(r.soilTemperature);
    count(zbSoil.reportTemperature());
  }
  if (has(MOD_AS3935)) {
    zbLightningCount.setAnalogInput(rtc.lightning.strikes);
    count(zbLightningCount.reportAnalogInput());
    if (rtc.lightning.lastDistanceKm > 0) {
      zbLightningDistance.setAnalogInput(rtc.lightning.lastDistanceKm);
      count(zbLightningDistance.reportAnalogInput());
    }
  }

  // Wait for the coordinator to acknowledge, so the radio is not cut mid-send.
  unsigned long start = millis();
  while (pendingReports > 0 && millis() - start < REPORT_ACK_TIMEOUT_MS) {
    pollRain();
    delay(20);
  }

  Serial.printf("Reported: wind %.1f km/h, gust %.1f, dir %.0f, rain %.2f mm, battery %.2f V", wind.averageKmh,
                wind.gustKmh, wind.directionDeg, rtc.rain.totalMm(), r.batteryVolts);
  if (r.climateOk) {
    Serial.printf(", %.2f C, %.1f %%", r.temperature, r.humidity);
  }
  if (r.pressureOk) {
    Serial.printf(", %.1f hPa", r.pressureHpa);
  }
  Serial.printf(" (%d unacknowledged)\n", (int)pendingReports);
}

void checkFactoryReset() {
  if (digitalRead(BUTTON_PIN) != LOW) {
    return;
  }
  unsigned long start = millis();
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    // Fast blink while held so you know when to release.
    digitalWrite(LED_PIN, (millis() / 100) % 2);
    if (millis() - start > FACTORY_RESET_HOLD_MS) {
      // Leaves the network only: the rain total stays in flash.
      Serial.println("Factory reset: leaving network and rebooting");
      saveTipsIfChanged();
      ledOn();
      delay(1000);
      Zigbee.factoryReset();  // reboots
    }
  }
  ledOff();
}

bool joinNetwork(bool coldBoot) {
  esp_zb_cfg_t config = ZIGBEE_DEFAULT_ED_CONFIG();
  // Poll the parent every 10 s while awake; the parent forgets us after 64 min without news.
  config.nwk_cfg.zed_cfg.keep_alive = 10000;
  Zigbee.setTimeout(JOIN_TIMEOUT_MS);
  if (!Zigbee.begin(&config, false)) {
    Serial.println("Zigbee failed to start");
    return false;
  }

  unsigned long timeout = coldBoot ? JOIN_TIMEOUT_PAIRING_MS : JOIN_TIMEOUT_MS;
  if (coldBoot) {
    Serial.println("Joining: open \"Permit join\" in Zigbee2MQTT now");
  }
  unsigned long start = millis();
  while (!Zigbee.connected() && millis() - start < timeout) {
    // Slow blink while searching (only after power-on: no LED on a normal wake).
    if (coldBoot) {
      digitalWrite(LED_PIN, (millis() / 500) % 2);
      checkFactoryReset();
    }
    pollRain();
    delay(50);
  }
  ledOff();
  return Zigbee.connected();
}

/* ---------- Sleep ---------- */

void deepSleep() {
  saveTipsIfChanged();
  uint64_t now = nowMs();
  rtc.clock.advance(now, TICK_MS);
  uint64_t sleepMs = rtc.clock.sleepMs(now);

  digitalWrite(VANE_POWER_PIN, LOW);
  ledOff();
  Serial.printf("Sleeping %llu ms\n", (unsigned long long)sleepMs);
  Serial.flush();

  esp_sleep_enable_timer_wakeup(sleepMs * 1000ULL);
  // A reed stuck closed would wake us in a loop: only arm the rain wake while it is open.
  if (digitalRead(RAIN_PIN) == HIGH) {
    esp_sleep_enable_ext1_wakeup_io(1ULL << RAIN_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
  }
  esp_deep_sleep_start();
}

/* ---------- Arduino ---------- */

void report(bool coldBoot) {
  Readings readings = readSensors();
  wx::WindReport wind = rtc.wind.take();

  setupEndpoints(readings.batteryVolts);
  Zigbee.onGlobalDefaultResponse(onGlobalResponse);
  if (!joinNetwork(coldBoot)) {
    Serial.println("No network, data kept for the next report");
    return;
  }

  sendReadings(readings, wind);
  rtc.rain.markReported();
  rtc.schedule.reported();
  dryHumiditySensorIfNeeded(readings);

  if (coldBoot) {
    // Stay reachable while Zigbee2MQTT interviews and configures the station.
    Serial.println("Awake for 2 minutes (hold BOOT 3 s to leave the network)");
    unsigned long start = millis();
    unsigned long lastReport = start;
    while (millis() - start < AWAKE_AFTER_BOOT_MS) {
      checkFactoryReset();
      pollRain();
      if (millis() - lastReport >= AWAKE_REPORT_MS) {
        lastReport = millis();
        sendReadings(readSensors(), wind);
      }
      delay(20);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  ledOff();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT);        // external 100 kΩ pull-up
  pinMode(ANEMOMETER_PIN, INPUT);  // external 100 kΩ pull-up
  pinMode(VANE_POWER_PIN, OUTPUT);
  digitalWrite(VANE_POWER_PIN, LOW);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(VANE_ADC_PIN, ADC_11db);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  bool coldBoot = cause == ESP_SLEEP_WAKEUP_UNDEFINED || rtc.magic != RTC_MAGIC;
  uint64_t now = nowMs();

  if (coldBoot) {
    memset(&rtc, 0, sizeof(rtc));
    rtc.magic = RTC_MAGIC;
    rtc.savedTips = loadStoredTips();
    rtc.rain.reset(rtc.savedTips);
    rtc.wind.reset();
    rtc.lightning.reset();
    rtc.schedule.reset();
    rtc.clock.start(now, TICK_MS);
    Serial.printf("Weather station starting, rain total %.2f mm\n", rtc.rain.totalMm());
    detectModules();
  } else if (cause == ESP_SLEEP_WAKEUP_EXT1) {
    rtc.rain.tip(now);
  }
  rainLevelHigh = digitalRead(RAIN_PIN) == HIGH;

  bool sendReport = coldBoot;
  if (rtc.clock.due(now)) {
    runTick();
    rtc.clock.advance(nowMs(), TICK_MS);
    sendReport = rtc.schedule.onTick(REPORT_EVERY_TICKS, rtc.rain.pending()) || sendReport;
  }

  if (sendReport) {
    useExternalAntenna();
    report(coldBoot);
  }

  waitForRainRelease();
  deepSleep();
}

void loop() {
  // Never reached: setup() always ends in deep sleep.
}
