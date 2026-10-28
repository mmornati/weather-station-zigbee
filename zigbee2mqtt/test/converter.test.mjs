// Integration tests: load the external converter through zigbee-herdsman-converters,
// the same library Zigbee2MQTT uses, and check what Z2M will see and send.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import * as zhc from 'zigbee-herdsman-converters';

import converter from '../weather_station.mjs';

// Must match the firmware (firmware/weather_station/weather_station.ino)
const FIRMWARE_MANUFACTURER = 'DIY';
const FIRMWARE_MODEL = 'C6WeatherStation';
const sketch = readFileSync(new URL('../../firmware/weather_station/weather_station.ino', import.meta.url), 'utf8');

const definition = zhc.prepareDefinition({...converter});

const ALL_ENDPOINTS = [1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15];
const CORE_ENDPOINTS = [1, 2, 3, 4];

function fakeDevice(endpointIds) {
    const calls = [];
    const endpoints = new Map(
        endpointIds.map((ID) => [
            ID,
            {
                ID,
                read: async (cluster, attributes) => calls.push({op: 'read', ID, cluster, attributes}),
                bind: async (cluster) => calls.push({op: 'bind', ID, cluster}),
                configureReporting: async () => {},
            },
        ]),
    );
    return {calls, getEndpoint: (ID) => endpoints.get(ID)};
}

function exposes(device) {
    const list = typeof definition.exposes === 'function' ? definition.exposes(device, {}) : definition.exposes;
    return list.filter((e) => e.name !== 'linkquality');
}

function properties(device) {
    return exposes(device)
        .map((e) => e.property)
        .sort();
}

function expose(property, device) {
    const found = exposes(device).find((e) => e.property === property);
    assert.ok(found, `${property} not exposed`);
    return found;
}

async function decode(cluster, endpointId, data, type = 'attributeReport') {
    const device = {modelID: FIRMWARE_MODEL, manufacturerName: FIRMWARE_MANUFACTURER};
    const meta = {device, logger: console, state: {}};
    const endpoint = {ID: endpointId, getClusterAttributeValue: () => undefined};
    const msg = {type, cluster, data, endpoint, device};
    const result = {};
    for (const fz of definition.fromZigbee) {
        const types = Array.isArray(fz.type) ? fz.type : [fz.type];
        if (fz.cluster === cluster && types.includes(type)) {
            Object.assign(result, await fz.convert(definition, msg, () => {}, {}, meta));
        }
    }
    return result;
}

function firmwareDefine(name) {
    const match = sketch.match(new RegExp(`#define\\s+${name}\\s+(\\d+)`));
    assert.ok(match, `${name} not defined in the sketch`);
    return Number(match[1]);
}

test('firmware source still sends the expected manufacturer and model', () => {
    const match = sketch.match(/setManufacturerAndModel\("([^"]+)",\s*"([^"]+)"\)/);
    assert.ok(match, 'setManufacturerAndModel() call not found in the sketch');
    assert.equal(match[1], FIRMWARE_MANUFACTURER);
    assert.equal(match[2], FIRMWARE_MODEL);
});

test('endpoint numbers match the firmware', () => {
    assert.equal(firmwareDefine('EP_WIND_SPEED'), 1);
    assert.equal(firmwareDefine('EP_WIND_GUST'), 2);
    assert.equal(firmwareDefine('EP_WIND_DIRECTION'), 3);
    assert.equal(firmwareDefine('EP_RAIN'), 4);
    assert.equal(firmwareDefine('EP_UV'), 5);
    assert.equal(firmwareDefine('EP_LIGHTNING_DISTANCE'), 6);
    assert.equal(firmwareDefine('EP_LIGHTNING_COUNT'), 7);
    assert.equal(firmwareDefine('EP_CLIMATE'), 10);
    assert.equal(firmwareDefine('EP_PRESSURE'), 11);
    assert.equal(firmwareDefine('EP_ILLUMINANCE'), 12);
    assert.equal(firmwareDefine('EP_GROUND_TEMPERATURE'), 13);
    assert.equal(firmwareDefine('EP_SOIL_TEMPERATURE'), 14);
    assert.equal(firmwareDefine('EP_BOX_TEMPERATURE'), 15);
});

test('matches the model string sent by the firmware', () => {
    assert.deepEqual(converter.zigbeeModel, [FIRMWARE_MODEL]);
    assert.equal(converter.model, FIRMWARE_MODEL);
    assert.equal(converter.vendor, FIRMWARE_MANUFACTURER);
});

test('Zigbee2MQTT resolves a battery end device reporting the firmware model', async () => {
    zhc.addExternalDefinition({...converter, externalConverterName: 'weather_station.mjs'});
    try {
        const device = {
            ieeeAddr: '0x0000000000000001',
            modelID: FIRMWARE_MODEL,
            manufacturerName: FIRMWARE_MANUFACTURER,
            type: 'EndDevice',
            powerSource: 'Battery',
            endpoints: [],
            interviewing: false,
            interviewCompleted: true,
            getEndpoint: () => undefined,
        };
        const found = await zhc.findByDevice(device);
        assert.ok(found, 'definition not found');
        assert.equal(found.model, FIRMWARE_MODEL);
        assert.equal(found.vendor, FIRMWARE_MANUFACTURER);
    } finally {
        zhc.removeExternalDefinitions('weather_station.mjs');
    }
});

test('a fully equipped station exposes every sensor', () => {
    assert.deepEqual(properties(fakeDevice(ALL_ENDPOINTS)), [
        'battery',
        'box_temperature',
        'ground_temperature',
        'humidity',
        'illuminance',
        'lightning_count',
        'lightning_distance',
        'pressure',
        'rain',
        'soil_temperature',
        'temperature',
        'uv_index',
        'voltage',
        'wind_direction',
        'wind_gust',
        'wind_speed',
    ]);
});

