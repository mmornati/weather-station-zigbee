#pragma once

#include <math.h>
#include <stdint.h>

/**
 * Hardware-free logic of the weather station, unit tested on the host (see firmware/test).
 *
 * Everything that lives across deep sleep is a plain struct with no constructor, so it can
 * sit in RTC memory (RTC_DATA_ATTR): a constructor would run again on every wake-up and
 * wipe the counts. Call reset() on a cold boot instead.
 *
 * - vaneDegrees()       wind vane voltage -> direction (16 positions)
 * - WindAccumulator     3 s windows -> average speed, gust and dominant direction
 * - RainCounter         bucket tips -> mm, with a guard against reed bounce
 * - LightningLog        AS3935 events -> strike count and distance of the last one
 * - TickClock           when the next 60 s tick is due, across deep sleep
 * - batteryPercent()    Li-ion voltage -> %
 * - zclIlluminance()    lux -> the log scale of the Zigbee Illuminance cluster
 * - uvIndex(), luxFromAls()  LTR390 counts -> UV index and lux
 */

namespace wx {

/* ---------- Weather meter kit (Misol / Fine Offset / SparkFun SEN-15901) ---------- */

// One reed closure per second = 2.4 km/h.
static const float KMH_PER_HZ = 2.4f;
// One bucket tip = 0.2794 mm of rain.
static const float MM_PER_TIP = 0.2794f;

/* ---------- Wind vane ---------- */

struct VanePosition {
  float degrees;
  float ohms;
};

// The vane closes one or two of eight reed switches, each with its own resistor,
// which gives 16 positions (resistances from the kit's datasheet).
static const VanePosition VANE_TABLE[16] = {
  {0.0f, 33000.0f},   {22.5f, 6570.0f},  {45.0f, 8200.0f},   {67.5f, 891.0f},
  {90.0f, 1000.0f},   {112.5f, 688.0f},  {135.0f, 2200.0f},  {157.5f, 1410.0f},
  {180.0f, 3900.0f},  {202.5f, 3140.0f}, {225.0f, 16000.0f}, {247.5f, 14120.0f},
  {270.0f, 120000.0f}, {292.5f, 42120.0f}, {315.0f, 64900.0f}, {337.5f, 21880.0f},
};

/**
 * Direction in degrees (0 = north, 90 = east) from the vane voltage, read across the vane
 * with `pullupOhms` between the supply and the ADC pin. Returns -1 when the reading cannot
 * come from the vane: open cable (close to the supply) or short circuit (close to 0 V).
 */
static inline float vaneDegrees(uint32_t vaneMillivolts, uint32_t supplyMillivolts, float pullupOhms = 10000.0f) {
  if (supplyMillivolts == 0) {
    return -1.0f;
  }
  float ratio = (float)vaneMillivolts / (float)supplyMillivolts;
  if (ratio < 0.03f || ratio > 0.97f) {
    return -1.0f;
  }
  float best = -1.0f;
  float bestError = 1.0f;
  for (uint8_t i = 0; i < 16; i++) {
    float expected = VANE_TABLE[i].ohms / (VANE_TABLE[i].ohms + pullupOhms);
    float error = fabsf(expected - ratio);
    if (error < bestError) {
      bestError = error;
      best = VANE_TABLE[i].degrees;
    }
  }
  return best;
}

// Turns any angle into 0 <= degrees < 360.
static inline float normalizeDegrees(float degrees) {
  float d = fmodf(degrees, 360.0f);
  if (d < 0) {
    d += 360.0f;
  }
  if (d >= 360.0f) {
    d -= 360.0f;
  }
  return d;
}

/* ---------- Wind ---------- */

struct WindReport {
  float averageKmh;   // over every window since the last report
  float gustKmh;      // best single window
  float directionDeg; // speed-weighted vector average; -1 if never known
};

/**
 * Collects the short wind windows (3 s every minute) between two reports.
 * The direction is averaged as a vector weighted by speed, so 350° and 10° give 0°, not 180°,
 * and a calm window does not pull the average. In a calm, the last known direction stays.
 */
struct WindAccumulator {
  uint32_t pulses;
  uint32_t milliseconds;
  float gustKmh;
  float sumX;
  float sumY;
  float lastDirectionDeg;

