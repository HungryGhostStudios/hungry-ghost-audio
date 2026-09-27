#pragma once
#include <array>
#include <cmath>
#include <complex>

namespace feral {
constexpr int maxBands = 8;
enum Shape { bell, lowShelf, highShelf, lowCut, highCut };
enum Channel { stereo, mid, side, left, right };
struct Band {
  float enabled = 0, frequency = 1000, gain = 0, q = 1;
  float threshold = -24, ratio = 3, attack = 10, release = 150, knee = 6,
        range = 6;
  float dynamic = 1, shape = 0, channel = 0, external = 0;
};
struct Parameters {
  std::array<Band, maxBands> bands;
  float busEnabled = 0, threshold = -18, ratio = 2, attack = 30, release = 150,
        knee = 6, range = 18;
  float input = 0, output = 0, mix = 100, external = 0, detectorHP = 20,
        bypass = 0;
};
struct Coefficients {
  double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
  double magnitude(double frequency, double sampleRate) const noexcept;
};
Coefficients filter(int shape, double frequency, double q, double gain,
                    double sampleRate) noexcept;
double reduction(double level, double threshold, double ratio, double knee,
                 double range) noexcept;
struct FilterState {
  double z1 = 0, z2 = 0;
  double tick(double in, const Coefficients &c) noexcept {
    const double out = c.b0 * in + z1;
    z1 = c.b1 * in - c.a1 * out + z2;
    z2 = c.b2 * in - c.a2 * out;
    if (std::abs(z1) < 1e-25)
      z1 = 0;
    if (std::abs(z2) < 1e-25)
      z2 = 0;
    return out;
  }
};
class DynamicsEngine {
public:
  void prepare(double rate, const Parameters &p);
  void reset();
  // All storage is fixed. The callback never allocates or takes a lock.
  void process(float *l, float *r, int count, const float *scL,
               const float *scR, const Parameters &p) noexcept;
  std::array<float, maxBands> gainReduction{};
  float busReduction = 0, inputPeak = 0, outputPeak = 0;

private:
  struct BandState {
    std::array<FilterState, 2> eq, detector;
    Coefficients eqC, detectorC;
    double envelope = 0, gr = 0, frequency = 1000, q = 1, gain = 0, enable = 0;
    double attackC = 0, releaseC = 0;
    int shape = 0, channel = 0;
  };
  std::array<BandState, maxBands> bands;
  std::array<FilterState, 2> sidechainHP;
  Coefficients hp;
  double sampleRate = 48000, smoothing = 0, detectorSmoothing = 0;
  double busEnvelope = 0, busGR = 0, busWet = 0, inputGain = 1, outputGain = 1,
         wet = 1;
  int phase = 0;
};
} // namespace feral
