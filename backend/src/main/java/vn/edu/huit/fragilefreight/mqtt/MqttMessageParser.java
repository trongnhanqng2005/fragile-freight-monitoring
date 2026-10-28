package vn.edu.huit.fragilefreight.mqtt;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.IOException;
import vn.edu.huit.fragilefreight.model.SensorState;

public final class MqttMessageParser {

    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

    private MqttMessageParser() {
    }

    public static SensorState parseTelemetry(String payload) {
        try {
            JsonNode root = OBJECT_MAPPER.readTree(payload);
            String deviceId = readRequiredText(root, "deviceId");
            double ax = root.path("ax").asDouble();
            double ay = root.path("ay").asDouble();
            double az = root.path("az").asDouble();
            double gForce = root.path("gForce").asDouble();
            String status = readRequiredText(root, "status");
            validateStatus(status);

            return new SensorState(deviceId, ax, ay, az, gForce, status);
        } catch (IOException ex) {
            throw new IllegalArgumentException("Invalid telemetry JSON payload", ex);
        }
    }

    public static SensorState parseAlert(String payload) {
        try {
            JsonNode root = OBJECT_MAPPER.readTree(payload);
            String deviceId = readRequiredText(root, "deviceId");
            double gForce = root.path("gForce").asDouble();
            String status = readRequiredText(root, "status");
            validateStatus(status);
            if (!"DANGER".equalsIgnoreCase(status)) {
                throw new IllegalArgumentException("Alert payload must report a DANGER state");
            }

            SensorState sensorState = new SensorState();
            sensorState.setDeviceId(deviceId);
            sensorState.setStatus("DANGER");
            sensorState.setGForce(gForce);
            sensorState.setUpdatedAt(java.time.Instant.now());
            return sensorState;
        } catch (IOException ex) {
            throw new IllegalArgumentException("Invalid alert JSON payload", ex);
        }
    }

    private static String readRequiredText(JsonNode root, String fieldName) {
        String value = root.path(fieldName).asText(null);
        if (value == null || value.isBlank()) {
            throw new IllegalArgumentException("Missing required field: " + fieldName);
        }
        return value;
    }

    private static void validateStatus(String status) {
        if (status == null || (!"SAFE".equalsIgnoreCase(status)
            && !"WARNING".equalsIgnoreCase(status)
            && !"DANGER".equalsIgnoreCase(status))) {
            throw new IllegalArgumentException("Status must be one of SAFE, WARNING, DANGER");
        }
    }
}
