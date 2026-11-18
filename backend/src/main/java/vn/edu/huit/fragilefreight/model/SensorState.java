package vn.edu.huit.fragilefreight.model;

import java.time.Instant;
import java.util.Objects;

public class SensorState {

    private String deviceId;
    private double ax;
    private double ay;
    private double az;
    private double gForce;
    private String status;
    private Instant updatedAt;

    public SensorState() {
        this.updatedAt = Instant.now();
    }

    public SensorState(String deviceId, double ax, double ay, double az, double gForce, String status) {
        this.deviceId = deviceId;
        this.ax = ax;
        this.ay = ay;
        this.az = az;
        this.gForce = gForce;
        this.status = status;
        this.updatedAt = Instant.now();
    }

    public String getDeviceId() {
        return deviceId;
    }

    public void setDeviceId(String deviceId) {
        this.deviceId = deviceId;
    }

    public double getAx() {
        return ax;
    }

    public void setAx(double ax) {
        this.ax = ax;
    }

    public double getAy() {
        return ay;
    }

    public void setAy(double ay) {
        this.ay = ay;
    }

    public double getAz() {
        return az;
    }

    public void setAz(double az) {
        this.az = az;
    }

    public double getGForce() {
        return gForce;
    }

    public void setGForce(double gForce) {
        this.gForce = gForce;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public Instant getUpdatedAt() {
        return updatedAt;
    }

    public void setUpdatedAt(Instant updatedAt) {
        this.updatedAt = updatedAt;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        SensorState that = (SensorState) o;
        return Double.compare(ax, that.ax) == 0
            && Double.compare(ay, that.ay) == 0
            && Double.compare(az, that.az) == 0
            && Double.compare(gForce, that.gForce) == 0
            && Objects.equals(deviceId, that.deviceId)
            && Objects.equals(status, that.status)
            && Objects.equals(updatedAt, that.updatedAt);
    }

    @Override
    public int hashCode() {
        return Objects.hash(deviceId, ax, ay, az, gForce, status, updatedAt);
    }
}
