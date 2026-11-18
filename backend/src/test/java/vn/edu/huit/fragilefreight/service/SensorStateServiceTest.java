package vn.edu.huit.fragilefreight.service;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import org.junit.jupiter.api.Test;
import vn.edu.huit.fragilefreight.model.SensorState;

class SensorStateServiceTest {
    @Test
    void alertDoesNotMutateOrMixTelemetryAndReturnedStateIsSnapshot() {
        SensorStateService service = new SensorStateService();
        SensorState input = new SensorState("esp32-01", 1, 2, 3, 4, "SAFE");
        service.updateTelemetry(input);
        service.updateAlert("esp32-01", 9);

        SensorState telemetry = service.getLatestState("esp32-01").orElseThrow();
        assertEquals(1, telemetry.getAx());
        assertEquals(4, telemetry.getGForce());
        assertEquals("SAFE", telemetry.getStatus());
        assertEquals(9, service.getLatestAlert("esp32-01").orElseThrow().getGForce());
        telemetry.setAx(99);
        assertNotEquals(99, service.getLatestState("esp32-01").orElseThrow().getAx());
    }
}