  void reset() {
    pulses = 0;
    milliseconds = 0;
    gustKmh = 0.0f;
    sumX = 0.0f;
    sumY = 0.0f;
    lastDirectionDeg = -1.0f;
  }

  static float speedKmh(uint32_t windowPulses, uint32_t windowMs) {
    if (windowMs == 0) {
      return 0.0f;
    }
    return windowPulses * 1000.0f / windowMs * KMH_PER_HZ;
  }

  // directionDeg < 0 means the vane could not be read during this window.
  void addWindow(uint32_t windowPulses, uint32_t windowMs, float directionDeg) {
    if (windowMs == 0) {
      return;
    }
    pulses += windowPulses;
    milliseconds += windowMs;
    float speed = speedKmh(windowPulses, windowMs);
    if (speed > gustKmh) {
      gustKmh = speed;
    }
    if (directionDeg >= 0.0f) {
      float radians = directionDeg * (float)M_PI / 180.0f;
      // Weight 0 in a calm: keep the vane's position only as a fallback.
      sumX += speed * sinf(radians);
      sumY += speed * cosf(radians);
      if (speed > 0.0f || lastDirectionDeg < 0.0f) {
        lastDirectionDeg = directionDeg;
      }
    }
  }

  bool empty() const {
    return milliseconds == 0;
  }

  // Returns the report and starts a new period (the last direction is kept).
  WindReport take() {
    WindReport report;
    report.averageKmh = speedKmh(pulses, milliseconds);
    report.gustKmh = gustKmh;
    if (sumX != 0.0f || sumY != 0.0f) {
      report.directionDeg = normalizeDegrees(atan2f(sumX, sumY) * 180.0f / (float)M_PI);
      lastDirectionDeg = report.directionDeg;
    } else {
      report.directionDeg = lastDirectionDeg;
    }
    float keep = lastDirectionDeg;
    reset();
    lastDirectionDeg = keep;
    return report;
  }
};

/* ---------- Rain ---------- */

/**
 * Counts bucket tips. A tip closes the reed for ~100 ms; a second closure within
 * `minGapMs` is reed bounce, not rain (the bucket cannot tip that fast).
 */
struct RainCounter {
  uint32_t tips;
  uint32_t reportedTips;
  uint64_t lastTipMs;
  bool anyTip;

  void reset(uint32_t storedTips) {
    tips = storedTips;
    reportedTips = storedTips;
    lastTipMs = 0;
    anyTip = false;
  }

  bool tip(uint64_t nowMs, uint32_t minGapMs = 300) {
    if (anyTip && nowMs - lastTipMs < minGapMs) {
      return false;
    }
    anyTip = true;
    lastTipMs = nowMs;
    tips++;
    return true;
  }

  float totalMm() const {
    return tips * MM_PER_TIP;
  }

  // True while some rain has not been reported yet.
  bool pending() const {
    return tips != reportedTips;
  }

  void markReported() {
    reportedTips = tips;
  }
};

/* ---------- Lightning (AS3935) ---------- */

static const uint8_t AS3935_INT_NOISE = 0x01;
static const uint8_t AS3935_INT_DISTURBER = 0x04;
static const uint8_t AS3935_INT_LIGHTNING = 0x08;

/**
 * Keeps what the AS3935 saw between two polls: it latches the cause of its last interrupt,
 * so polling once a minute catches at most one strike per minute, which is enough for a
 * "storm at N km" alert. The count only grows (HA: total_increasing).
 */
struct LightningLog {
  uint32_t strikes;
  uint8_t lastDistanceKm;  // 1 = overhead, 63 = out of range, 0 = none seen yet

  void reset() {
    strikes = 0;
    lastDistanceKm = 0;
  }

  // Returns true when the event is a strike.
  bool add(uint8_t interruptCause, uint8_t distanceKm) {
    if ((interruptCause & AS3935_INT_LIGHTNING) == 0) {
      return false;
    }
    strikes++;
    lastDistanceKm = distanceKm;
    return true;
  }
};

/* ---------- Time across deep sleep ---------- */

/**
 * The station wakes every `tickMs` (60 s). Rain tips can wake it in between; those wakes
 * must not shift the minute, so the next deadline is kept rather than "now + 60 s".
 */
struct TickClock {
  uint64_t nextTickMs;

  void start(uint64_t nowMs, uint32_t tickMs) {
    nextTickMs = nowMs + tickMs;
  }

