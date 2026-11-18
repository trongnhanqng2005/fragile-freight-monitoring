#include "ImpactLogic.h"

#include <cmath>

namespace fragilefreight {
namespace {
constexpr float WARNING_DEVIATION_G = 0.4f;
constexpr float DANGER_DEVIATION_G = 0.9f;
constexpr uint32_t ALERT_COOLDOWN_MS = 1000;
}

ImpactState classifyImpact(float deviationG) {
  if (deviationG >= DANGER_DEVIATION_G) return ImpactState::DANGER;
  if (deviationG >= WARNING_DEVIATION_G) return ImpactState::WARNING;
  return ImpactState::SAFE;
}

const char* stateName(ImpactState state) {
  switch (state) {
    case ImpactState::WARNING:
      return "WARNING";
    case ImpactState::DANGER:
      return "DANGER";
    default:
      return "SAFE";
  }
}

ImpactSnapshot ImpactLogic::update(float axG, float ayG, float azG, uint32_t nowMs) {
  const float magnitudeG = std::sqrt(axG * axG + ayG * ayG + azG * azG);
  const float deviationG = std::fabs(magnitudeG - 1.0f);
  const bool valid = std::isfinite(axG) && std::isfinite(ayG) && std::isfinite(azG) &&
                     std::isfinite(magnitudeG);

  state_ = classifyImpact(deviationG);
  bool dangerEvent = false;
  if (state_ == ImpactState::DANGER && alertArmed_) {
    lastDangerEventMs_ = nowMs;
    hasDangerEvent_ = true;
    alertArmed_ = false;
    dangerEvent = true;
  }

  if (state_ == ImpactState::SAFE && hasDangerEvent_ &&
      static_cast<uint32_t>(nowMs - lastDangerEventMs_) >= ALERT_COOLDOWN_MS) {
    alertArmed_ = true;
  }

  return {axG, ayG, azG, magnitudeG, deviationG, state_, valid, dangerEvent};
}

void ImpactLogic::reset() {
  state_ = ImpactState::SAFE;
  lastDangerEventMs_ = 0;
  hasDangerEvent_ = false;
  alertArmed_ = true;
}

}  // namespace fragilefreight
