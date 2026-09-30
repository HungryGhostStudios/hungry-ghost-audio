#include "DSP/SuiteEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace hungryghost;
namespace {
constexpr double pi = 3.14159265358979323846;
using Controls = std::array<float, 6>;
constexpr Controls nominal{-20.f, 4.f, 2.f, 120.f, 20.f, 0.f};

void require(bool condition, const std::string& reason) {
  if (!condition) throw std::runtime_error(reason);
}
void near(double actual, double expected, double tolerance,
          const std::string& reason) {
  require(std::abs(actual - expected) <= tolerance,
          reason + " (actual " + std::to_string(actual) + ", expected " +
              std::to_string(expected) + ")");
}
AdvancedSettings precision() {
  AdvancedSettings a;
  a.bondModel = 1;
  a.bondDetector = 1;
  a.bondKnee = 0;
  return a;
}
struct Stereo {
  std::vector<float> l, r;
  explicit Stereo(int count) : l(count), r(count) {}
  int size() const { return static_cast<int>(l.size()); }
};
Stereo sine(double sr, double seconds, double hz, float l = .5f,
            float r = .5f) {
  Stereo signal(static_cast<int>(std::lround(sr * seconds)));
  for (int i = 0; i < signal.size(); ++i) {
    const float value = static_cast<float>(std::sin(2 * pi * hz * i / sr));
    signal.l[i] = value * l;
    signal.r[i] = value * r;
  }
  return signal;
}
SuiteEngine engine(double sr, const AdvancedSettings& advanced,
                   const Controls& controls = nominal) {
  SuiteEngine e(Kind::BusCompressor);
  e.prepare(sr);
  e.setControls(controls, 1, 0);
  e.setAdvanced(advanced);
  e.reset();
  return e;
}
void process(SuiteEngine& e, Stereo& signal, const Stereo* key = nullptr,
             int block = 137) {
  for (int offset = 0; offset < signal.size(); offset += block)
    e.process(signal.l.data() + offset, signal.r.data() + offset,
              key ? key->l.data() + offset : nullptr,
              key ? key->r.data() + offset : nullptr,
              std::min(block, signal.size() - offset));
}
double rms(const std::vector<float>& values, int start = 0) {
  double sum = 0;
  for (int i = start; i < static_cast<int>(values.size()); ++i)
    sum += static_cast<double>(values[i]) * values[i];
  return std::sqrt(sum / (values.size() - start));
}
void finite(const Stereo& signal, double bound, const std::string& reason) {
  for (const auto* channel : {&signal.l, &signal.r})
    for (float value : *channel)
      require(std::isfinite(value) && std::abs(value) <= bound, reason);
}

void calibratedCompression() {
  // A sustained sinusoidal external key has a known RMS level. The independent
  // quiet programme avoids conflating detector calibration with waveform gain.
  auto e = engine(48000, precision());
  auto signal = sine(48000, 2, 337, .05f, .03f);
  const auto original = signal;
  const auto key = sine(48000, 2, 1000);
  process(e, signal, &key);
  const double expected = (20 * std::log10(.5 / std::sqrt(2.)) + 20) * .75;
  near(e.gainReductionLeft(), expected, .25, "RMS compression ratio calibration");
  near(-20 * std::log10(rms(signal.l, 48000) / rms(original.l, 48000)),
       expected, .25, "Reported reduction agrees with applied signal gain");
  near(e.gainReductionRight(), e.gainReductionLeft(), .0001,
       "Linked channel reduction meters agree");
  near(e.gainReduction(), e.gainReductionLeft(), .0001,
       "Combined meter agrees with linked reduction");
  std::cout << "PASS calibrated RMS ratio and signal/meter agreement\n";
}

void legacyIsolation() {
  AdvancedSettings original;
  auto precisionControls = precision();
  precisionControls.bondModel = 0;
  precisionControls.bondTopology = 1;
  precisionControls.bondDetector = 1;
  precisionControls.bondLink = 0;
  precisionControls.bondKnee = 24;
  precisionControls.bondRange = 0;
  precisionControls.bondKeyLowpass = 1000;
  precisionControls.bondAutoRelease = true;
  auto unchanged = engine(48000, original);
  auto advanced = engine(48000, precisionControls);
  auto first = sine(48000, .5, 997, .7f, .03f);
  auto second = first;
  process(unchanged, first);
  process(advanced, second);
  require(first.l == second.l && first.r == second.r,
          "Precision controls must not alter Original-mode samples");
  require(unchanged.gainReduction() > 7,
          "Original mode must retain its existing compression response");
  std::cout << "PASS Original-mode isolation from Precision controls\n";
}

void stereoLinking() {
  const auto key = sine(48000, 1.5, 1000, .5f, .01f);
  std::array<float, 3> rightReduction{};
  int index = 0;
  for (float link : {0.f, .5f, 1.f}) {
    auto a = precision();
    a.bondLink = link;
    auto e = engine(48000, a);
    auto signal = sine(48000, 1.5, 337, .12f, .03f);
    process(e, signal, &key);
    require(e.gainReductionLeft() > 7.5, "Left key must cause useful compression");
    rightReduction[index++] = e.gainReductionRight();
    if (link == 0)
      require(e.gainReductionRight() < .05,
              "Unlinked quiet channel must remain uncompressed");
    if (link == 1) {
      near(e.gainReductionLeft(), e.gainReductionRight(), .0001,
           "Full link must apply identical channel reduction");
      for (int i = 48000; i < signal.size(); ++i)
        near(signal.l[i], signal.r[i] * 4, .000001,
             "Full linking preserves programme stereo balance");
    }
  }
  require(rightReduction[1] > rightReduction[0] + .5 &&
              rightReduction[1] < rightReduction[2] - .5,
          "Partial link must provide an intermediate coupling response");
  std::cout << "PASS independent, partially linked and fully linked channels\n";
}

void rangeAndUnity() {
  for (float range : {0.f, 3.f, 60.f}) {
    auto a = precision();
    a.bondRange = range;
    auto e = engine(48000, a, {-40, 10, .1f, 100, 20, 0});
    auto signal = sine(48000, 1, 997, .8f, .3f);
    const auto original = signal;
    process(e, signal);
    require(e.gainReductionLeft() <= range + .001,
            "Applied gain reduction must respect range limit");
    if (range == 0) {
      near(e.gainReductionLeft(), 0, .00001, "Zero range meter must read zero");
      for (int i = 0; i < signal.size(); ++i) {
        near(signal.l[i], original.l[i], .000001, "Zero range must preserve left audio");
        near(signal.r[i], original.r[i], .000001, "Zero range must preserve right audio");
      }
    } else if (range == 3) {
      near(e.gainReductionLeft(), 3, .01, "Range cap must be attainable");
    } else {
      require(e.gainReductionLeft() > 25, "Full range must permit deep compression");
    }
  }
  auto a = precision();
  a.bondKnee = 24;
  auto unity = engine(48000, a, {-60, 1, .1f, 10, 20, 0});
  auto signal = sine(48000, .25, 677, .8f, .4f);
  const auto original = signal;
  process(unity, signal);
  for (int i = 0; i < signal.size(); ++i)
    near(signal.l[i], original.l[i], .000001, "Ratio one must be transparent");
  std::cout << "PASS range limits, zero-range transparency and unity ratio\n";
}

float keyResponse(double hz, float highpass, float lowpass) {
  auto a = precision();
  a.bondKeyLowpass = lowpass;
  auto controls = nominal;
  controls[0] = -24;
  controls[4] = highpass;
  auto e = engine(48000, a, controls);
  auto signal = sine(48000, 1.5, 337, .01f, .01f);
  const auto key = sine(48000, 1.5, hz, .6f, .6f);
  process(e, signal, &key);
  return e.gainReductionLeft();
}
void detectorFilters() {
  const float bassOpen = keyResponse(60, 20, 20000);
  const float bassCut = keyResponse(60, 500, 20000);
  const float trebleOpen = keyResponse(8000, 20, 20000);
  const float trebleCut = keyResponse(8000, 20, 1000);
  require(bassOpen > 10 && bassOpen - bassCut > 8,
          "Key high-pass must remove low-frequency detector pumping");
  require(trebleOpen > 10 && trebleOpen - trebleCut > 8,
          "Key low-pass must reject high-frequency detector energy");
  std::cout << "PASS external key high-pass and low-pass rejection\n";
}

void softKnee() {
  std::array<float, 2> belowThreshold{};
  std::array<float, 2> aboveKnee{};
  for (int index = 0; index < 2; ++index) {
    auto a = precision();
    a.bondKnee = index == 0 ? 0.f : 12.f;
    for (int level = 0; level < 2; ++level) {
      auto e = engine(48000, a);
      auto signal = sine(48000, 1.5, 337, .03f, .03f);
      const float peak = static_cast<float>(std::sqrt(2.) *
                            std::pow(10., (level == 0 ? -23. : -8.) / 20.));
      const auto key = sine(48000, 1.5, 1000, peak, peak);
      process(e, signal, &key);
      (level == 0 ? belowThreshold : aboveKnee)[index] = e.gainReductionLeft();
    }
  }
  require(belowThreshold[0] < .02 && belowThreshold[1] > .2,
          "Soft knee must begin compressing gently below threshold");
  near(aboveKnee[1], aboveKnee[0], .05,
       "Knee width must preserve compression ratio above the knee");
  std::cout << "PASS soft-knee onset and preserved above-knee ratio\n";
}

void detectorsAndTopology() {
  std::array<float, 2> transientReduction{};
  for (int detector : {0, 1}) {
    auto a = precision();
    a.bondDetector = detector;
    auto e = engine(48000, a, {-20, 4, .1f, 100, 20, 0});
    auto signal = sine(48000, .003, 1000, .9f, .9f);
    process(e, signal);
    transientReduction[detector] = e.gainReductionLeft();
  }
  require(transientReduction[0] > transientReduction[1] + 2,
          "Peak mode must catch short transients more strongly than RMS mode");

  std::array<Stereo, 2> outputs{sine(48000, 1.5, 997, .7f, .3f),
                                sine(48000, 1.5, 997, .7f, .3f)};
  for (int topology : {0, 1}) {
    auto a = precision();
    a.bondTopology = topology;
    auto e = engine(48000, a, {-24, 4, 5, 100, 20, 0});
    process(e, outputs[topology]);
    finite(outputs[topology], .701, "Feed-forward/feedback output must remain bounded");
    require(e.gainReductionLeft() > 5 && e.gainReductionLeft() < 24,
            "Each topology must settle to useful finite compression");
    const double expected = (20 * std::log10(.7 / std::sqrt(2.)) + 24) * .75;
    near(e.gainReductionLeft(), expected, .4,
         "Each topology must retain its selected steady-state ratio");
  }
  double difference = 0;
  for (int i = 0; i < 12000; ++i) {
    const double d = outputs[0].l[i] - outputs[1].l[i];
    difference += d * d;
  }
  require(std::sqrt(difference / 12000) > .0001,
          "Feedback and feed-forward must have distinct transient responses");
  std::cout << "PASS peak/RMS transient contrast and stable distinct topologies\n";
}

float releaseAfter(bool automatic, double excitationSeconds) {
  auto a = precision();
  a.bondDetector = 0;
  a.bondAutoRelease = automatic;
  auto e = engine(48000, a, {-24, 4, .1f, 200, 20, 0});
  auto signal = sine(48000, excitationSeconds, 1000, .8f, .8f);
  process(e, signal);
  Stereo silence(5760); // 120 ms after the stimulus stops.
  process(e, silence);
  return e.gainReductionLeft();
}
void adaptiveRelease() {
  const float briefManual = releaseAfter(false, .01);
  const float briefAuto = releaseAfter(true, .01);
  const float heldManual = releaseAfter(false, 2);
  const float heldAuto = releaseAfter(true, 2);
  require(briefAuto + .1 < briefManual,
          "Auto release must recover faster after a brief transient");
  require(heldAuto > heldManual + .1,
          "Auto release must retain a longer tail after sustained compression");
  std::cout << "PASS programme-dependent release after brief and sustained input\n";
}

void resetAndBlockSize() {
  for (int topology : {0, 1}) {
    auto a = precision();
    a.bondTopology = topology;
    a.bondLink = .37f;
    a.bondAutoRelease = true;
    a.bondKeyLowpass = 8000;
    auto one = engine(48000, a);
    auto split = engine(48000, a);
    auto first = sine(48000, .4, 731, .8f, .12f);
    auto second = first;
    process(one, first, nullptr, first.size());
    process(split, second, nullptr, 17);
    require(first.l == second.l && first.r == second.r,
            "Host block boundaries must not change sample results");
    one.reset();
    near(one.gainReductionLeft(), 0, .000001, "Reset must clear left meter");
    near(one.gainReductionRight(), 0, .000001, "Reset must clear right meter");
    auto repeated = sine(48000, .4, 731, .8f, .12f);
    process(one, repeated, nullptr, 83);
    require(repeated.l == first.l && repeated.r == first.r,
            "Reset must reproduce initial processing exactly");
  }
  std::array<float, 5> reductions{};
  int index = 0;
  for (double sr : {22050., 44100., 48000., 96000., 192000.}) {
    auto e = engine(sr, precision());
    auto signal = sine(sr, 1.5, 1000, .5f, .2f);
    process(e, signal);
    reductions[index++] = e.gainReductionLeft();
    finite(signal, .501, "Sample-rate sweep must remain finite and bounded");
  }
  for (float reduction : reductions)
    near(reduction, reductions[2], .15,
         "Steady compression should be consistent across sample rates");
  std::cout << "PASS reset reproducibility, exact block invariance and sample-rate calibration\n";
}

void extremesAndAutomation() {
  for (double sr : {8000., 44100., 192000., 384000.}) {
    auto e = engine(sr, precision());
    std::uint32_t seed = 0x143689adu;
    for (int step = 0; step < 48; ++step) {
      auto a = precision();
      a.bondModel = step % 3 == 0 ? 0 : 1;
      a.bondTopology = step % 2;
      a.bondDetector = (step / 2) % 2;
      a.bondAutoRelease = (step / 3) % 2 != 0;
      a.bondKnee = step % 2 ? 24.f : 0.f;
      a.bondRange = step % 2 ? 60.f : 0.f;
      a.bondLink = step % 2 ? 1.f : 0.f;
      a.bondKeyLowpass = step % 2 ? 20000.f : 1000.f;
      e.setAdvanced(a);
      e.setControls(step % 2 ? Controls{-60, 10, .1f, 10, 500, 24}
                            : Controls{0, 1, 100, 1000, 20, -12},
                    step % 4 == 0 ? 0.f : 1.f, step % 2 ? 24.f : -60.f);
      Stereo signal(257);
      for (int i = 0; i < signal.size(); ++i) {
        seed = seed * 1664525u + 1013904223u;
        signal.l[i] = (static_cast<int>(seed >> 8) - 8388608) / 8388608.f;
        signal.r[i] = -.35f * signal.l[i];
      }
      if (step == 24) {
        signal.l[7] = std::numeric_limits<float>::quiet_NaN();
        signal.r[11] = std::numeric_limits<float>::infinity();
      }
      process(e, signal, nullptr, 31);
      finite(signal, 252, "Extreme controls and mode automation must remain finite/bounded");
      require(std::isfinite(e.gainReductionLeft()) &&
                  std::isfinite(e.gainReductionRight()),
              "Automation must keep both reduction meters finite");
    }
    float mono = .1f;
    e.process(&mono, nullptr, nullptr, nullptr, 0);
    near(mono, .1f, 0, "Empty blocks must not touch audio");
    e.process(&mono, nullptr, nullptr, nullptr, 1);
    require(std::isfinite(mono), "Mono processing must remain finite");
  }
  std::cout << "PASS extreme settings, mode automation, bad input and mono/empty blocks\n";
}

void extremeFiniteRecovery() {
  for (int model : {0, 1}) {
    auto a = precision();
    a.bondModel = model;
    a.bondTopology = 1;
    auto e = engine(48000, a, {-24, 4, 2, 100, 80, 0});
    Stereo hostile(1024);
    for (int i = 0; i < hostile.size(); ++i)
      hostile.l[i] = hostile.r[i] =
          (i % 2 ? -1.f : 1.f) * std::numeric_limits<float>::max();
    process(e, hostile);
    for (float sample : hostile.l)
      require(std::isfinite(sample), "Extreme finite input must not emit NaN/Inf");
    auto normal = sine(48000, 2, 1000, .5f, .5f);
    process(e, normal);
    double tailEnergy = 0;
    for (int i = normal.size() - 12000; i < normal.size(); ++i)
      tailEnergy += normal.l[i] * normal.l[i];
    require(std::sqrt(tailEnergy / 12000) > .001,
            "Normal audio must recover after overflowing host input");
    require(std::isfinite(e.gainReductionLeft()) &&
                std::isfinite(e.gainReductionRight()),
            "Overflowing host input must not poison reduction meters");
  }
  std::cout << "PASS Original and Precision recovery from extreme finite host input\n";
}
} // namespace

int main() {
  try {
    legacyIsolation();
    calibratedCompression();
    stereoLinking();
    rangeAndUnity();
    detectorFilters();
    softKnee();
    detectorsAndTopology();
    adaptiveRelease();
    resetAndBlockSize();
    extremesAndAutomation();
    extremeFiniteRecovery();
    std::cout << "PASS BOND Precision measured behaviours\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAIL BOND: " << error.what() << '\n';
    return 1;
  }
}