test('optional modules are only exposed when their endpoint exists', () => {
    assert.deepEqual(properties(fakeDevice(CORE_ENDPOINTS)), ['battery', 'rain', 'voltage', 'wind_direction', 'wind_gust', 'wind_speed']);
    // SHT45 + BME280 only
    assert.deepEqual(properties(fakeDevice([...CORE_ENDPOINTS, 10, 11, 15])), [
        'battery',
        'box_temperature',
        'humidity',
        'pressure',
        'rain',
        'temperature',
        'voltage',
        'wind_direction',
        'wind_gust',
        'wind_speed',
    ]);
});

test('units are the ones Home Assistant expects', () => {
    const device = fakeDevice(ALL_ENDPOINTS);
    assert.equal(expose('wind_speed', device).unit, 'km/h');
    assert.equal(expose('wind_gust', device).unit, 'km/h');
    assert.equal(expose('rain', device).unit, 'mm');
    assert.equal(expose('temperature', device).unit, '°C');
    assert.equal(expose('pressure', device).unit, 'hPa');
    assert.equal(expose('illuminance', device).unit, 'lx');
    assert.equal(expose('box_temperature', device).category, 'diagnostic');
});

test('nothing is writable: the station sleeps', () => {
    for (const e of exposes(fakeDevice(ALL_ENDPOINTS))) {
        assert.equal(e.access & 0b010, 0, `${e.property} must not expose SET`);
    }
    assert.deepEqual(converter.toZigbee, []);
});

test('wind endpoints -> wind_speed, wind_gust, wind_direction', async () => {
    assert.deepEqual(await decode('genAnalogInput', 1, {presentValue: 12.345}), {wind_speed: 12.3});
    assert.deepEqual(await decode('genAnalogInput', 2, {presentValue: 41.04}), {wind_gust: 41});
    assert.deepEqual(await decode('genAnalogInput', 3, {presentValue: 247.5}), {wind_direction: 248});
    assert.deepEqual(await decode('genAnalogInput', 3, {presentValue: 359.7}), {wind_direction: 0});
    assert.deepEqual(await decode('genAnalogInput', 3, {presentValue: -1}), {});
    assert.deepEqual(await decode('genAnalogInput', 1, {presentValue: 0}, 'readResponse'), {wind_speed: 0});
});

test('endpoint 4 -> rain total in mm', async () => {
    assert.deepEqual(await decode('genAnalogInput', 4, {presentValue: 1234.5678}), {rain: 1234.57});
});

test('optional analog endpoints -> uv_index and lightning', async () => {
    assert.deepEqual(await decode('genAnalogInput', 5, {presentValue: 6.25}), {uv_index: 6.3});
    assert.deepEqual(await decode('genAnalogInput', 6, {presentValue: 14}), {lightning_distance: 14});
    assert.deepEqual(await decode('genAnalogInput', 7, {presentValue: 3}), {lightning_count: 3});
});

test('temperatures are routed by endpoint', async () => {
    assert.deepEqual(await decode('msTemperatureMeasurement', 10, {measuredValue: 1234}), {temperature: 12.34});
    assert.deepEqual(await decode('msTemperatureMeasurement', 13, {measuredValue: -312}), {ground_temperature: -3.12});
    assert.deepEqual(await decode('msTemperatureMeasurement', 14, {measuredValue: 875}), {soil_temperature: 8.75});
    assert.deepEqual(await decode('msTemperatureMeasurement', 15, {measuredValue: 2100}), {box_temperature: 21});
    assert.deepEqual(await decode('msTemperatureMeasurement', 13, {measuredValue: -32768}), {});
});

test('humidity and pressure', async () => {
    assert.deepEqual(await decode('msRelativeHumidity', 10, {measuredValue: 8765}), {humidity: 87.7});
    assert.deepEqual(await decode('msPressureMeasurement', 11, {measuredValue: 1013}), {pressure: 1013});
});

test('illuminance converts the Zigbee log scale back to lux', async () => {
    assert.deepEqual(await decode('msIlluminanceMeasurement', 12, {measuredValue: 0}), {illuminance: 0});
    assert.deepEqual(await decode('msIlluminanceMeasurement', 12, {measuredValue: 10001}), {illuminance: 10});
    assert.deepEqual(await decode('msIlluminanceMeasurement', 12, {measuredValue: 50001}), {illuminance: 100000});
});

test('battery percentage from the Power Configuration cluster', async () => {
    assert.deepEqual(await decode('genPowerCfg', 1, {batteryPercentageRemaining: 170}), {battery: 85});
    assert.deepEqual(await decode('genPowerCfg', 1, {batteryVoltage: 39}, 'readResponse'), {voltage: 3900});
});

test('configure binds every endpoint the station has, and nothing else', async () => {
    const device = fakeDevice([...CORE_ENDPOINTS, 10, 11]);
    const coordinator = {ID: 1};
    await definition.configure(device, coordinator, definition);
    const bound = device.calls.filter((c) => c.op === 'bind').map((c) => `${c.ID}:${c.cluster}`).sort();
    assert.deepEqual(bound, [
        '10:msRelativeHumidity',
        '10:msTemperatureMeasurement',
        '11:msPressureMeasurement',
        '1:genAnalogInput',
        '1:genPowerCfg',
        '2:genAnalogInput',
        '3:genAnalogInput',
        '4:genAnalogInput',
    ]);
    assert.ok(device.calls.some((c) => c.op === 'read' && c.ID === 1 && c.cluster === 'genPowerCfg'));
});
