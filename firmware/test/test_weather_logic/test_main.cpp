// Host-side unit tests for weather_logic.h (run with: pio test -e native)
#include <unity.h>

#include "weather_logic.h"

using namespace wx;

void setUp() {}
void tearDown() {}

// Voltage the vane gives for a given resistance with the 10 kΩ pull-up on 3.3 V.
static uint32_t vaneMv(float ohms) {
  return (uint32_t)lroundf(3300.0f * ohms / (ohms + 10000.0f));
}

/* ---------- Vane ---------- */

void test_vane_decodes_all_16_positions() {
  for (uint8_t i = 0; i < 16; i++) {
    TEST_ASSERT_EQUAL_FLOAT(VANE_TABLE[i].degrees, vaneDegrees(vaneMv(VANE_TABLE[i].ohms), 3300));
  }
}

void test_vane_tolerates_adc_error() {
  // 3 % off on 90° (1 kΩ, 300 mV) still reads east, not 112.5° (688 Ω, 213 mV).
  TEST_ASSERT_EQUAL_FLOAT(90.0f, vaneDegrees(309, 3300));
  TEST_ASSERT_EQUAL_FLOAT(270.0f, vaneDegrees(vaneMv(120000.0f) - 30, 3300));
}

void test_vane_rejects_open_and_short() {
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, vaneDegrees(3290, 3300));  // cable unplugged
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, vaneDegrees(10, 3300));    // short circuit
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, vaneDegrees(1000, 0));
}

void test_normalize_degrees() {
  TEST_ASSERT_EQUAL_FLOAT(350.0f, normalizeDegrees(-10.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, normalizeDegrees(360.0f));
  TEST_ASSERT_EQUAL_FLOAT(45.0f, normalizeDegrees(405.0f));
}

/* ---------- Wind ---------- */

void test_one_pulse_per_second_is_2_4_kmh() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.4f, WindAccumulator::speedKmh(3, 3000));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, WindAccumulator::speedKmh(5, 0));
}

void test_wind_average_and_gust_over_five_windows() {
  WindAccumulator wind;
  wind.reset();
  // 5 windows of 3 s: 1, 2, 10, 2, 0 pulses per second
  wind.addWindow(3, 3000, 90.0f);
  wind.addWindow(6, 3000, 90.0f);
  wind.addWindow(30, 3000, 90.0f);
  wind.addWindow(6, 3000, 90.0f);
  wind.addWindow(0, 3000, 90.0f);
  WindReport report = wind.take();
  // 45 pulses in 15 s = 3 Hz = 7.2 km/h; best window 10 Hz = 24 km/h
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.2f, report.averageKmh);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 24.0f, report.gustKmh);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, report.directionDeg);
}

void test_wind_direction_averages_across_north() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(10, 3000, 337.5f);
  wind.addWindow(10, 3000, 22.5f);
  WindReport report = wind.take();
  float d = report.directionDeg;
  TEST_ASSERT_TRUE(d < 0.5f || d > 359.5f);
}

void test_wind_direction_is_weighted_by_speed() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(30, 3000, 0.0f);   // strong north
  wind.addWindow(3, 3000, 180.0f);  // light south
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, wind.take().directionDeg);
}

void test_calm_keeps_last_direction() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(9, 3000, 225.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 225.0f, wind.take().directionDeg);
  wind.addWindow(0, 3000, 45.0f);
  wind.addWindow(0, 3000, 45.0f);
  WindReport calm = wind.take();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calm.averageKmh);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calm.gustKmh);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 225.0f, calm.directionDeg);
}

void test_calm_from_cold_boot_uses_vane_position() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(0, 3000, 135.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 135.0f, wind.take().directionDeg);
}

void test_direction_unknown_without_vane() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(12, 3000, -1.0f);
  WindReport report = wind.take();
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.6f, report.averageKmh);
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, report.directionDeg);
}

