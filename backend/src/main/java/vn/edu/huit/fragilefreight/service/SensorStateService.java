package vn.edu.huit.fragilefreight.service;

import java.time.Instant;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import org.springframework.stereotype.Service;
import vn.edu.huit.fragilefreight.model.SensorState;

@Service
public class SensorStateService {

    private final Map<String, SensorState> latestStates = new ConcurrentHashMap<>();

    public void updateTelemetry(SensorState sensorState) {
        if (sensorState == null || sensorState.getDeviceId() == null || sensorState.getDeviceId().isBlank()) {
            throw new IllegalArgumentException("Device ID is required");
        }
        sensorState.setUpdatedAt(Instant.now());
        latestStates.put(sensorState.getDeviceId(), sensorState);
    }

    public void updateAlert(String deviceId, double gForce) {
        if (deviceId == null || deviceId.isBlank()) {
            throw new IllegalArgumentException("Device ID is required");
        }

        SensorState previousState = latestStates.getOrDefault(deviceId, new SensorState());
        previousState.setDeviceId(deviceId);
        previousState.setStatus("DANGER");
        previousState.setGForce(gForce);
        previousState.setUpdatedAt(Instant.now());
        latestStates.put(deviceId, previousState);
    }

    public Optional<SensorState> getLatestState(String deviceId) {
        return Optional.ofNullable(latestStates.get(deviceId));
    }

    public Optional<SensorState> getLatestState() {
        return latestStates.values().stream().findFirst();
    }

    public void clear() {
        latestStates.clear();
    }
}
