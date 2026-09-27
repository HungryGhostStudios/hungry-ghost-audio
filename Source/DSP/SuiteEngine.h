#pragma once
#include "../Catalogue.h"
#include <array>
#include <cstdint>
#include <vector>

namespace hungryghost {
class SuiteEngine {
public:
  explicit SuiteEngine(Kind);
  void prepare(double sampleRate);
  void reset();
  void setControls(const std::array<float, 6> &, float mix, float outputDb);
  void process(float *left, float *right, const float *keyLeft,
               const float *keyRight, int samples) noexcept;
  float gainReduction() const noexcept { return reduction; }

private:
  struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float tick(float) noexcept;
    void clear() noexcept;
  };
  Kind kind;
  double sr = 48000, phase = 0;
  std::array<float, 6> target{}, smooth{};
  float targetMix = 1, mix = 1, targetOutput = 1, output = 1, smoothing = .002f;
  float envelope = 0, fastEnvelope = 0, slowEnvelope = 0, gainState = 1,
        reduction = 0, heldL = 0, heldR = 0, holdClock = 0;
  float feedbackL = 0, feedbackR = 0, toneL = 0, toneR = 0, dcInL = 0,
        dcInR = 0, dcOutL = 0, dcOutR = 0;
  float sideLow = 0, keyLowL = 0, keyLowR = 0;
  int holdSamples = 0;
  bool gateOpen = false;
  std::array<std::array<Biquad, 6>, 2> filters{};
  std::array<std::array<float, 6>, 2> allpassIn{}, allpassOut{};
  std::array<std::vector<float>, 2> delay;
  std::size_t writeIndex = 0;
  unsigned counter = 0;
  std::uint32_t noise = 0x137ac41u;
  void updateFilters() noexcept;
  float readDelay(int channel, float timeMs) const noexcept;
  float random() noexcept;
  bool usesDelay() const noexcept;
  static float db(float) noexcept;
  static float toDb(float) noexcept;
  void configure(Biquad &, int type, float frequency, float q,
                 float gain = 0) noexcept;
};
} // namespace hungryghost