void test_take_starts_a_new_period() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(30, 3000, 90.0f);
  wind.take();
  TEST_ASSERT_TRUE(wind.empty());
  wind.addWindow(3, 3000, 90.0f);
  WindReport report = wind.take();
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.4f, report.gustKmh);
}

void test_zero_length_window_is_ignored() {
  WindAccumulator wind;
  wind.reset();
  wind.addWindow(50, 0, 90.0f);
  TEST_ASSERT_TRUE(wind.empty());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, wind.take().averageKmh);
}

/* ---------- Rain ---------- */

void test_rain_counts_tips_in_mm() {
  RainCounter rain;
  rain.reset(0);
  for (int i = 0; i < 10; i++) {
    TEST_ASSERT_TRUE(rain.tip(1000ULL * (i + 1)));
  }
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.794f, rain.totalMm());
}

void test_rain_ignores_reed_bounce() {
  RainCounter rain;
  rain.reset(0);
  TEST_ASSERT_TRUE(rain.tip(10000));
  TEST_ASSERT_FALSE(rain.tip(10050));
  TEST_ASSERT_FALSE(rain.tip(10299));
  TEST_ASSERT_TRUE(rain.tip(10400));
  TEST_ASSERT_EQUAL_UINT32(2, rain.tips);
}

void test_rain_first_tip_right_after_boot_counts() {
  RainCounter rain;
  rain.reset(0);
  TEST_ASSERT_TRUE(rain.tip(5));
}

void test_rain_resumes_from_stored_total() {
  RainCounter rain;
  rain.reset(1000);
  TEST_ASSERT_FALSE(rain.pending());
  rain.tip(1);
  TEST_ASSERT_TRUE(rain.pending());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 279.6794f, rain.totalMm());
  rain.markReported();
  TEST_ASSERT_FALSE(rain.pending());
}

/* ---------- Lightning ---------- */

void test_lightning_counts_only_strikes() {
  LightningLog log;
  log.reset();
  TEST_ASSERT_FALSE(log.add(0, 0));
  TEST_ASSERT_FALSE(log.add(AS3935_INT_DISTURBER, 0));
  TEST_ASSERT_FALSE(log.add(AS3935_INT_NOISE, 0));
  TEST_ASSERT_TRUE(log.add(AS3935_INT_LIGHTNING, 14));
  TEST_ASSERT_TRUE(log.add(AS3935_INT_LIGHTNING, 8));
  TEST_ASSERT_EQUAL_UINT32(2, log.strikes);
  TEST_ASSERT_EQUAL_UINT8(8, log.lastDistanceKm);
}

/* ---------- Tick clock and schedule ---------- */

void test_tick_clock_keeps_the_minute_through_rain_wakes() {
  TickClock clock;
  clock.start(0, 60000);
  TEST_ASSERT_FALSE(clock.due(25000));                     // a rain tip wakes us at 25 s
  TEST_ASSERT_EQUAL_UINT64(35000, clock.sleepMs(25000));   // back to sleep until 60 s
  TEST_ASSERT_TRUE(clock.due(60010));
  clock.advance(63500, 60000);                             // tick took 3.5 s
  TEST_ASSERT_EQUAL_UINT64(120000, clock.nextTickMs);
}

void test_tick_clock_skips_missed_ticks() {
  TickClock clock;
  clock.start(0, 60000);
  clock.advance(250000, 60000);  // stayed awake 4 minutes (pairing)
  TEST_ASSERT_EQUAL_UINT64(300000, clock.nextTickMs);
  TEST_ASSERT_EQUAL_UINT64(0, clock.sleepMs(300001));
}

void test_report_every_five_ticks() {
  ReportSchedule schedule;
  schedule.reset();
  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_FALSE(schedule.onTick(5, false));
  }
  TEST_ASSERT_TRUE(schedule.onTick(5, false));
  schedule.reported();
  TEST_ASSERT_FALSE(schedule.onTick(5, false));
}

void test_report_on_next_tick_when_rain_is_pending() {
  ReportSchedule schedule;
  schedule.reset();
  TEST_ASSERT_TRUE(schedule.onTick(5, true));
}

