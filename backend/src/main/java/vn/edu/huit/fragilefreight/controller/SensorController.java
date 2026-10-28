package vn.edu.huit.fragilefreight.controller;

import java.util.Map;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import vn.edu.huit.fragilefreight.model.SensorState;
import vn.edu.huit.fragilefreight.service.SensorStateService;

@RestController
@RequestMapping("/api")
public class SensorController {

    private final SensorStateService sensorStateService;

    public SensorController(SensorStateService sensorStateService) {
        this.sensorStateService = sensorStateService;
    }

    @GetMapping("/sensor/latest")
    public ResponseEntity<Object> getLatestSensorState() {
        return sensorStateService.getLatestState()
            .map(sensorState -> ResponseEntity.ok((Object) sensorState))
            .orElseGet(() -> ResponseEntity.status(HttpStatus.NOT_FOUND)
                .body(Map.of("message", "No sensor data received yet")));
    }

    @GetMapping("/sensor/{deviceId}")
    public ResponseEntity<Object> getLatestSensorState(@PathVariable String deviceId) {
        return sensorStateService.getLatestState(deviceId)
            .map(sensorState -> ResponseEntity.ok((Object) sensorState))
            .orElseGet(() -> ResponseEntity.status(HttpStatus.NOT_FOUND)
                .body(Map.of("deviceId", deviceId, "message", "No data available for this device")));
    }
}
