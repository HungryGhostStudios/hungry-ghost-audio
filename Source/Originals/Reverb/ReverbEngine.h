#pragma once
#include <array>
#include <cstddef>
#include <vector>

namespace after {
struct Parameters {
  int character = 2;
  float decay = 3.2f, size = 68.0f, tone = -20.0f, motion = 22.0f;
  float preDelay = 24.0f, mix = 25.0f;
  float early = 20.0f, diffusion = 88.0f, width = 100.0f;
  float lowRatio = 1.25f, highRatio = 0.65f, lowCrossover = 250.0f,
        highCrossover = 6000.0f;
  float lowCut = 120.0f, highCut = 12500.0f;
  float rate = 0.35f, duck = 0.0f, release = 350.0f, inputDb = 0.0f,
        outputDb = 0.0f;
  bool freeze = false, bypass = false;
};

// No JUCE dependency: the same engine runs in the plug-in and offline tests.
class ReverbEngine {
public:
  void prepare(double sampleRate);
  void reset() noexcept;
  void setParameters(const Parameters &) noexcept;
  void process(float *left, float *right, int samples) noexcept;
  double getSampleRate() const noexcept { return sr; }

private:
  struct Smooth {
    float value = 0, target = 0, coefficient = 0;
    void prepare(double rate, float seconds) noexcept;
    void set(float x, bool immediate = false) noexcept {
      target = x;
      if (immediate)
        value = x;
    }
    float next() noexcept;
  };
  struct Delay {
    std::vector<float> data;
    int writeIndex = 0;
    void prepare(int capacity);
    void clear() noexcept;
    float read(float samples) const noexcept;
    void push(float x) noexcept;
  };
  struct Allpass {
    Delay delay;
    float samples = 1;
    float process(float input, float gain) noexcept;
  };
  struct Line {
    Delay delay;
    Smooth length, lowGain, midGain, highGain;
    float lowState = 0, highState = 0;
    double oscillatorSin = 0, oscillatorCos = 1, rotationSin = 0,
           rotationCos = 1;
    float holdLength = 1;
  };
  std::array<Line, 16> lines;
  std::array<std::array<Allpass, 4>, 2> diffusers;
  std::array<Delay, 2> preDelays, earlyDelays;
  std::array<float, 2> wetLowState{}, wetHighState{};
  Smooth preDelay, wetMix, size, motion, earlyMix, diffusion, width, inputGain,
      outputGain, freezeMix, bypassMix;
  Smooth lowPole, highPole, lowCutPole, highCutPole, duckAmount, duckRelease;
  double sr = 48000;
  float envelope = 0, attackPole = 0;
  bool prepared = false, firstParameters = true;
  Parameters params;
  static void hadamard(std::array<float, 16> &) noexcept;
};
} // namespace after
