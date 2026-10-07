# Impact Monitoring Requirements

## Purpose

Monitor acceleration of fragile freight using an MPU6050 connected to an ESP32. Firmware derives a three-axis acceleration magnitude and uses its deviation from 1g to classify local impact status.

## Impact metric

Given acceleration components in g:

```text
magnitudeG = sqrt(axG * axG + ayG * ayG + azG * azG)
deviationG = abs(magnitudeG - 1.0)
```

Magnitude must use all three axes. MPU6050 acceleration readings include gravity; the deviation metric makes a stationary ~1g reading the SAFE baseline.

## Demo classification thresholds

- **SAFE:** `deviationG < 0.4g`
- **WARNING:** `0.4g <= deviationG < 0.9g`
- **DANGER:** `deviationG >= 0.9g`

These values are initial demo/calibration thresholds only. They are not universal physical cargo-damage or safety limits and require calibration for any real application.

## Current implementation boundary

Classification and local outputs run on the ESP32. Network reporting and external notification paths are planned separately and are not part of this local requirement.
