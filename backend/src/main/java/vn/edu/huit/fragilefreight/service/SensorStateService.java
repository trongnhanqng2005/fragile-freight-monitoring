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
    private final Map<String, SensorState> latestAlerts = new ConcurrentHashMap<>();

    public void updateTelemetry(SensorState sensorState) {
        if (sensorState == null || sensorState.getDeviceId() == null || sensorState.getDeviceId().isBlank()) {
            throw new IllegalArgumentException("Device ID is required");
        }
        SensorState snapshot = new SensorState(sensorState.getDeviceId(), sensorState.getAx(),
            sensorState.getAy(), sensorState.getAz(), sensorState.getGForce(), sensorState.getStatus());
        snapshot.setUpdatedAt(Instant.now());
        latestStates.put(snapshot.getDeviceId(), snapshot);
    }

    public void updateAlert(String deviceId, double gForce) {
        if (deviceId == null || deviceId.isBlank()) {
            throw new IllegalArgumentException("Device ID is required");
        }

        if (!Double.isFinite(gForce)) {
            throw new IllegalArgumentException("G-force must be finite");
        }
        SensorState alert = new SensorState();
        alert.setDeviceId(deviceId);
        alert.setStatus("DANGER");
        alert.setGForce(gForce);
        alert.setUpdatedAt(Instant.now());
        latestAlerts.put(deviceId, alert);
    }

    public Optional<SensorState> getLatestState(String deviceId) {
        return Optional.ofNullable(latestStates.get(deviceId)).map(SensorStateService::copy);
    }

    public Optional<SensorState> getLatestState() {
        return latestStates.values().stream().findFirst().map(SensorStateService::copy);
    }

    public Optional<SensorState> getLatestAlert(String deviceId) {
        return Optional.ofNullable(latestAlerts.get(deviceId)).map(SensorStateService::copy);
    }

    private static SensorState copy(SensorState source) {
        SensorState copy = new SensorState(source.getDeviceId(), source.getAx(), source.getAy(),
            source.getAz(), source.getGForce(), source.getStatus());
        copy.setUpdatedAt(source.getUpdatedAt());
        return copy;
    }

    public void clear() {
        latestStates.clear();
        latestAlerts.clear();
    }
}