  bool due(uint64_t nowMs) const {
    return nowMs >= nextTickMs;
  }

  // Moves to the next deadline after now; missed ticks are skipped, not replayed.
  void advance(uint64_t nowMs, uint32_t tickMs) {
    if (tickMs == 0) {
      return;
    }
    if (nowMs >= nextTickMs) {
      uint64_t missed = (nowMs - nextTickMs) / tickMs + 1;
      nextTickMs += missed * tickMs;
    }
  }

  uint64_t sleepMs(uint64_t nowMs) const {
    return nowMs >= nextTickMs ? 0 : nextTickMs - nowMs;
  }
};

/**
 * Decides when a tick sends a report: every `everyTicks` ticks, or on the next tick
 * as soon as new rain is waiting.
 */
struct ReportSchedule {
  uint16_t ticksSinceReport;

  void reset() {
    ticksSinceReport = 0;
  }

  bool onTick(uint16_t everyTicks, bool rainPending) {
    ticksSinceReport++;
    return ticksSinceReport >= everyTicks || rainPending;
  }

  void reported() {
    ticksSinceReport = 0;
  }
};

/* ---------- Battery ---------- */

// Resting voltage of a Li-ion cell (NCR18650B class) at a light load, linear in between.
struct BatteryPoint {
  float volts;
  uint8_t percent;
};

static const BatteryPoint BATTERY_CURVE[] = {
  {3.00f, 0},  {3.45f, 5},  {3.68f, 10}, {3.74f, 20}, {3.77f, 30}, {3.79f, 40},
  {3.82f, 50}, {3.87f, 60}, {3.92f, 70}, {3.98f, 80}, {4.06f, 90}, {4.20f, 100},
};

static inline uint8_t batteryPercent(float volts) {
  const uint8_t n = sizeof(BATTERY_CURVE) / sizeof(BATTERY_CURVE[0]);
  if (!(volts > BATTERY_CURVE[0].volts)) {
    return 0;
  }
  if (volts >= BATTERY_CURVE[n - 1].volts) {
    return 100;
  }
  for (uint8_t i = 1; i < n; i++) {
    if (volts < BATTERY_CURVE[i].volts) {
      const BatteryPoint &lo = BATTERY_CURVE[i - 1];
      const BatteryPoint &hi = BATTERY_CURVE[i];
      float t = (volts - lo.volts) / (hi.volts - lo.volts);
      return (uint8_t)lroundf(lo.percent + t * (hi.percent - lo.percent));
    }
  }
  return 100;
}

// Zigbee Power Configuration cluster: battery voltage in units of 100 mV.
static inline uint8_t zclBatteryVoltage(float volts) {
  if (!(volts > 0)) {
    return 0;
  }
  long units = lroundf(volts * 10.0f);
  return units > 255 ? 255 : (uint8_t)units;
}

/* ---------- Light (LTR390) ---------- */

/**
 * Zigbee Illuminance cluster value: 10000 * log10(lux) + 1, 0 for darkness,
 * 0xFFFE at most (0xFFFF means "invalid").
 */
static inline uint16_t zclIlluminance(float lux) {
  if (!(lux >= 1.0f)) {
    return lux > 0.0f ? 1 : 0;
  }
  float value = 10000.0f * log10f(lux) + 1.0f;
  if (value > 65534.0f) {
    return 65534;
  }
  return (uint16_t)lroundf(value);
}

/**
 * LTR390 datasheet: 2300 UVS counts per UV index at gain 18 and 20-bit resolution.
 * The firmware reads UV at gain 18 / 18 bits (100 ms), a quarter of those counts.
 * windowFactor corrects for the PTFE disc in front of the sensor (1 = bare sensor).
 */
static const float LTR390_UVS_COUNTS_PER_UVI = 2300.0f / 4.0f;

static inline float uvIndex(uint32_t uvsCounts, float windowFactor = 1.0f) {
  return uvsCounts / LTR390_UVS_COUNTS_PER_UVI * windowFactor;
}

// Datasheet: lux = 0.6 * ALS / (gain * integration factor), integration factor 1 at 18 bits.
static inline float luxFromAls(uint32_t alsCounts, float gain, float windowFactor = 1.0f) {
  if (!(gain > 0)) {
    return 0.0f;
  }
  return 0.6f * alsCounts / gain * windowFactor;
}

}  // namespace wx
