const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const flow = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "flows.json"), "utf8"));
const nodes = Object.fromEntries(flow.map((node) => [node.id, node]));

function invokeFunction(node, msg, state = new Map(), env = {}) {
    const context = { get: (key) => state.get(key), set: (key, value) => state.set(key, value) };
    const fn = vm.runInNewContext(`(function (msg, context, env) { ${node.func}\n})`, { Date, Number });
    return fn(msg, context, { get: (key) => env[key] });
}

const telemetry = (status = "SAFE") => ({
    deviceId: "esp32-01", ax: 0.02, ay: -0.01, az: 1, gForce: 1, status,
});
const telemetryMsg = (payload) => ({ topic: "fragile-freight/esp32-01/telemetry", payload });
const alertMsg = (payload) => ({ topic: "fragile-freight/esp32-01/alert", payload });
const alertObject = { topic: "fragile-freight/esp32-01/alert", payload: { deviceId: "esp32-01", status: "DANGER", gForce: 2 } };

test("graph validates raw MQTT before topic split; Telegram has no telemetry bypass", () => {
    assert.deepEqual(nodes.in1.wires, [["validate1"]]);
    assert.equal(flow.some((node) => node.type === "json"), false);
    assert.deepEqual(nodes.validate1.wires, [["sw1"]]);
    assert.deepEqual(nodes.sw1.wires[1], ["dbg2", "eb9411b45aa7183e"]);
    assert.deepEqual(nodes.eb9411b45aa7183e.wires, [["f267fced113536c6"]]);
    assert.equal(nodes.in1.qos, "0");
    assert.equal(nodes.in1.rh, 2);
    for (const id of ["mqttout1", "mqttout2"]) {
        assert.equal(nodes[id].qos, "0");
        assert.equal(nodes[id].retain, "false");
    }
});

test("accepts all telemetry states and valid escaped member names", () => {
    for (const status of ["SAFE", "WARNING", "DANGER"]) {
        const msg = telemetryMsg(`{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"${status}"}`);
        assert.equal(invokeFunction(nodes.validate1, msg), msg);
    }
    const spaced = telemetryMsg(' { "deviceId" : "esp32-01" , "ax" : 0.02 , "ay" : -0.01 , "az" : 1 , "gForce" : 1 , "status" : "SAFE" } ');
    assert.equal(invokeFunction(nodes.validate1, spaced), spaced);
    const escaped = telemetryMsg('{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"\\u0073tatus":"SAFE"}');
    assert.equal(invokeFunction(nodes.validate1, escaped), escaped);
});

test("rejects malformed telemetry, mismatched identity, retained input, and non-string payloads", () => {
    assert.equal(invokeFunction(nodes.validate1, { ...telemetryMsg("{}"), retain: true }), null);
    for (const payload of [
        '{"deviceId":"esp32-01","ax":"0.02","ay":-0.01,"az":1,"gForce":1,"status":"SAFE"}',
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1e400,"status":"SAFE"}',
        '{"deviceId":"other","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"SAFE"}',
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"SAFE","extra":1}',
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"danger"}',
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"SAFE"',
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"SAFE"} {}',
        'not json',
    ]) assert.equal(invokeFunction(nodes.validate1, telemetryMsg(payload)), null);
    assert.equal(invokeFunction(nodes.validate1, telemetryMsg(telemetry())), null);
});

test("rejects malformed alerts and mismatched fields", () => {
    for (const payload of [
        '{"deviceId":"esp32-01","status":"SAFE","gForce":2}',
        '{"deviceId":"other","status":"DANGER","gForce":2}',
        '{"deviceId":"esp32-01","status":"DANGER","gForce":"2"}',
        '{"deviceId":"esp32-01","status":"DANGER","gForce":2,"extra":true}',
        '{"deviceId":"esp32-01","status":"DANGER"',
        '[]',
    ]) assert.equal(invokeFunction(nodes.validate1, alertMsg(payload)), null);
});

test("rejects duplicate member names, including escaped aliases, before cooldown", () => {
    for (const payload of [
        '{"deviceId":"esp32-01","status":"SAFE","status":"DANGER","gForce":2}',
        '{"deviceId":"esp32-01","status":"DANGER","gForce":2,"gForce":1}',
        '{"deviceId":"esp32-01","status":"SAFE","\\u0073tatus":"DANGER","gForce":2}',
        '{"deviceId":"esp32-01","status":"DANGER","gForce":2,"\\u0067Force":1}',
    ]) assert.equal(invokeFunction(nodes.validate1, alertMsg(payload)), null);

    for (const payload of [
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"SAFE","status":"DANGER"}',
        '{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1,"status":"SAFE","\\u0073tatus":"DANGER"}',
    ]) assert.equal(invokeFunction(nodes.validate1, telemetryMsg(payload)), null);
});

