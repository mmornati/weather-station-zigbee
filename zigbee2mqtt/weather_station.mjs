// Zigbee2MQTT external converter for the DIY XIAO ESP32-C6 weather station.
// Copy to <z2m data dir>/external_converters/weather_station.mjs and restart Zigbee2MQTT.
//
// Endpoints (see firmware/weather_station/weather_station.ino):
//   1  genAnalogInput + genPowerCfg   wind speed, km/h (5 min average) + battery
//   2  genAnalogInput                 wind gust, km/h
//   3  genAnalogInput                 wind direction, degrees (0 = north)
//   4  genAnalogInput                 rain since installation, mm
//   5  genAnalogInput                 UV index (optional LTR390)
//   6  genAnalogInput                 distance of the last lightning strike, km (optional AS3935)
//   7  genAnalogInput                 lightning strikes since power-on (optional AS3935)
//   10 temperature + humidity, 11 pressure, 12 illuminance (optional LTR390)
//   13 ground temperature, 14 soil temperature (optional DS18B20), 15 box temperature
//
// The station is a sleepy end device: it only listens for a few seconds every 5 minutes,
// so nothing here writes to it.
import * as fz from 'zigbee-herdsman-converters/converters/fromZigbee';
import * as exposes from 'zigbee-herdsman-converters/lib/exposes';
import * as reporting from 'zigbee-herdsman-converters/lib/reporting';

const e = exposes.presets;
const ea = exposes.access;

const EP = {
    windSpeed: 1,
    windGust: 2,
    windDirection: 3,
    rain: 4,
    uv: 5,
    lightningDistance: 6,
    lightningCount: 7,
    climate: 10,
    pressure: 11,
    illuminance: 12,
    groundTemperature: 13,
    soilTemperature: 14,
    boxTemperature: 15,
};

const round = (value, decimals) => Number(value.toFixed(decimals));

const fzAnalogInput = {
    cluster: 'genAnalogInput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const value = msg.data.presentValue;
        if (typeof value !== 'number' || !Number.isFinite(value)) return;
        switch (msg.endpoint.ID) {
            case EP.windSpeed:
                return {wind_speed: round(Math.max(0, value), 1)};
            case EP.windGust:
                return {wind_gust: round(Math.max(0, value), 1)};
            case EP.windDirection:
                // The firmware never sends an unknown direction; guard anyway.
                if (value < 0 || value > 360) return;
                return {wind_direction: Math.round(value) % 360};
            case EP.rain:
                return {rain: round(Math.max(0, value), 2)};
            case EP.uv:
                return {uv_index: round(Math.max(0, value), 1)};
            case EP.lightningDistance:
                return {lightning_distance: Math.round(value)};
            case EP.lightningCount:
                return {lightning_count: Math.round(value)};
        }
    },
};

const TEMPERATURE_KEYS = {
    [EP.climate]: 'temperature',
    [EP.groundTemperature]: 'ground_temperature',
    [EP.soilTemperature]: 'soil_temperature',
    [EP.boxTemperature]: 'box_temperature',
};

const fzTemperature = {
    cluster: 'msTemperatureMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const value = msg.data.measuredValue;
        const key = TEMPERATURE_KEYS[msg.endpoint.ID];
        // -32768 (0x8000) is the ZCL "invalid" value.
        if (typeof value !== 'number' || value === -32768 || !key) return;
        return {[key]: round(value / 100, 2)};
    },
};

const fzHumidity = {
    cluster: 'msRelativeHumidity',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const value = msg.data.measuredValue;
        if (typeof value !== 'number' || msg.endpoint.ID !== EP.climate) return;
        const humidity = value / 100;
        if (humidity >= 0 && humidity <= 100) return {humidity: round(humidity, 1)};
    },
};

// The Illuminance cluster uses a log scale: 10000 * log10(lux) + 1.
const fzIlluminance = {
    cluster: 'msIlluminanceMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg) => {
        const value = msg.data.measuredValue;
        if (typeof value !== 'number' || value === 0xffff) return;
        const lux = value === 0 ? 0 : 10 ** ((value - 1) / 10000);
        return {illuminance: Math.round(lux)};
    },
};

