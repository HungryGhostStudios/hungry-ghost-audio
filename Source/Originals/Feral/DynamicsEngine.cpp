#include "DynamicsEngine.h"
#include <algorithm>

namespace feral {
namespace {
constexpr double pi = 3.14159265358979323846;
double db(double x) noexcept { return 20 * std::log10(std::max(1e-12, x)); }
double amp(double x) noexcept { return std::pow(10.0, x / 20.0); }
double timeCoefficient(double ms, double sr) noexcept {
  return std::exp(-1.0 / (sr * std::max(.05, ms) * .001));
}
Coefficients bandpass(double f, double q, double sr) noexcept {
  const double w = 2 * pi * std::clamp(f, 20.0, sr * .45) / sr;
  const double a = std::sin(w) / (2 * std::clamp(q, .1, 18.0)), d = 1 + a;
  return {a / d, 0, -a / d, -2 * std::cos(w) / d, (1 - a) / d};
}
double smooth(double target, double value, double c) noexcept {
  return target + c * (value - target);
}
} // namespace
double Coefficients::magnitude(double f, double sr) const noexcept {
  const auto z = std::polar(1.0, -2 * pi * std::clamp(f, 0.0, sr * .5) / sr);
  return std::abs((b0 + b1 * z + b2 * z * z) / (1.0 + a1 * z + a2 * z * z));
}
Coefficients filter(int shape, double f, double q, double gain,
                    double sr) noexcept {
  f = std::clamp(f, 20.0, sr * .45);
  q = std::clamp(q, .1, 18.0);
  const double w = 2 * pi * f / sr, cs = std::cos(w), sn = std::sin(w);
  const double a = sn / (2 * q),
               A = std::pow(10.0, std::clamp(gain, -36.0, 24.0) / 40.0);
  double b0, b1, b2, a0, a1, a2;
  if (shape == lowCut || shape == highCut) {
    const double sign = shape == lowCut ? 1.0 : -1.0;
    b0 = (1 + sign * cs) * .5;
    b1 = -sign * (1 + sign * cs);
    b2 = b0;
    a0 = 1 + a;
    a1 = -2 * cs;
    a2 = 1 - a;
  } else if (shape == lowShelf || shape == highShelf) {
    // Fixed monotonic shelf slope. Q is used by its detector, not shelf
    // resonance.
    const double beta = std::sqrt(2 * A) * sn;
    if (shape == lowShelf) {
      b0 = A * ((A + 1) - (A - 1) * cs + beta);
      b1 = 2 * A * ((A - 1) - (A + 1) * cs);
      b2 = A * ((A + 1) - (A - 1) * cs - beta);
      a0 = (A + 1) + (A - 1) * cs + beta;
      a1 = -2 * ((A - 1) + (A + 1) * cs);
      a2 = (A + 1) + (A - 1) * cs - beta;
    } else {
      b0 = A * ((A + 1) + (A - 1) * cs + beta);
      b1 = -2 * A * ((A - 1) + (A + 1) * cs);
      b2 = A * ((A + 1) + (A - 1) * cs - beta);
      a0 = (A + 1) - (A - 1) * cs + beta;
      a1 = 2 * ((A - 1) - (A + 1) * cs);
      a2 = (A + 1) - (A - 1) * cs - beta;
    }
  } else {
    b0 = 1 + a * A;
    b1 = -2 * cs;
    b2 = 1 - a * A;
    a0 = 1 + a / A;
    a1 = -2 * cs;
    a2 = 1 - a / A;
  }
  return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}
double reduction(double level, double threshold, double ratio, double knee,
                 double range) noexcept {
  const double over = level - threshold, slope = 1 - 1 / std::max(1.0, ratio);
  double gr = 0;
  if (knee > .001 && over > -knee * .5 && over < knee * .5)
    gr = slope * std::pow(over + knee * .5, 2) / (2 * knee);
  else if (over >= knee * .5)
    gr = slope * over;
  return std::clamp(gr, 0.0, std::max(0.0, range));
}
void DynamicsEngine::prepare(double sr, const Parameters &p) {
  sampleRate = std::max(8000.0, sr);
  smoothing = timeCoefficient(10, sampleRate);
  detectorSmoothing = timeCoefficient(3, sampleRate);
  reset();
  inputGain = amp(p.input);
  outputGain = amp(p.output);
  wet = p.mix * .01;
  busWet = p.busEnabled;
  for (int i = 0; i < maxBands; ++i) {
    auto &b = bands[i];
    const auto &v = p.bands[i];
    b.frequency = v.frequency;
    b.q = v.q;
    b.gain = v.gain;
    b.enable = v.enabled;
    b.shape = (int)v.shape;
    b.channel = (int)v.channel;
    b.eqC = filter(b.shape, b.frequency, b.q, b.gain, sampleRate);
  }
}
void DynamicsEngine::reset() {
  bands = {};
  sidechainHP = {};
  busEnvelope = busGR = 0;
  phase = 0;
  gainReduction = {};
  busReduction = inputPeak = outputPeak = 0;
}
void DynamicsEngine::process(float *l, float *r, int count, const float *scL,
                             const float *scR, const Parameters &p) noexcept {
  const bool stereoInput = r != nullptr;
  const double inTarget = amp(p.input), outTarget = amp(p.output),
               wetTarget = p.mix * .01;
  const double busAttack = timeCoefficient(p.attack, sampleRate),
               busRelease = timeCoefficient(p.release, sampleRate);
  const double controlSmooth = std::pow(smoothing, 16);
  hp = filter(lowCut, p.detectorHP, .707, 0, sampleRate);
  for (int i = 0; i < maxBands; ++i) {
    bands[i].attackC = timeCoefficient(p.bands[i].attack, sampleRate);
    bands[i].releaseC = timeCoefficient(p.bands[i].release, sampleRate);
  }
  inputPeak = outputPeak = 0;
  for (int n = 0; n < count; ++n) {
    const double dryL = l[n], dryR = stereoInput ? r[n] : dryL;
    inputPeak =
        std::max(inputPeak, (float)std::max(std::abs(dryL), std::abs(dryR)));
    inputGain = smooth(inTarget, inputGain, smoothing);
    outputGain = smooth(outTarget, outputGain, smoothing);
    wet = smooth(wetTarget, wet, smoothing);
    busWet = smooth(p.busEnabled, busWet, smoothing);
    const double preL = dryL * inputGain, preR = dryR * inputGain;
    double xL = preL, xR = preR;
    const double keyL = scL ? scL[n] : 0, keyR = scR ? scR[n] : keyL;
    for (int i = 0; i < maxBands; ++i) {
      auto &b = bands[i];
      const auto &v = p.bands[i];
      if (phase == 0) {
        const int sh = (int)v.shape, ch = (int)v.channel;
        if (sh != b.shape || ch != b.channel) {
          b.eq = {};
          b.detector = {};
          b.envelope = 0;
        }
        b.shape = sh;
        b.channel = ch;
        b.frequency = smooth(v.frequency, b.frequency, controlSmooth);
        b.q = smooth(v.q, b.q, controlSmooth);
        b.gain = smooth(v.gain, b.gain, controlSmooth);
        b.enable = smooth(v.enabled, b.enable, controlSmooth);
        b.detectorC =
            sh == lowShelf ? filter(highCut, b.frequency, .707, 0, sampleRate)
            : sh == highShelf ? filter(lowCut, b.frequency, .707, 0, sampleRate)
                              : bandpass(b.frequency, b.q, sampleRate);
        b.eqC = filter(sh, b.frequency, b.q, b.gain - b.gr, sampleRate);
      }
      double dL = v.external > .5f ? keyL : preL,
             dR = v.external > .5f ? keyR : preR;
      if (b.channel == mid)
        dL = dR = (dL + dR) * .5;
      else if (b.channel == side)
        dL = dR = stereoInput ? (dL - dR) * .5 : 0;
      else if (b.channel == left)
        dR = dL;
      else if (b.channel == right)
        dL = dR;
      dL = b.detector[0].tick(dL, b.detectorC);
      dR = b.detector[1].tick(dR, b.detectorC);
      b.envelope =
          smooth(std::max(dL * dL, dR * dR), b.envelope, detectorSmoothing);
      const double desired =
          v.dynamic > .5f && b.shape < lowCut
              ? reduction(db(std::sqrt(std::max(0.0, b.envelope))), v.threshold,
                          v.ratio, v.knee, v.range)
              : 0;
      b.gr = smooth(desired, b.gr, desired > b.gr ? b.attackC : b.releaseC);
      if (b.channel == mid || b.channel == side) {
        double m = (xL + xR) * .5, s = (xL - xR) * .5;
        double &x = b.channel == mid ? m : s;
        x += b.enable * (b.eq[0].tick(x, b.eqC) - x);
        xL = m + s;
        xR = m - s;
      } else {
        const double yL = b.eq[0].tick(xL, b.eqC), yR = b.eq[1].tick(xR, b.eqC);
        if (b.channel != right)
          xL += b.enable * (yL - xL);
        if (b.channel != left)
          xR += b.enable * (yR - xR);
      }
    }
    double dL = p.external > .5f ? keyL : xL, dR = p.external > .5f ? keyR : xR;
    dL = sidechainHP[0].tick(dL, hp);
    dR = sidechainHP[1].tick(dR, hp);
    busEnvelope =
        smooth(std::max(dL * dL, dR * dR), busEnvelope, detectorSmoothing);
    const double desired = reduction(db(std::sqrt(std::max(0.0, busEnvelope))),
                                     p.threshold, p.ratio, p.knee, p.range);
    busGR = smooth(desired, busGR, desired > busGR ? busAttack : busRelease);
    const double busGain = amp(-busGR * busWet);
    xL = (preL + wet * (xL * busGain - preL)) * outputGain;
    xR = (preR + wet * (xR * busGain - preR)) * outputGain;
    // Host bypass is exact, while filter and detector history remains warm.
    l[n] = p.bypass > .5f ? (float)dryL : (float)xL;
    if (stereoInput)
      r[n] = p.bypass > .5f ? (float)dryR : (float)xR;
    outputPeak =
        std::max(outputPeak, (float)std::max(std::abs(l[n]),
                                             stereoInput ? std::abs(r[n]) : 0));
    phase = (phase + 1) % 16;
  }
  for (int i = 0; i < maxBands; ++i)
    gainReduction[i] = (float)(bands[i].gr * bands[i].enable);
  busReduction = (float)(busGR * busWet);
}
} // namespace feral
