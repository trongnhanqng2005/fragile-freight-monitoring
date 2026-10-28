package vn.edu.huit.fragilefreight.mqtt;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

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
}