export default {
    zigbeeModel: ['C6WeatherStation'],
    model: 'C6WeatherStation',
    vendor: 'DIY',
    description: 'Solar weather station: wind, rain, temperature, humidity, pressure, UV, lightning (XIAO ESP32-C6)',
    fromZigbee: [fzAnalogInput, fzTemperature, fzHumidity, fz.pressure, fzIlluminance, fz.battery],
    toZigbee: [],
    // Only what the station has: optional modules show up once their endpoint exists.
    exposes: (device) => {
        const has = (id) => !device || Boolean(device.getEndpoint?.(id));
        const list = [
            e.numeric('wind_speed', ea.STATE).withUnit('km/h').withDescription('Average wind speed over the last 5 minutes'),
            e.numeric('wind_gust', ea.STATE).withUnit('km/h').withDescription('Strongest 3-second wind over the last 5 minutes'),
            e
                .numeric('wind_direction', ea.STATE)
                .withUnit('°')
                .withValueMin(0)
                .withValueMax(359)
                .withDescription('Where the wind comes from: 0 = north, 90 = east'),
            e.numeric('rain', ea.STATE).withUnit('mm').withDescription('Rain since installation (never resets)'),
        ];
        if (has(EP.climate)) list.push(e.temperature(), e.humidity());
        if (has(EP.pressure)) list.push(e.pressure());
        if (has(EP.uv)) list.push(e.numeric('uv_index', ea.STATE).withDescription('UV index'));
        if (has(EP.illuminance)) list.push(e.illuminance());
        if (has(EP.groundTemperature)) {
            list.push(e.numeric('ground_temperature', ea.STATE).withUnit('°C').withDescription('Temperature at grass level (frost)'));
        }
        if (has(EP.soilTemperature)) {
            list.push(e.numeric('soil_temperature', ea.STATE).withUnit('°C').withDescription('Soil temperature at 10 cm'));
        }
        if (has(EP.lightningCount)) {
            list.push(
                e.numeric('lightning_distance', ea.STATE).withUnit('km').withDescription('Distance of the last lightning strike, 63 = out of range'),
                e.numeric('lightning_count', ea.STATE).withDescription('Lightning strikes since the station was powered on'),
            );
        }
        if (has(EP.boxTemperature)) {
            list.push(
                e
                    .numeric('box_temperature', ea.STATE)
                    .withUnit('°C')
                    .withDescription('Temperature inside the electronics box, next to the battery')
                    .withCategory('diagnostic'),
            );
        }
        list.push(e.battery(), e.battery_voltage());
        return list;
    },
    configure: async (device, coordinatorEndpoint) => {
        // The firmware sends its reports to the bound coordinator: bind what exists.
        const bindings = [
            [EP.windSpeed, ['genAnalogInput', 'genPowerCfg']],
            [EP.windGust, ['genAnalogInput']],
            [EP.windDirection, ['genAnalogInput']],
            [EP.rain, ['genAnalogInput']],
            [EP.uv, ['genAnalogInput']],
            [EP.lightningDistance, ['genAnalogInput']],
            [EP.lightningCount, ['genAnalogInput']],
            [EP.climate, ['msTemperatureMeasurement', 'msRelativeHumidity']],
            [EP.pressure, ['msPressureMeasurement']],
            [EP.illuminance, ['msIlluminanceMeasurement']],
            [EP.groundTemperature, ['msTemperatureMeasurement']],
            [EP.soilTemperature, ['msTemperatureMeasurement']],
            [EP.boxTemperature, ['msTemperatureMeasurement']],
        ];
        for (const [id, clusters] of bindings) {
            const endpoint = device.getEndpoint(id);
            if (endpoint) await reporting.bind(endpoint, coordinatorEndpoint, clusters);
        }
        await device.getEndpoint(EP.windSpeed)?.read('genPowerCfg', ['batteryPercentageRemaining', 'batteryVoltage']);
    },
};
