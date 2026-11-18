#pragma once

#include <cstddef>

#include <ImpactLogic.h>

namespace fragilefreight {

extern const char DEVICE_ID[];
extern const char TELEMETRY_TOPIC[];
extern const char ALERT_TOPIC[];

bool formatTelemetry(const ImpactSnapshot& sample, char* output, size_t capacity,
                     size_t& length);
bool formatAlert(const ImpactSnapshot& sample, char* output, size_t capacity, size_t& length);

}  // namespace fragilefreight
