#pragma once

#include <cstdint>

namespace fragilefreight {

enum class ImpactState : uint8_t { SAFE, WARNING, DANGER };

struct ImpactSnapshot {
  float axG;
  float ayG;
  float azG;
  float magnitudeG;
  float deviationG;
  ImpactState state;
  bool valid;
  bool dangerEvent;
};

ImpactState classifyImpact(float deviationG);
const char* stateName(ImpactState state);

class ImpactLogic {
 public:
  ImpactSnapshot update(float axG, float ayG, float azG, uint32_t nowMs);
  ImpactState state() const { return state_; }
  bool alertArmed() const { return alertArmed_; }
  void reset();

 private:
  ImpactState state_ = ImpactState::SAFE;
  uint32_t lastDangerEventMs_ = 0;
  bool hasDangerEvent_ = false;
  bool alertArmed_ = true;
};

}  // namespace fragilefreight
