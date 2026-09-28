#pragma once
#include "Catalogue.h"
#include <algorithm>
#include <cmath>
namespace hungryghost {
inline bool hasTempoSync(Kind k){return k>=Kind::Delay&&k<=Kind::DubDelay||k>=Kind::Chorus&&k<=Kind::AutoPan||k==Kind::Vibrato;}
inline bool hasDetector(Kind k){return k>=Kind::Compressor&&k<=Kind::Expander&&k!=Kind::Transient||k==Kind::Ducker;}
inline bool hasColourFilter(Kind k){return k>=Kind::SoftSaturation&&k<=Kind::RateReducer;}
inline bool hasRepeatFilter(Kind k){return k>=Kind::Delay&&k<=Kind::Comb;}
inline float syncedControl(Kind kind,double bpm,int division){
  // Quarter-note beats, including triplets and dotted notes.
  constexpr double beats[]{4,2,1,.5,.25,.125,1.5,.75,.375,2./3,1./3,1./6};
  bpm=std::isfinite(bpm)?std::clamp(bpm,20.,400.):120.;
  const double seconds=60./bpm*beats[std::clamp(division,0,11)];
  return static_cast<float>(kind>=Kind::Delay&&kind<=Kind::DubDelay?seconds*1000:1./seconds);
}
struct AdvancedSettings {
  std::array<float,3> bandQ{.707f,.707f,.707f};
  float detectorCut=0,inputCut=0,repeatCut=0;
  bool listenKey=false;
  bool monoListen=false;
  float syncedValue=0;
};
} // namespace hungryghost
