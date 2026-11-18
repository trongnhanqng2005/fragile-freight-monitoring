package vn.edu.huit.fragilefreight.mqtt;

import tools.jackson.core.StreamReadFeature;
import tools.jackson.databind.DeserializationFeature;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.json.JsonMapper;
import java.util.Set;
import vn.edu.huit.fragilefreight.model.SensorState;

public final class MqttMessageParser {

    private static final JsonMapper OBJECT_MAPPER = JsonMapper.builder()
        .enable(StreamReadFeature.STRICT_DUPLICATE_DETECTION)
        .enable(DeserializationFeature.FAIL_ON_TRAILING_TOKENS)
        .build();
    private static final Set<String> TELEMETRY_FIELDS =
        Set.of("deviceId", "ax", "ay", "az", "gForce", "status");
    private static final Set<String> ALERT_FIELDS = Set.of("deviceId", "status", "gForce");

    private MqttMessageParser() {
    }

    public static SensorState parseTelemetry(String payload) {
        try {
            JsonNode root = OBJECT_MAPPER.readTree(payload);
            validateObject(root, TELEMETRY_FIELDS);
            String deviceId = readRequiredText(root, "deviceId");
            validateDeviceId(deviceId);
            double ax = readFiniteNumber(root, "ax");
            double ay = readFiniteNumber(root, "ay");
            double az = readFiniteNumber(root, "az");
            double gForce = readFiniteNumber(root, "gForce");
            String status = readRequiredText(root, "status");
            validateStatus(status);

            return new SensorState(deviceId, ax, ay, az, gForce, status);
        } catch (tools.jackson.core.JacksonException ex) {
            throw new IllegalArgumentException("Invalid telemetry JSON payload", ex);
        }
    }

    public static SensorState parseAlert(String payload) {
        try {
            JsonNode root = OBJECT_MAPPER.readTree(payload);
            validateObject(root, ALERT_FIELDS);
            String deviceId = readRequiredText(root, "deviceId");
            validateDeviceId(deviceId);
            double gForce = readFiniteNumber(root, "gForce");
            String status = readRequiredText(root, "status");
            validateStatus(status);
            if (!"DANGER".equals(status)) {
                throw new IllegalArgumentException("Alert payload must report a DANGER state");
            }

            SensorState sensorState = new SensorState();
            sensorState.setDeviceId(deviceId);
            sensorState.setStatus("DANGER");
            sensorState.setGForce(gForce);
            sensorState.setUpdatedAt(java.time.Instant.now());
            return sensorState;
        } catch (tools.jackson.core.JacksonException ex) {
            throw new IllegalArgumentException("Invalid alert JSON payload", ex);
        }
    }

    private static String readRequiredText(JsonNode root, String fieldName) {
        JsonNode field = root.get(fieldName);
        String value = field != null && field.isString() ? field.stringValue() : null;
        if (value == null || value.isBlank()) {
            throw new IllegalArgumentException("Missing required field: " + fieldName);
        }
        return value;
    }

    private static void validateStatus(String status) {
        if (!Set.of("SAFE", "WARNING", "DANGER").contains(status)) {
            throw new IllegalArgumentException("Status must be one of SAFE, WARNING, DANGER");
        }
    }

    private static void validateDeviceId(String deviceId) {
        if (!"esp32-01".equals(deviceId)) {
            throw new IllegalArgumentException("Device ID must be esp32-01");
        }
    }

    private static double readFiniteNumber(JsonNode root, String fieldName) {
        JsonNode field = root.get(fieldName);
        if (field == null || !field.isNumber() || !Double.isFinite(field.doubleValue())) {
            throw new IllegalArgumentException(fieldName + " must be a finite JSON number");
        }
        return field.doubleValue();
    }

    private static void validateObject(JsonNode root, Set<String> expectedFields) {
        if (root == null || !root.isObject() || root.size() != expectedFields.size()) {
            throw new IllegalArgumentException("Payload must contain exactly the contract fields");
        }
        root.propertyNames().forEach(field -> {
            if (!expectedFields.contains(field)) {
                throw new IllegalArgumentException("Unexpected field: " + field);
            }
        });
    }

}
