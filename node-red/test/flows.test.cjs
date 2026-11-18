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

test("graph validates parsed MQTT before topic split; Telegram has no telemetry bypass", () => {
    assert.deepEqual(nodes.json1.wires, [["validate1"]]);
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

test("accepts all telemetry states with exact contract fields", () => {
    for (const status of ["SAFE", "WARNING", "DANGER"]) {
        const msg = { topic: "fragile-freight/esp32-01/telemetry", payload: telemetry(status) };
        assert.equal(invokeFunction(nodes.validate1, msg), msg);
    }
});

test("rejects malformed telemetry, mismatched identity, retained input, and malformed alerts", () => {
    const topic = "fragile-freight/esp32-01/telemetry";
    assert.equal(invokeFunction(nodes.validate1, { topic, payload: telemetry(), retain: true }), null);
    for (const payload of [
        { ...telemetry(), ax: "0.02" },
        { ...telemetry(), gForce: Infinity },
        { ...telemetry(), deviceId: "other" },
        { ...telemetry(), extra: 1 },
        { ...telemetry(), status: "danger" },
    ]) assert.equal(invokeFunction(nodes.validate1, { topic, payload }), null);

    const alertTopic = "fragile-freight/esp32-01/alert";
    for (const payload of [
        { deviceId: "esp32-01", status: "SAFE", gForce: 1 },
        { deviceId: "other", status: "DANGER", gForce: 1 },
        { deviceId: "esp32-01", status: "DANGER", gForce: "1" },
        { deviceId: "esp32-01", status: "DANGER", gForce: 1, extra: true },
    ]) assert.equal(invokeFunction(nodes.validate1, { topic: alertTopic, payload }), null);
});

test("DANGER alerts pass cooldown once; only valid alerts format for configured sender", () => {
    const state = new Map();
    const alert = { topic: "fragile-freight/esp32-01/alert", payload: { deviceId: "esp32-01", status: "DANGER", gForce: 2 } };
    const now = Date.now;
    let clock = 5000;
    Date.now = () => clock;
    try {
        assert.equal(invokeFunction(nodes.validate1, alert, state), alert);
        assert.equal(invokeFunction(nodes.validate1, alert, state), null);
        clock += 1000;
        assert.equal(invokeFunction(nodes.validate1, alert, state), alert);
    } finally {
        Date.now = now;
    }
    assert.equal(invokeFunction(nodes.eb9411b45aa7183e, alert, new Map(), {}), null);
    const formatted = invokeFunction(nodes.eb9411b45aa7183e, alert, new Map(), { TELEGRAM_CHAT_ID: "123" });
    assert.equal(formatted.payload.chatId, "123");
    assert.match(formatted.payload.content, /DANGER/);
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
