package vn.edu.huit.fragilefreight.mqtt;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;
import vn.edu.huit.fragilefreight.model.SensorState;

class MqttMessageParserTest {

    @Test
    void parseTelemetry_shouldReadValidFields() {
        String payload = "{\"deviceId\":\"esp32-01\",\"ax\":0.02,\"ay\":-0.01,\"az\":1.00,\"gForce\":1.00,\"status\":\"SAFE\"}";

        SensorState sensorState = MqttMessageParser.parseTelemetry(payload);

        assertEquals("esp32-01", sensorState.getDeviceId());
        assertEquals(0.02, sensorState.getAx());
        assertEquals(-0.01, sensorState.getAy());
        assertEquals(1.00, sensorState.getAz());
        assertEquals(1.00, sensorState.getGForce());
        assertEquals("SAFE", sensorState.getStatus());
    }

    @Test
    void parseAlert_shouldRejectNonDanger() {
        String payload = "{\"deviceId\":\"esp32-01\",\"status\":\"SAFE\",\"gForce\":1.00}";

        IllegalArgumentException exception = assertThrows(IllegalArgumentException.class,
            () -> MqttMessageParser.parseAlert(payload));

        assertEquals("Alert payload must report a DANGER state", exception.getMessage());
    }

    @Test
    void parseTelemetry_rejectsCoercionExtraFieldsAndWrongIdentity() {
        assertThrows(IllegalArgumentException.class, () -> MqttMessageParser.parseTelemetry(
            "{\"deviceId\":\"esp32-01\",\"ax\":\"0.02\",\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"}"));
        assertThrows(IllegalArgumentException.class, () -> MqttMessageParser.parseTelemetry(
            "{\"deviceId\":\"esp32-01\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\",\"extra\":1}"));
        assertThrows(IllegalArgumentException.class, () -> MqttMessageParser.parseTelemetry(
            "{\"deviceId\":\"other\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"}"));
    }

    @Test
    void parseTelemetry_rejectsOverflowAndNonExactStatus() {
        assertThrows(IllegalArgumentException.class, () -> MqttMessageParser.parseTelemetry(
            "{\"deviceId\":\"esp32-01\",\"ax\":1e400,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"}"));
        IllegalArgumentException error = assertThrows(IllegalArgumentException.class,
            () -> MqttMessageParser.parseTelemetry(
                "{\"deviceId\":\"esp32-01\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"safe\"}"));
        assertTrue(error.getMessage().contains("Status"));
    }

    @Test
    void parseTelemetry_rejectsTrailingJson() {
        assertThrows(IllegalArgumentException.class, () -> MqttMessageParser.parseTelemetry(
            "{\"deviceId\":\"esp32-01\",\"ax\":0,\"ay\":0,\"az\":1,\"gForce\":1,\"status\":\"SAFE\"} {}"));
    }
}