test("DANGER alerts pass cooldown once; invalid alerts do not consume it", () => {
    const now = Date.now;
    let clock = 5000;
    Date.now = () => clock;
    try {
        const fresh = new Map();
        assert.equal(invokeFunction(nodes.validate1, alertMsg('{"deviceId":"esp32-01","status":"SAFE","status":"DANGER","gForce":2}'), fresh), null);
        assert.equal(invokeFunction(nodes.validate1, alertMsg('{"deviceId":"esp32-01","status":"DANGER"'), fresh), null);
        const valid = alertMsg('{"deviceId":"esp32-01","status":"DANGER","gForce":2}');
        assert.equal(invokeFunction(nodes.validate1, valid, fresh).payload.status, "DANGER");

        const state = new Map();
        assert.equal(invokeFunction(nodes.validate1, alertMsg('{"deviceId":"esp32-01","status":"DANGER","gForce":2}'), state).payload.gForce, 2);
        clock = 5500;
        assert.equal(invokeFunction(nodes.validate1, alertMsg('{"deviceId":"esp32-01","status":"SAFE","\\u0073tatus":"DANGER","gForce":2}'), state), null);
        clock = 5999;
        assert.equal(invokeFunction(nodes.validate1, alertMsg('{"deviceId":"esp32-01","status":"DANGER","gForce":2}'), state), null);
        clock = 6000;
        assert.equal(invokeFunction(nodes.validate1, alertMsg('{"deviceId":"esp32-01","status":"DANGER","gForce":2}'), state).payload.gForce, 2);
    } finally {
        Date.now = now;
    }
    assert.equal(invokeFunction(nodes.eb9411b45aa7183e, alertObject, new Map(), {}), null);
    const formatted = invokeFunction(nodes.eb9411b45aa7183e, alertObject, new Map(), { TELEGRAM_CHAT_ID: "123" });
    assert.equal(formatted.payload.chatId, "123");
    assert.match(formatted.payload.content, /DANGER/);
});

test("raw MQTT messages keep parsed values through feed and Telegram function chains", () => {
    const env = { ADAFRUIT_USERNAME: "adafruit-user", TELEGRAM_CHAT_ID: "123" };
    const state = new Map();
    const rawTelemetry = telemetryMsg('{"deviceId":"esp32-01","ax":0.02,"ay":-0.01,"az":1,"gForce":1.25,"status":"WARNING"}');
    const parsedTelemetry = invokeFunction(nodes.validate1, rawTelemetry, state);
    assert.equal(parsedTelemetry, rawTelemetry);
    assert.equal(JSON.stringify(parsedTelemetry.payload), JSON.stringify({ ...telemetry("WARNING"), gForce: 1.25 }));
    const gforceFeed = invokeFunction(nodes.chg1, { ...parsedTelemetry }, state, env);
    assert.equal(gforceFeed.topic, "adafruit-user/feeds/gforce");
    assert.equal(gforceFeed.payload, 1.25);
    const statusFeed = invokeFunction(nodes.chg2, { ...parsedTelemetry }, state, env);
    assert.equal(statusFeed.topic, "adafruit-user/feeds/status");
    assert.equal(statusFeed.payload, "WARNING");

    const now = Date.now;
    Date.now = () => 7000;
    try {
        const alert = () => alertMsg('{"deviceId":"esp32-01","status":"DANGER","gForce":1.97}');
        const accepted = invokeFunction(nodes.validate1, alert(), state);
        assert.equal(accepted.payload.status, "DANGER");
        assert.equal(accepted.payload.gForce, 1.97);
        const formatted = invokeFunction(nodes.eb9411b45aa7183e, accepted, state, env);
        assert.equal(formatted.payload.chatId, "123");
        assert.equal(formatted.payload.content, "CẢNH BÁO VA ĐẬP (DANGER)\nThiết bị: esp32-01\nG-force: 1.97 g");
        assert.equal(invokeFunction(nodes.validate1, alert(), state), null);
    } finally {
        Date.now = now;
    }
});

test("Adafruit feeds are built from ADAFRUIT_USERNAME; unset drops publication", () => {
    const sample = telemetry("WARNING");
    assert.equal(invokeFunction(nodes.chg1, { payload: sample }, new Map(), {}), null);
    assert.equal(invokeFunction(nodes.chg2, { payload: sample }, new Map(), {}), null);

    const env = { ADAFRUIT_USERNAME: "adafruit-user" };
    const gforce = invokeFunction(nodes.chg1, { payload: sample }, new Map(), env);
    assert.equal(gforce.topic, "adafruit-user/feeds/gforce");
    assert.equal(gforce.payload, 1);
    const status = invokeFunction(nodes.chg2, { payload: sample }, new Map(), env);
    assert.equal(status.topic, "adafruit-user/feeds/status");
    assert.equal(status.payload, "WARNING");

    assert.equal(JSON.stringify(flow).includes("ADAFRUIT_USERNAME/feeds"), false);
    assert.equal(nodes.rate1.pauseType, "rate");
    assert.equal(nodes.rate1.rate, "1");
    assert.equal(nodes.rate1.nbRateUnits, "3");
    assert.equal(nodes.rate1.drop, true);
    assert.equal(nodes.rbe1.func, "rbe");
});

test("Adafruit broker uses TLS 8883 with certificate verification and no inline secrets", () => {
    assert.equal(nodes.broker2.broker, "io.adafruit.com");
    assert.equal(nodes.broker2.port, "8883");
    assert.equal(nodes.broker2.usetls, true);
    const tls = nodes[nodes.broker2.tls];
    assert.equal(tls.type, "tls-config");
    assert.equal(tls.verifyservercert, true);
    assert.equal(tls.ca, "");
    assert.equal(nodes.broker2.username, undefined);
    assert.equal(nodes.broker2.password, undefined);
    for (const id of ["mqttout1", "mqttout2"]) assert.equal(nodes[id].broker, "broker2");
});