/* ---------- Battery ---------- */

void test_battery_percent_curve() {
  TEST_ASSERT_EQUAL_UINT8(100, batteryPercent(4.25f));
  TEST_ASSERT_EQUAL_UINT8(100, batteryPercent(4.20f));
  TEST_ASSERT_EQUAL_UINT8(50, batteryPercent(3.82f));
  TEST_ASSERT_EQUAL_UINT8(85, batteryPercent(4.02f));
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(3.0f));
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(0.0f));
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(NAN));
}

void test_battery_percent_is_monotonic() {
  uint8_t previous = 0;
  for (float v = 2.9f; v <= 4.3f; v += 0.005f) {
    uint8_t p = batteryPercent(v);
    TEST_ASSERT_TRUE(p >= previous);
    previous = p;
  }
}

void test_zcl_battery_voltage_in_100mv() {
  TEST_ASSERT_EQUAL_UINT8(37, zclBatteryVoltage(3.71f));
  TEST_ASSERT_EQUAL_UINT8(42, zclBatteryVoltage(4.2f));
  TEST_ASSERT_EQUAL_UINT8(0, zclBatteryVoltage(-1.0f));
}

/* ---------- Light ---------- */

void test_zcl_illuminance_log_scale() {
  TEST_ASSERT_EQUAL_UINT16(0, zclIlluminance(0.0f));
  TEST_ASSERT_EQUAL_UINT16(1, zclIlluminance(1.0f));
  TEST_ASSERT_EQUAL_UINT16(10001, zclIlluminance(10.0f));
  TEST_ASSERT_EQUAL_UINT16(50001, zclIlluminance(100000.0f));
  TEST_ASSERT_EQUAL_UINT16(65534, zclIlluminance(1e9f));
}

void test_uv_index_from_counts() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, uvIndex(0));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, uvIndex(2300));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, uvIndex(2300, 2.0f));
}

void test_lux_from_als_counts() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 600.0f, luxFromAls(1000, 1.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, luxFromAls(1000, 3.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, luxFromAls(1000, 0.0f));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_vane_decodes_all_16_positions);
  RUN_TEST(test_vane_tolerates_adc_error);
  RUN_TEST(test_vane_rejects_open_and_short);
  RUN_TEST(test_normalize_degrees);
  RUN_TEST(test_one_pulse_per_second_is_2_4_kmh);
  RUN_TEST(test_wind_average_and_gust_over_five_windows);
  RUN_TEST(test_wind_direction_averages_across_north);
  RUN_TEST(test_wind_direction_is_weighted_by_speed);
  RUN_TEST(test_calm_keeps_last_direction);
  RUN_TEST(test_calm_from_cold_boot_uses_vane_position);
  RUN_TEST(test_direction_unknown_without_vane);
  RUN_TEST(test_take_starts_a_new_period);
  RUN_TEST(test_zero_length_window_is_ignored);
  RUN_TEST(test_rain_counts_tips_in_mm);
  RUN_TEST(test_rain_ignores_reed_bounce);
  RUN_TEST(test_rain_first_tip_right_after_boot_counts);
  RUN_TEST(test_rain_resumes_from_stored_total);
  RUN_TEST(test_lightning_counts_only_strikes);
  RUN_TEST(test_tick_clock_keeps_the_minute_through_rain_wakes);
  RUN_TEST(test_tick_clock_skips_missed_ticks);
  RUN_TEST(test_report_every_five_ticks);
  RUN_TEST(test_report_on_next_tick_when_rain_is_pending);
  RUN_TEST(test_battery_percent_curve);
  RUN_TEST(test_battery_percent_is_monotonic);
  RUN_TEST(test_zcl_battery_voltage_in_100mv);
  RUN_TEST(test_zcl_illuminance_log_scale);
  RUN_TEST(test_uv_index_from_counts);
  RUN_TEST(test_lux_from_als_counts);
  return UNITY_END();
}
