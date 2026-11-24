#include "MqttPayload.h"

#include <cmath>
#include <cstdio>

namespace fragilefreight {
namespace {
bool validSample(const ImpactSnapshot& sample) {
  const bool validState = sample.state == ImpactState::SAFE ||
                          sample.state == ImpactState::WARNING ||
                          sample.state == ImpactState::DANGER;
  return sample.valid && validState && std::isfinite(sample.axG) &&
         std::isfinite(sample.ayG) && std::isfinite(sample.azG) &&
         std::isfinite(sample.magnitudeG) && sample.magnitudeG >= 0.0f;
}

bool finishFormat(int written, char* output, size_t capacity, size_t& length) {
  if (written < 0 || static_cast<size_t>(written) >= capacity) {
    output[0] = '\0';
    return false;
  }
  length = static_cast<size_t>(written);
  return true;
}
}

const char DEVICE_ID[] = "esp32-01";
const char TELEMETRY_TOPIC[] = "fragile-freight/esp32-01/telemetry";
const char ALERT_TOPIC[] = "fragile-freight/esp32-01/alert";
const char COMMAND_TOPIC[] = "fragile-freight/esp32-01/command";

bool formatTelemetry(const ImpactSnapshot& sample, char* output, size_t capacity,
                     size_t& length) {
  length = 0;
  if (!output || capacity == 0 || !validSample(sample)) return false;

  const int written = std::snprintf(
      output, capacity,
      "{\"deviceId\":\"%s\",\"ax\":%.9g,\"ay\":%.9g,\"az\":%.9g,\"gForce\":%.9g,\"status\":\"%s\"}",
      DEVICE_ID, static_cast<double>(sample.axG), static_cast<double>(sample.ayG),
      static_cast<double>(sample.azG), static_cast<double>(sample.magnitudeG),
      stateName(sample.state));
  return finishFormat(written, output, capacity, length);
}

bool formatAlert(const ImpactSnapshot& sample, char* output, size_t capacity, size_t& length) {
  length = 0;
  if (!output || capacity == 0 || !validSample(sample) ||
      sample.state != ImpactState::DANGER) {
    return false;
  }

  const int written = std::snprintf(
      output, capacity, "{\"deviceId\":\"%s\",\"status\":\"DANGER\",\"gForce\":%.9g}",
      DEVICE_ID, static_cast<double>(sample.magnitudeG));
  return finishFormat(written, output, capacity, length);
}

}  // namespace fragilefreight
