#include "ReverbEngine.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace after {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr std::array<float, 16> times{
    29.7f, 34.3f, 39.1f, 43.7f, 47.9f, 53.3f,  59.9f,  64.7f,
    71.3f, 76.1f, 82.9f, 89.3f, 97.1f, 103.9f, 109.7f, 117.1f};
constexpr std::array<float, 5> characterScale{.58f, .79f, 1.0f, .87f, 1.30f};
constexpr std::array<float, 5> characterMotion{.5f, .7f, 1.0f, .6f, 1.45f};
constexpr std::array<int, 16> inL{1,  1,  1,  1,  1,  1,  1,  1,
                                  -1, -1, -1, -1, -1, -1, -1, -1};
constexpr std::array<int, 16> inR{1, 1, -1, -1, 1, 1, -1, -1,
                                  1, 1, -1, -1, 1, 1, -1, -1};
constexpr std::array<int, 16> outL{1, -1, 1, -1, 1, -1, 1, -1,
                                   1, -1, 1, -1, 1, -1, 1, -1};
constexpr std::array<int, 16> outR{1,  1,  1,  1,  -1, -1, -1, -1,
                                   -1, -1, -1, -1, 1,  1,  1,  1};
float finiteClamp(float v, float lo, float hi, float fallback) noexcept {
  return std::isfinite(v) ? std::clamp(v, lo, hi) : fallback;
}
float zap(float x) noexcept { return std::abs(x) < 1.0e-25f ? 0.0f : x; }
float dbGain(float db) noexcept { return std::pow(10.0f, db * .05f); }
} // namespace
void ReverbEngine::Smooth::prepare(double rate, float seconds) noexcept {
  coefficient = static_cast<float>(std::exp(-1.0 / (rate * seconds)));
}
float ReverbEngine::Smooth::next() noexcept {
  value = target + (value - target) * coefficient;
  if (std::abs(value - target) < 1.0e-6f)
    value = target;
  return value;
}
void ReverbEngine::Delay::prepare(int capacity) {
  data.assign(static_cast<std::size_t>(capacity), 0.0f);
  writeIndex = 0;
}
void ReverbEngine::Delay::clear() noexcept {
  std::fill(data.begin(), data.end(), 0.0f);
  writeIndex = 0;
}
float ReverbEngine::Delay::read(float samples) const noexcept {
  samples = std::clamp(samples, 1.0f, static_cast<float>(data.size() - 2));
  float index = static_cast<float>(writeIndex) - samples;
  if (index < 0)
    index += static_cast<float>(data.size());
  const int a = static_cast<int>(index),
            b = a + 1 == static_cast<int>(data.size()) ? 0 : a + 1;
  const float fraction = index - static_cast<float>(a);
  return data[static_cast<std::size_t>(a)] +
         fraction * (data[static_cast<std::size_t>(b)] -
                     data[static_cast<std::size_t>(a)]);
}
void ReverbEngine::Delay::push(float x) noexcept {
  data[static_cast<std::size_t>(writeIndex)] = zap(x);
  if (++writeIndex == static_cast<int>(data.size()))
    writeIndex = 0;
}
float ReverbEngine::Allpass::process(float input, float gain) noexcept {
  const float delayed = delay.read(samples);
  const float y = delayed - gain * input;
  delay.push(input + gain * y);
  return y;
}
void ReverbEngine::prepare(double rate) {
  sr = std::clamp(rate, 8000.0, 384000.0);
  for (auto &line : lines) {
    line.delay.prepare(static_cast<int>(sr * .36) + 16);
    line.length.prepare(sr, .15f);
    line.lowGain.prepare(sr, .03f);
    line.midGain.prepare(sr, .03f);
    line.highGain.prepare(sr, .03f);
  }
  constexpr float apTimes[2][4] = {{4.3f, 7.7f, 12.7f, 19.3f},
                                   {5.1f, 8.9f, 13.9f, 21.7f}};
  for (int c = 0; c < 2; ++c) {
    preDelays[c].prepare(static_cast<int>(sr * .26) + 16);
    earlyDelays[c].prepare(static_cast<int>(sr * .20) + 16);
    for (int a = 0; a < 4; ++a) {
      diffusers[c][a].samples =
          std::round(static_cast<float>(sr) * apTimes[c][a] * .001f);
      diffusers[c][a].delay.prepare(static_cast<int>(diffusers[c][a].samples) +
                                    8);
    }
  }
  for (auto *s :
       {&preDelay, &wetMix, &size, &motion, &earlyMix, &diffusion, &width,
        &inputGain, &outputGain, &freezeMix, &bypassMix, &lowPole, &highPole,
        &lowCutPole, &highCutPole, &duckAmount, &duckRelease})
    s->prepare(sr, .03f);
  preDelay.prepare(sr, .08f);
  freezeMix.prepare(sr, .03f);
  attackPole = static_cast<float>(std::exp(-1.0 / (sr * .004)));
  prepared = true;
  reset();
  setParameters(params);
}
void ReverbEngine::reset() noexcept {
  for (std::size_t i = 0; i < lines.size(); ++i) {
    auto &l = lines[i];
    l.delay.clear();
    l.lowState = l.highState = 0;
    const double phase = i * 2.399963229728653;
    l.oscillatorSin = std::sin(phase);
    l.oscillatorCos = std::cos(phase);
  }
  for (auto &channel : diffusers)
    for (auto &ap : channel)
      ap.delay.clear();
  for (auto &d : preDelays)
    d.clear();
  for (auto &d : earlyDelays)
    d.clear();
  wetLowState.fill(0);
  wetHighState.fill(0);
  envelope = 0;
  firstParameters = true;
}
void ReverbEngine::setParameters(const Parameters &incoming) noexcept {
  const bool wasFrozen = params.freeze;
  params = incoming;
  params.character = std::clamp(params.character, 0, 4);
  params.decay = finiteClamp(params.decay, .15f, 30, 3.2f);
  params.size = finiteClamp(params.size, 0, 100, 68);
  params.tone = finiteClamp(params.tone, -100, 100, -20);
  params.motion = finiteClamp(params.motion, 0, 100, 22);
  params.preDelay = finiteClamp(params.preDelay, 0, 250, 24);
  params.mix = finiteClamp(params.mix, 0, 100, 25);
  params.early = finiteClamp(params.early, 0, 100, 20);
  params.diffusion = finiteClamp(params.diffusion, 0, 100, 88);
  params.width = finiteClamp(params.width, 0, 150, 100);
  params.lowRatio = finiteClamp(params.lowRatio, .25f, 2, 1.25f);
  params.highRatio = finiteClamp(params.highRatio, .15f, 2, .65f);
  params.lowCrossover = finiteClamp(params.lowCrossover, 100, 1000, 250);
  params.highCrossover = finiteClamp(params.highCrossover, 1000, 12000, 6000);
  params.lowCut = finiteClamp(params.lowCut, 20, 1000, 120);
  params.highCut = finiteClamp(params.highCut, 1000, 20000, 12500);
  params.rate = finiteClamp(params.rate, .05f, 4, .35f);
  params.duck = finiteClamp(params.duck, 0, 100, 0);
  params.release = finiteClamp(params.release, 50, 1500, 350);
  params.inputDb = finiteClamp(params.inputDb, -24, 12, 0);
  params.outputDb = finiteClamp(params.outputDb, -24, 12, 0);
  if (!prepared)
    return;
  const bool immediate = firstParameters;
  firstParameters = false;
  const auto pole = [this](float hz) {
    return static_cast<float>(
        std::exp(-2 * pi * std::min(static_cast<double>(hz), sr * .45) / sr));
  };
  preDelay.set(params.preDelay * static_cast<float>(sr) * .001f, immediate);
  wetMix.set(params.mix * .01f, immediate);
  size.set(params.size * .01f, immediate);
  motion.set(params.motion * .01f * characterMotion[params.character],
             immediate);
  earlyMix.set(params.early * .01f, immediate);
  diffusion.set(params.diffusion * .0068f, immediate);
  width.set(params.width * .01f, immediate);
  inputGain.set(dbGain(params.inputDb), immediate);
  outputGain.set(dbGain(params.outputDb), immediate);
  freezeMix.set(params.freeze ? 1.0f : 0.0f, immediate);
  bypassMix.set(params.bypass ? 1.0f : 0.0f, immediate);
  lowPole.set(pole(params.lowCrossover), immediate);
  highPole.set(pole(params.highCrossover), immediate);
  lowCutPole.set(pole(params.lowCut), immediate);
  highCutPole.set(pole(params.highCut), immediate);
  duckAmount.set(params.duck * .01f, immediate);
  duckRelease.set(
      static_cast<float>(std::exp(-1.0 / (sr * params.release * .001f))),
      immediate);
  const float highTime =
      params.decay * params.highRatio * std::pow(2.0f, params.tone / 120.0f);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    auto &line = lines[i];
    const float length = std::round(times[i] * .001f * static_cast<float>(sr) *
                                    (.35f + 1.35f * params.size * .01f) *
                                    characterScale[params.character]);
    line.length.set(length, immediate);
    if ((params.freeze && !wasFrozen) || immediate)
      line.holdLength = std::round(line.length.value);
    const auto gain = [&](float time) {
      return std::pow(10.0f, -3.0f * length / (static_cast<float>(sr) * time));
    };
    const float gl = gain(params.decay * params.lowRatio),
                gm = gain(params.decay), gh = gain(highTime);
    // Cascaded complementary shelves. Cap the product of their peak gains to
    // guarantee a passive feedback loop even for crossed low/high RT60 ratios.
    const float upperBound = std::max(gl, gm) * std::max(1.0f, gh / gm);
    const float correction = std::max({gl, gm, gh}) / upperBound;
    line.lowGain.set(gl * correction, immediate);
    line.midGain.set(gm * correction, immediate);
    line.highGain.set(gh / gm, immediate);
    const double angle = 2 * pi * params.rate * (.73 + .037 * i) / sr;
    line.rotationSin = std::sin(angle);
    line.rotationCos = std::cos(angle);
    // Re-normalise oscillators at block boundaries to avoid long-run drift.
    const double norm = std::hypot(line.oscillatorSin, line.oscillatorCos);
    if (norm > 0) {
      line.oscillatorSin /= norm;
      line.oscillatorCos /= norm;
    }
  }
}
void ReverbEngine::hadamard(std::array<float, 16> &v) noexcept {
  for (int span = 1; span < 16; span *= 2)
    for (int start = 0; start < 16; start += 2 * span)
      for (int j = 0; j < span; ++j) {
        const auto a = v[start + j], b = v[start + j + span];
        v[start + j] = a + b;
        v[start + j + span] = a - b;
      }
  for (auto &x : v)
    x *= .25f;
}
void ReverbEngine::process(float *left, float *right, int count) noexcept {
  if (!prepared || left == nullptr || count <= 0)
    return;
  for (int n = 0; n < count; ++n) {
    const float dryL = std::isfinite(left[n]) ? left[n] : 0,
                dryR = right ? (std::isfinite(right[n]) ? right[n] : 0) : dryL;
    const float inGain = inputGain.next(), outGain = outputGain.next(),
                wet = wetMix.next(), fr = freezeMix.next(),
                by = bypassMix.next();
    const float s = size.next(), m = motion.next(), ap = diffusion.next(),
                er = earlyMix.next(), stereo = width.next();
    const float lp = lowPole.next(), hp = highPole.next(),
                cutLo = lowCutPole.next(), cutHi = highCutPole.next(),
                duck = duckAmount.next(), rel = duckRelease.next();
    const float level =
        std::max(std::abs(dryL * inGain), std::abs(dryR * inGain));
    const float envPole = level > envelope ? attackPole : rel;
    envelope = zap(level + (envelope - level) * envPole);
    const float duckGain = 1.0f / (1.0f + duck * 12.0f * envelope);
    const float pd = preDelay.next();
    std::array<float, 2> signal{}, early{};
    for (int c = 0; c < 2; ++c) {
      const float in = (c == 0 ? dryL : dryR) * inGain * (1 - fr);
      const float delayed = preDelays[c].read(std::max(pd, 1.0f));
      const float predelayed = pd < 1 ? in * (1 - pd) + delayed * pd : delayed;
      preDelays[c].push(in);
      constexpr float taps[2][6] = {{7.1f, 11.9f, 19.3f, 31.1f, 43.7f, 61.3f},
                                    {8.9f, 14.3f, 23.9f, 37.1f, 49.9f, 67.7f}};
      for (int t = 0; t < 6; ++t)
        early[c] += earlyDelays[c].read(taps[c][t] * (.4f + 1.3f * s) *
                                        static_cast<float>(sr) * .001f) *
                    (t % 3 == 2 ? -.16f : .22f) / (1 + .2f * t);
      earlyDelays[c].push(predelayed);
      float diffuse = predelayed;
      for (auto &stage : diffusers[c])
        diffuse = stage.process(diffuse, ap);
      signal[c] = diffuse;
    }
    std::array<float, 16> feedback{};
    float tailL = 0, tailR = 0;
    for (std::size_t i = 0; i < lines.size(); ++i) {
      auto &l = lines[i];
      const double sine = l.oscillatorSin;
      l.oscillatorSin = sine * l.rotationCos + l.oscillatorCos * l.rotationSin;
      l.oscillatorCos = l.oscillatorCos * l.rotationCos - sine * l.rotationSin;
      const float normalDelay = l.length.next() + static_cast<float>(sine) * m *
                                                      static_cast<float>(sr) *
                                                      .00065f;
      const float delay = normalDelay * (1 - fr) + l.holdLength * fr;
      const float value = l.delay.read(delay);
      tailL += value * outL[i] * .25f;
      tailR += value * outR[i] * .25f;
      l.lowState = zap((1 - lp) * value + lp * l.lowState);
      const float gl = l.lowGain.next(), gm = l.midGain.next(),
                  gh = l.highGain.next();
      const float shelf = gl * l.lowState + gm * (value - l.lowState);
      l.highState = zap((1 - hp) * shelf + hp * l.highState);
      const float damped = l.highState + gh * (shelf - l.highState);
      feedback[i] = damped * (1 - fr) + value * fr;
    }
    hadamard(feedback);
    for (std::size_t i = 0; i < lines.size(); ++i)
      lines[i].delay.push(feedback[i] +
                          (signal[0] * inL[i] + signal[1] * inR[i]) *
                              .1767767f * (1 - fr));
    // Width only affects the wet side; dry stereo position is never changed.
    float wl = tailL * .65f * (1 - er) + early[0] * er,
          wr = tailR * .65f * (1 - er) + early[1] * er;
    const float mid = (wl + wr) * .5f, side = (wl - wr) * .5f * stereo;
    wl = mid + side;
    wr = mid - side;
    std::array<float, 2> wetSignal{wl, wr};
    for (int c = 0; c < 2; ++c) {
      wetLowState[c] = zap((1 - cutLo) * wetSignal[c] + cutLo * wetLowState[c]);
      const float highpassed = wetSignal[c] - wetLowState[c];
      wetHighState[c] = zap((1 - cutHi) * highpassed + cutHi * wetHighState[c]);
      wetSignal[c] = wetHighState[c] * duckGain;
    }
    // Linear mix keeps 0% dry and 100% wet exact, with no hidden dry boost.
    const float processedL =
                    (dryL * inGain * (1 - wet) + wetSignal[0] * wet) * outGain,
                processedR =
                    (dryR * inGain * (1 - wet) + wetSignal[1] * wet) * outGain;
    if (right) {
      left[n] = processedL * (1 - by) + dryL * by;
      right[n] = processedR * (1 - by) + dryR * by;
    } else
      left[n] = ((processedL + processedR) * .5f) * (1 - by) + dryL * by;
  }
}
} // namespace after
