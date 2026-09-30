#include "SuiteEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace hungryghost {
namespace {
constexpr double pi = 3.14159265358979323846;
float finite(float x) { return std::isfinite(x) ? x : 0.f; }
float clamp(float x, float lo, float hi) { return std::clamp(x, lo, hi); }
} // namespace
float SuiteEngine::Biquad::tick(float x) noexcept {
  double y = b0 * x + z1;
  z1 = b1 * x - a1 * y + z2;
  z2 = b2 * x - a2 * y;
  if (!std::isfinite(y)) {
    clear();
    return 0;
  }
  return static_cast<float>(y);
}
void SuiteEngine::Biquad::clear() noexcept { z1 = z2 = 0; }
float SuiteEngine::BondKeyFilter::tick(float x, float hpG, float lpG) noexcept {
  // Topology-preserving state-variable Butterworth sections remain stable
  // while their cutoff is automated. Each section is 12 dB/octave.
  const auto section = [](float input, float g, float& state1, float& state2,
                          bool highpass) noexcept {
    constexpr float k = 1.4142135623730951f;
    const float a1 = 1.f / (1.f + g * (g + k));
    const float a2 = g * a1, a3 = g * a2;
    const float v3 = input - state2;
    const float v1 = a1 * state1 + a2 * v3;
    const float v2 = state2 + a2 * state1 + a3 * v3;
    state1 = 2.f * v1 - state1;
    state2 = 2.f * v2 - state2;
    // Denormal suppression also makes a reset-to-silence deterministic.
    if (std::abs(state1) < 1.e-20f) state1 = 0;
    if (std::abs(state2) < 1.e-20f) state2 = 0;
    return highpass ? input - k * v1 - v2 : v2;
  };
  return section(section(x, hpG, hp1, hp2, true), lpG, lp1, lp2, false);
}
float SuiteEngine::db(float x) noexcept { return std::pow(10.f, x * .05f); }
float SuiteEngine::toDb(float x) noexcept {
  return 20.f * std::log10(std::max(x, 1.e-9f));
}
SuiteEngine::SuiteEngine(Kind k) : kind(k) {
  for (const auto &p : products)
    if (p.kind == k) {
      for (int i = 0; i < 6; ++i)
        target[i] = smooth[i] = p.controls[i].initial;
      break;
    }
}
bool SuiteEngine::usesDelay() const noexcept {
  return kind >= Kind::Delay && kind <= Kind::Vibrato || kind == Kind::Haas;
}
void SuiteEngine::prepare(double rate) {
  sr = std::clamp(rate, 8000., 384000.);
  smoothing = static_cast<float>(1. - std::exp(-1. / (.01 * sr)));
  bondPowerCoefficient = static_cast<float>(1. - std::exp(-1. / (.015 * sr)));
  bondHistoryCoefficient = static_cast<float>(1. - std::exp(-1. / (.25 * sr)));
  for (auto &d : delay)
    d.assign(usesDelay()?static_cast<std::size_t>(sr*(kind==Kind::SlapDelay?13.7:kind>=Kind::Delay&&kind<=Kind::DubDelay?12.2:kind==Kind::Comb?.075:.25))+8:8,0.f);
  reset();
}
void SuiteEngine::reset() {
  for (auto &bank : filters)
    for (auto &f : bank)
      f.clear();
  for (auto &d : delay)
    std::fill(d.begin(), d.end(), 0.f);
  for (auto &a : allpassIn)
    a.fill(0);
  for (auto &a : allpassOut)
    a.fill(0);
  phase = 0;
  writeIndex = counter = 0;
  envelope = fastEnvelope = slowEnvelope = 0;
  gainState = 1;
  reduction = 0;
  heldL = heldR = holdClock = 0;
  feedbackL = feedbackR = toneL = toneR = 0;
  dcInL = dcInR = dcOutL = dcOutR = sideLow = keyLowL = keyLowR = 0;
  holdSamples = 0;
  gateOpen = false;
  smooth = target;
  advancedSmooth=advanced;
  detectorLow.fill(0);inputLow.fill(0);repeatLow.fill(0);
  listenBlend=advanced.listenKey?1.f:0.f;
  monoBlend=advanced.monoListen?1.f:0.f;
  bondPaths = {};
  bondReduction.fill(0);
  bondModelBlend=static_cast<float>(advanced.bondModel);
  bondTopologyBlend=static_cast<float>(advanced.bondTopology);
  bondDetectorBlend=static_cast<float>(advanced.bondDetector);
  bondAutoBlend=advanced.bondAutoRelease?1.f:0.f;
  bondExternalBlend=0;
  mix = targetMix;
  output = targetOutput;
  updateFilters();
}
void SuiteEngine::setControls(const std::array<float, 6> &values, float wet,
                              float out) {
  for (const auto &p : products)
    if (p.kind == kind) {
      for (int i = 0; i < 6; ++i)
        target[i] =
            clamp(finite(values[i]), p.controls[i].min, p.controls[i].max);
      break;
    }
  if(hasTempoSync(kind)&&advanced.syncedValue>0)
    target[0]=clamp(advanced.syncedValue,kind>=Kind::Delay&&kind<=Kind::DubDelay?1.f:.01f,kind>=Kind::Delay&&kind<=Kind::DubDelay?12000.f:40.f);
  targetMix = clamp(finite(wet), 0, 1);
  targetOutput = db(clamp(finite(out), -60, 24));
}
float SuiteEngine::random() noexcept {
  noise ^= noise << 13;
  noise ^= noise >> 17;
  noise ^= noise << 5;
  return static_cast<float>(noise) * (1.f / 4294967296.f) - .5f;
}
void SuiteEngine::setAdvanced(const AdvancedSettings& a){
  for(int i=0;i<3;++i)advanced.bandQ[i]=clamp(finite(a.bandQ[i]),.1f,12.f);
  advanced.detectorCut=clamp(finite(a.detectorCut),0,1000);
  advanced.inputCut=clamp(finite(a.inputCut),0,1000);
  advanced.repeatCut=clamp(finite(a.repeatCut),0,1000);
  advanced.listenKey=a.listenKey&&hasDetector(kind);
  advanced.monoListen=a.monoListen;
  advanced.syncedValue=std::max(0.f,finite(a.syncedValue));
  advanced.bondModel=std::clamp(a.bondModel,0,1);
  advanced.bondTopology=std::clamp(a.bondTopology,0,1);
  advanced.bondDetector=std::clamp(a.bondDetector,0,1);
  advanced.bondLink=clamp(finite(a.bondLink),0,1);
  advanced.bondKnee=clamp(finite(a.bondKnee),0,24);
  advanced.bondRange=clamp(finite(a.bondRange),0,60);
  advanced.bondKeyLowpass=clamp(finite(a.bondKeyLowpass),200,20000);
  advanced.bondAutoRelease=a.bondAutoRelease;
}
std::array<float,4> SuiteEngine::processBondPrecision(float left, float right,
    float keyLeft, float keyRight, bool externalKey) noexcept {
  advancedSmooth.bondLink+=smoothing*(advanced.bondLink-advancedSmooth.bondLink);
  advancedSmooth.bondKnee+=smoothing*(advanced.bondKnee-advancedSmooth.bondKnee);
  advancedSmooth.bondRange+=smoothing*(advanced.bondRange-advancedSmooth.bondRange);
  advancedSmooth.bondKeyLowpass+=smoothing*(advanced.bondKeyLowpass-advancedSmooth.bondKeyLowpass);
  bondModelBlend+=smoothing*(static_cast<float>(advanced.bondModel)-bondModelBlend);
  bondTopologyBlend+=smoothing*(static_cast<float>(advanced.bondTopology)-bondTopologyBlend);
  bondDetectorBlend+=smoothing*(static_cast<float>(advanced.bondDetector)-bondDetectorBlend);
  bondAutoBlend+=smoothing*((advanced.bondAutoRelease?1.f:0.f)-bondAutoBlend);
  bondExternalBlend+=smoothing*((externalKey?1.f:0.f)-bondExternalBlend);
  // Avoid persistent tiny blends after a mode transition. In particular, a
  // settled Legacy selection must execute its exact original output path.
  if (std::abs(bondModelBlend-static_cast<float>(advanced.bondModel))<1.e-5f)
    bondModelBlend=static_cast<float>(advanced.bondModel);
  const float srF=static_cast<float>(sr);
  const float hpG=std::tan(static_cast<float>(pi)*clamp(smooth[4],1,srF*.45f)/srF);
  const float lpG=std::tan(static_cast<float>(pi)*clamp(advancedSmooth.bondKeyLowpass,1,srF*.45f)/srF);
  const float ratio=std::max(1.f,smooth[1]);
  const float knee=advancedSmooth.bondKnee;
  const float range=advancedSmooth.bondRange;
  const float input[2]{left,right}, key[2]{keyLeft,keyRight};
  float audition[2][2]{};
  for (int topology=0;topology<2;++topology) {
    auto& path=bondPaths[static_cast<std::size_t>(topology)];
    const bool feedback=topology==1&&!externalKey;
    float levels[2]{};
    for (int ch=0;ch<2;++ch) {
      // Internal feedback observes the gain stage before makeup, mix and
      // output trim. Its previous gain makes the loop causal without adding
      // latency to the audio. An external sidechain always uses feed-forward
      // detection: the external signal cannot observe our gain stage.
      const float source=clamp(feedback?input[ch]*path.gain[ch]:key[ch],-1.e6f,1.e6f);
      const float filtered=path.key[ch].tick(source,hpG,lpG);
      audition[topology][ch]=filtered;
      const float peak=std::abs(filtered);
      path.power[ch]+=bondPowerCoefficient*(peak*peak-path.power[ch]);
      if (path.power[ch]<1.e-20f) path.power[ch]=0;
      const float rms=std::sqrt(std::max(0.f,path.power[ch]));
      levels[ch]=peak+bondDetectorBlend*(rms-peak);
    }
    const float linked=std::max(levels[0],levels[1]);
    for (int ch=0;ch<2;++ch) {
      const float detector=levels[ch]+advancedSmooth.bondLink*(linked-levels[ch]);
      const float over=toDb(detector)-smooth[0];
      float above=std::max(0.f,over);
      if (knee>1.e-4f&&over>-knee*.5f&&over<knee*.5f)
        above=(over+knee*.5f)*(over+knee*.5f)/(2.f*knee);
      // The feedback law needs ratio-1 to reach the same steady-state ratio
      // as feed-forward. Scaling its integration time by ratio keeps that
      // negative-feedback loop stable, including fastest attack at 8 kHz.
      const float slope=feedback?ratio-1.f:1.f-1.f/ratio;
      const float requested=std::min(range,above*slope);
      path.sustained[ch]+=bondHistoryCoefficient*(requested-path.sustained[ch]);
      const float autoFactor=.35f+1.65f*clamp(path.sustained[ch]/12.f,0,1);
      const float release=smooth[3]*(1.f+bondAutoBlend*(autoFactor-1.f));
      const float milliseconds=requested>path.reductionDb[ch]?smooth[2]:release;
      const float timeScale=feedback?ratio:1.f;
      const float coefficient=1.f-std::exp(-1.f/(std::max(.1f,milliseconds)*.001f*srF*timeScale));
      path.reductionDb[ch]+=coefficient*(requested-path.reductionDb[ch]);
      path.reductionDb[ch]=clamp(path.reductionDb[ch],0,range);
      if (path.reductionDb[ch]<1.e-8f) path.reductionDb[ch]=0;
      path.gain[ch]=db(-path.reductionDb[ch]);
    }
  }
  const float topology=bondTopologyBlend*(1.f-bondExternalBlend);
  return {bondPaths[0].gain[0]+topology*(bondPaths[1].gain[0]-bondPaths[0].gain[0]),
          bondPaths[0].gain[1]+topology*(bondPaths[1].gain[1]-bondPaths[0].gain[1]),
          audition[0][0]+topology*(audition[1][0]-audition[0][0]),
          audition[0][1]+topology*(audition[1][1]-audition[0][1])};
}
void SuiteEngine::configure(Biquad &f, int type, float hz, float quality,
                            float gain) noexcept {
  const double w = 2 * pi * clamp(hz, 1, static_cast<float>(sr * .45)) / sr,
               c = std::cos(w), s = std::sin(w), q = std::max(.1f, quality),
               alpha = s / (2 * q), A = std::pow(10., gain / 40.),
               root = 2 * std::sqrt(A) * alpha;
  double a0 = 1, b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
  switch (type) {
  case 0:
    b0 = (1 - c) / 2;
    b1 = 1 - c;
    b2 = b0;
    a0 = 1 + alpha;
    a1 = -2 * c;
    a2 = 1 - alpha;
    break;
  case 1:
    b0 = (1 + c) / 2;
    b1 = -(1 + c);
    b2 = b0;
    a0 = 1 + alpha;
    a1 = -2 * c;
    a2 = 1 - alpha;
    break;
  case 2:
    b0 = alpha;
    b1 = 0;
    b2 = -alpha;
    a0 = 1 + alpha;
    a1 = -2 * c;
    a2 = 1 - alpha;
    break;
  case 3:
    b0 = 1;
    b1 = -2 * c;
    b2 = 1;
    a0 = 1 + alpha;
    a1 = -2 * c;
    a2 = 1 - alpha;
    break;
  case 4:
    b0 = 1 + alpha * A;
    b1 = -2 * c;
    b2 = 1 - alpha * A;
    a0 = 1 + alpha / A;
    a1 = -2 * c;
    a2 = 1 - alpha / A;
    break;
  case 5:
    b0 = A * ((A + 1) - (A - 1) * c + root);
    b1 = 2 * A * ((A - 1) - (A + 1) * c);
    b2 = A * ((A + 1) - (A - 1) * c - root);
    a0 = (A + 1) + (A - 1) * c + root;
    a1 = -2 * ((A - 1) + (A + 1) * c);
    a2 = (A + 1) + (A - 1) * c - root;
    break;
  case 6:
    b0 = A * ((A + 1) + (A - 1) * c + root);
    b1 = -2 * A * ((A - 1) + (A + 1) * c);
    b2 = A * ((A + 1) + (A - 1) * c - root);
    a0 = (A + 1) - (A - 1) * c + root;
    a1 = 2 * ((A - 1) - (A + 1) * c);
    a2 = (A + 1) - (A - 1) * c - root;
    break;
  }
  f.b0 = b0 / a0;
  f.b1 = b1 / a0;
  f.b2 = b2 / a0;
  f.a1 = a1 / a0;
  f.a2 = a2 / a0;
}
void SuiteEngine::updateFilters() noexcept {
  for (auto &bank : filters)
    switch (kind) {
    case Kind::DeEsser:
      configure(bank[0], 2, smooth[1], smooth[2]);
      break;
    case Kind::ParametricEQ:
      for (int i = 0; i < 3; ++i)
        configure(bank[i], 4, smooth[i * 2], advancedSmooth.bandQ[i], smooth[i * 2 + 1]);
      break;
    case Kind::TiltEQ:
      configure(bank[0], 5, smooth[0], advancedSmooth.bandQ[0], -smooth[1]);
      configure(bank[1], 6, smooth[0], advancedSmooth.bandQ[0], smooth[1]);
      break;
    case Kind::LowShelf:
      configure(bank[0], 5, smooth[0], smooth[2], smooth[1]);
      break;
    case Kind::HighShelf:
      configure(bank[0], 6, smooth[0], smooth[2], smooth[1]);
      break;
    case Kind::Notch:
      configure(bank[0], 3, smooth[0], smooth[1]);
      break;
    case Kind::BandPass:
      configure(bank[0], 2, smooth[0], smooth[1]);
      break;
    case Kind::Cuts:
      configure(bank[0], 1, smooth[0], smooth[2]);
      configure(bank[1], 0, std::max(smooth[1], smooth[0] * 1.05f), smooth[2]);
      break;
    default:
      break;
    }
  if (kind == Kind::MidSideEQ) {
    configure(filters[0][0], 4, smooth[0], smooth[3], smooth[1]);
    configure(filters[1][0], 4, smooth[0], smooth[3], smooth[2]);
  }
}
float SuiteEngine::readDelay(int channel, float ms) const noexcept {
  const auto &d = delay[static_cast<std::size_t>(channel)];
  double position = static_cast<double>(writeIndex) -
                    clamp(ms * static_cast<float>(sr) * .001f, 1.f,
                          static_cast<float>(d.size() - 3));
  if (position < 0)
    position += static_cast<double>(d.size());
  const auto i = static_cast<std::size_t>(position);
  const float frac = static_cast<float>(position - static_cast<double>(i));
  return d[i] * (1 - frac) + d[(i + 1) % d.size()] * frac;
}
void SuiteEngine::process(float *left, float *right, const float *keyLeft,
                          const float *keyRight, int n) noexcept {
  if (delay[0].empty() || !left || n <= 0)
    return;
  const bool stereo = right != nullptr;
  const float srF = static_cast<float>(sr);
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < 6; ++j)
      smooth[j] += smoothing * (target[j] - smooth[j]);
    mix += smoothing * (targetMix - mix);
    output += smoothing * (targetOutput - output);
    for(int j=0;j<3;++j)advancedSmooth.bandQ[j]+=smoothing*(advanced.bandQ[j]-advancedSmooth.bandQ[j]);
    advancedSmooth.detectorCut+=smoothing*(advanced.detectorCut-advancedSmooth.detectorCut);
    advancedSmooth.inputCut+=smoothing*(advanced.inputCut-advancedSmooth.inputCut);
    advancedSmooth.repeatCut+=smoothing*(advanced.repeatCut-advancedSmooth.repeatCut);
    listenBlend+=smoothing*((advanced.listenKey?1.f:0.f)-listenBlend);
    monoBlend+=smoothing*((advanced.monoListen?1.f:0.f)-monoBlend);
    if ((counter++ & 15u) == 0)
      updateFilters();
    float l = finite(left[i]), r = stereo ? finite(right[i]) : l;
    const float dryL = l, dryR = r;
    float kl = keyLeft ? finite(keyLeft[i]) : l,
          kr = keyRight ? finite(keyRight[i]) : r;
    auto cut=[&](float& a,float& b,std::array<float,2>& low,float hz){
      if(hz<.000001f)return;
      const float c=1-std::exp(-2.f*static_cast<float>(pi)*std::max(1.f,hz)/srF);
      low[0]+=c*(a-low[0]);low[1]+=c*(b-low[1]);
      const float blend=clamp(hz,0,1);a-=low[0]*blend;b-=low[1]*blend;
    };
    if(hasDetector(kind)&&kind!=Kind::BusCompressor)cut(kl,kr,detectorLow,advancedSmooth.detectorCut);
    if(hasColourFilter(kind))cut(l,r,inputLow,advancedSmooth.inputCut);
    switch (kind) {
    case Kind::Compressor:
    case Kind::FastCompressor:
    case Kind::RmsCompressor:
    case Kind::BusCompressor:
    case Kind::ParallelCompressor:
    case Kind::Ducker:
    case Kind::DeEsser: {
      std::array<float,4> precision{};
      if (kind == Kind::BusCompressor) {
        const bool externalKey=keyLeft!=nullptr||keyRight!=nullptr;
        const float precisionKeyL=keyLeft?finite(keyLeft[i]):keyRight?finite(keyRight[i]):l;
        const float precisionKeyR=keyRight?finite(keyRight[i]):keyLeft?finite(keyLeft[i]):r;
        precision=processBondPrecision(l,r,precisionKeyL,precisionKeyR,externalKey);
      }
      if (kind == Kind::BusCompressor) {
        float a = 1 - std::exp(-2.f * static_cast<float>(pi) * smooth[4] / srF);
        keyLowL += a * (kl - keyLowL);
        keyLowR += a * (kr - keyLowR);
        // Host input may be finite yet large enough to overflow the state
        // update. Recover instead of allowing a NaN key to poison audition
        // or the parallel model blend after normal audio resumes.
        if (!std::isfinite(keyLowL)) keyLowL = 0;
        if (!std::isfinite(keyLowR)) keyLowR = 0;
        kl -= keyLowL;
        kr -= keyLowR;
      }
      if (kind == Kind::DeEsser) {
        kl = filters[0][0].tick(kl);
        kr = filters[1][0].tick(kr);
      }
      float level = std::max(std::abs(kl), std::abs(kr));
      if (kind == Kind::FastCompressor) {
        fastEnvelope =
            std::max(level, fastEnvelope * std::exp(-1.f / (.0005f * srF)));
        level = fastEnvelope;
      }
      if (kind == Kind::RmsCompressor || kind == Kind::ParallelCompressor) {
        const float a =
            1 - std::exp(-1.f /
                         ((kind == Kind::RmsCompressor ? .025f : .003f) * srF));
        envelope += a * (level * level - envelope);
        level = std::sqrt(std::max(0.f, envelope));
      }
      const float over = toDb(level) - smooth[0],
                  ratioValue = kind == Kind::DeEsser ? 10.f : smooth[1];
      const float k = kind == Kind::BusCompressor || kind == Kind::Ducker ||
                              kind == Kind::DeEsser
                          ? 3.f
                          : smooth[4];
      float overSoft = over;
      if (k > 0) {
        if (over < -k * .5f)
          overSoft = 0;
        else if (over < k * .5f)
          overSoft = (over + k * .5f) * (over + k * .5f) / (2 * k);
      }
      float gr = std::max(0.f, overSoft) * (1 - 1 / std::max(1.f, ratioValue));
      if (kind == Kind::Ducker)
        gr = std::min(gr, smooth[4]);
      if (kind == Kind::DeEsser)
        gr = std::min(gr, smooth[4]);
      const float aMs = kind == Kind::DeEsser ? smooth[5] : smooth[2],
                  rMs =
                      smooth[3] * (kind == Kind::FastCompressor
                                       ? (.25f + .75f * clamp(gr / 18.f, 0, 1))
                                       : 1.f);
      const float desired = db(-gr),
                  coefficient = std::exp(
                      -1.f / (std::max(.1f, desired < gainState ? aMs : rMs) *
                              .001f * srF));
      gainState = desired + (gainState - desired) * coefficient;
      reduction = -toDb(gainState);
      const float m =
          kind == Kind::DeEsser || kind == Kind::Ducker ? 1.f : db(smooth[5]);
      l *= gainState * m;
      r *= gainState * m;
      if (kind == Kind::BusCompressor) {
        if (bondModelBlend>0) {
          const float gainL=gainState+bondModelBlend*(precision[0]-gainState);
          const float gainR=gainState+bondModelBlend*(precision[1]-gainState);
          l=dryL*(gainL*m);
          r=dryR*(gainR*m);
          kl+=bondModelBlend*(precision[2]-kl);
          kr+=bondModelBlend*(precision[3]-kr);
          bondReduction={-toDb(gainL),-toDb(gainR)};
          reduction=std::max(bondReduction[0],bondReduction[1]);
        } else {
          bondReduction={reduction,reduction};
        }
      }
      break;
    }
    case Kind::Gate: {
      float level = toDb(std::max(std::abs(kl), std::abs(kr)));
      if (level > smooth[0]) {
        gateOpen = true;
        holdSamples = static_cast<int>(smooth[5] * .001f * srF);
      } else if (holdSamples > 0)
        --holdSamples;
      else if (level < smooth[0] - smooth[4])
        gateOpen = false;
      float desired = gateOpen ? 1.f : db(smooth[1]);
      float c = std::exp(
          -1.f / (std::max(.1f, desired > gainState ? smooth[2] : smooth[3]) *
                  .001f * srF));
      gainState = desired + (gainState - desired) * c;
      l *= gainState;
      r *= gainState;
      reduction = -toDb(gainState);
      break;
    }
    case Kind::Expander: {
      float below =
          std::max(0.f, smooth[0] - toDb(std::max(std::abs(kl), std::abs(kr))));
      float desired = db(-std::min(smooth[4], below * (smooth[1] - 1)));
      float c = std::exp(
          -1.f / (std::max(.1f, desired > gainState ? smooth[2] : smooth[3]) *
                  .001f * srF));
      gainState = desired + (gainState - desired) * c;
      l *= gainState * db(smooth[5]);
      r *= gainState * db(smooth[5]);
      reduction = -toDb(gainState);
      break;
    }
    case Kind::Transient:
    case Kind::Sustain: {
      float level = std::max(std::abs(l), std::abs(r));
      float f = 1 - std::exp(-1.f / (smooth[2] * .001f * srF)),
            s = 1 - std::exp(-1.f / (smooth[3] * .001f * srF));
      fastEnvelope += f * (level - fastEnvelope);
      slowEnvelope += s * (level - slowEnvelope);
      float delta =
          (fastEnvelope - slowEnvelope) / std::max(.01f, slowEnvelope);
      float attackWeight = clamp(delta * smooth[4], 0, 1),
            body = clamp(-delta * smooth[4], 0, 1);
      float g = kind == Kind::Transient
                    ? db((smooth[0] * attackWeight + smooth[1] * body) * .12f)
                    : db(smooth[0] * .12f * body *
                         (1 - smooth[1] * .01f * attackWeight));
      l *= g * db(smooth[5]);
      r *= g * db(smooth[5]);
      break;
    }
    case Kind::Limiter: {
      l *= db(smooth[1]);
      r *= db(smooth[1]);
      float limit = db(smooth[0]), peak = std::max(std::abs(l), std::abs(r)),
            desired = std::min(1.f, limit / std::max(1.e-9f, peak));
      float c = std::exp(-1.f / (smooth[2] * .001f * srF));
      gainState =
          desired < gainState ? desired : desired + (gainState - desired) * c;
      l *= gainState;
      r *= gainState;
      reduction = -toDb(gainState);
      break;
    }
    case Kind::Clipper: {
      float ceiling = db(smooth[0]), input = db(smooth[1]),
            soft = smooth[2] * .01f;
      auto clip = [&](float x) {
        x *= input / ceiling;
        float hard = clamp(x, -1, 1), gentle = std::tanh(x);
        return ceiling * (hard * (1 - soft) + gentle * soft);
      };
      l = clip(l);
      r = clip(r);
      break;
    }
    case Kind::Leveller: {
      float level = std::max(std::abs(l), std::abs(r));
      float c = 1 - std::exp(-1.f / (.1f * srF));
      envelope += c * (level * level - envelope);
      float levelDb = toDb(std::sqrt(std::max(0.f, envelope)));
      float desired = db(levelDb < smooth[4] ? 0.f
                                             : clamp(smooth[0] - levelDb,
                                                     -smooth[1], smooth[1]));
      float g = std::exp(
          -1.f / (std::max(1.f, desired < gainState ? smooth[2] : smooth[3]) *
                  .001f * srF));
      gainState = desired + (gainState - desired) * g;
      l *= gainState;
      r *= gainState;
      break;
    }
    case Kind::ParametricEQ:
      for (int j = 0; j < 3; ++j) {
        l = filters[0][j].tick(l);
        r = filters[1][j].tick(r);
      }
      break;
    case Kind::TiltEQ:
    case Kind::Cuts:
      for (int j = 0; j < 2; ++j) {
        l = filters[0][j].tick(l);
        r = filters[1][j].tick(r);
      }
      break;
    case Kind::LowShelf:
    case Kind::HighShelf:
    case Kind::Notch:
    case Kind::BandPass:
      l = filters[0][0].tick(l);
      r = filters[1][0].tick(r);
      break;
    case Kind::MidSideEQ: {
      float m = filters[0][0].tick((l + r) * .5f),
            s = filters[1][0].tick((l - r) * .5f);
      l = m + s;
      r = m - s;
      break;
    }
    case Kind::SoftSaturation:
    case Kind::AsymmetricSaturation:
    case Kind::TubeSaturation:
    case Kind::Wavefolder:
    case Kind::Rectifier:
    case Kind::BitCrusher:
    case Kind::RateReducer: {
      auto shape = [&](float x) {
        const float d = db(smooth[0]);
        switch (kind) {
        case Kind::SoftSaturation:
          return std::tanh(x * d) / std::max(1.f, std::sqrt(d));
        case Kind::AsymmetricSaturation: {
          const float b = smooth[1];
          return (std::tanh(x * d + b) - std::tanh(b)) /
                 std::max(1.f, std::sqrt(d));
        }
        case Kind::TubeSaturation: {
          const float a = smooth[1] * .01f;
          return std::tanh(std::tanh(x * d) * (1 + 2 * a)) /
                 std::max(1.f, std::sqrt(d));
        }
        case Kind::Wavefolder: {
          float a = x * d + smooth[1];
          float folded = (2.f / static_cast<float>(pi)) *
                         std::asin(std::sin(a * static_cast<float>(pi) * .5f));
          float bias =
              (2.f / static_cast<float>(pi)) *
              std::asin(std::sin(smooth[1] * static_cast<float>(pi) * .5f));
          return folded - bias;
        }
        case Kind::Rectifier:
          return x * (1 - smooth[0] * .01f) +
                 (std::abs(x + smooth[1]) - smooth[1]) * smooth[0] * .01f;
        case Kind::BitCrusher: {
          float steps = std::pow(2.f, std::round(smooth[0]) - 1);
          return std::round((x + random() * smooth[1] * .01f / steps) * steps) /
                 steps;
        }
        default:
          return x;
        }
      };
      if (kind == Kind::RateReducer) {
        holdClock += std::min(srF, smooth[0]) / srF;
        if (holdClock >= 1) {
          holdClock -= 1;
          heldL = l;
          heldR = r;
        }
        l = heldL;
        r = heldR;
      } else {
        l = shape(l);
        r = shape(r);
      }
      float hz = kind == Kind::SoftSaturation ? smooth[1]
                 : kind == Kind::RateReducer  ? smooth[1]
                                              : smooth[2];
      const float a = 1 - std::exp(-2.f * static_cast<float>(pi) *
                                   clamp(hz, 20, srF * .45f) / srF);
      toneL += a * (l - toneL);
      toneR += a * (r - toneR);
      l = toneL;
      r = toneR;
      const float c = std::exp(-2.f * static_cast<float>(pi) * 5.f / srF);
      float yl = l - dcInL + c * dcOutL, yr = r - dcInR + c * dcOutR;
      dcInL = l;
      dcInR = r;
      dcOutL = yl;
      dcOutR = yr;
      l = yl;
      r = yr;
      break;
    }
    case Kind::Delay:
    case Kind::TapeDelay:
    case Kind::PingPong:
    case Kind::SlapDelay:
    case Kind::DubDelay:
    case Kind::Comb: {
      phase += 2 * pi * .27 / sr;
      float move =
          kind == Kind::TapeDelay ? smooth[3] * .02f : smooth[3] * .005f;
      float dl = readDelay(0, smooth[0] +
                                  move * static_cast<float>(std::sin(phase))),
            dr = readDelay(
                1, smooth[0] * (kind == Kind::SlapDelay ? 1.12f : 1.f) +
                       move * static_cast<float>(std::sin(phase + 1.3)));
      float a = 1 - std::exp(-2.f * static_cast<float>(pi) *
                             clamp(smooth[2], 20, srF * .45f) / srF);
      toneL += a * (dl - toneL);
      toneR += a * (dr - toneR);
      float repeatL=toneL,repeatR=toneR;
      cut(repeatL,repeatR,repeatLow,advancedSmooth.repeatCut);
      float fb = smooth[1] * .01f;
      float nextL = l + fb * (kind == Kind::PingPong ? repeatR : repeatL),
            nextR = r + fb * (kind == Kind::PingPong ? repeatL : repeatR);
      if (kind == Kind::TapeDelay || kind == Kind::DubDelay) {
        const float drive = kind == Kind::DubDelay ? 1.8f : 1.f;
        nextL = std::tanh(nextL * drive) / drive;
        nextR = std::tanh(nextR * drive) / drive;
      }
      delay[0][writeIndex] = clamp(nextL, -8, 8);
      delay[1][writeIndex] = clamp(nextR, -8, 8);
      l = repeatL;
      r = repeatR;
      break;
    }
    case Kind::Chorus:
    case Kind::Flanger:
    case Kind::Vibrato: {
      phase += 2 * pi * smooth[0] / sr;
      float depth =
          smooth[1] * .01f *
          std::min(smooth[2] * .8f, kind == Kind::Flanger ? 5.f : 10.f);
      float dl = readDelay(0, smooth[2] +
                                  depth * static_cast<float>(std::sin(phase))),
            dr = readDelay(1, smooth[2] + depth * static_cast<float>(std::sin(
                                                      phase + pi * .5)));
      delay[0][writeIndex] = clamp(l + dl * smooth[3] * .01f, -8, 8);
      delay[1][writeIndex] = clamp(r + dr * smooth[3] * .01f, -8, 8);
      l = dl;
      r = dr;
      break;
    }
    case Kind::Phaser: {
      phase += 2 * pi * smooth[0] / sr;
      float mod = std::pow(2.f, 2 * smooth[1] * .01f *
                                    static_cast<float>(std::sin(phase)));
      float hz = clamp(smooth[2] * mod, 20, srF * .4f),
            t = std::tan(static_cast<float>(pi) * hz / srF),
            a = (1 - t) / (1 + t);
      float x[2] = {l + feedbackL * smooth[3] * .01f,
                    r + feedbackR * smooth[3] * .01f};
      for (int ch = 0; ch < 2; ++ch)
        for (int j = 0; j < 6; ++j) {
          float y = -a * x[ch] + allpassIn[ch][j] + a * allpassOut[ch][j];
          allpassIn[ch][j] = x[ch];
          allpassOut[ch][j] = y;
          x[ch] = y;
        }
      feedbackL = clamp(x[0], -8, 8);
      feedbackR = clamp(x[1], -8, 8);
      l = x[0];
      r = x[1];
      break;
    }
    case Kind::Tremolo:
    case Kind::AutoPan:
    case Kind::RingMod: {
      phase += 2 * pi * smooth[0] / sr;
      float wave = static_cast<float>(std::sin(phase));
      if (kind != Kind::RingMod) {
        float shape = smooth[2] * .01f;
        wave = wave * (1 - shape) + std::tanh(wave * 12) * shape;
      }
      float depth = smooth[1] * .01f;
      if (kind == Kind::Tremolo) {
        float g = 1 - depth * (.5f + .5f * wave);
        l *= g;
        r *= g;
      } else if (kind == Kind::AutoPan) {
        l *= 1 - std::max(0.f, wave * depth);
        r *= 1 + std::min(0.f, wave * depth);
      } else {
        l *= 1 - depth + depth * wave;
        r *= 1 - depth + depth * wave;
      }
      break;
    }
    case Kind::Width:
    case Kind::MonoBass: {
      float m = (l + r) * .5f, s = (l - r) * .5f;
      float hz = kind == Kind::Width ? smooth[1] : smooth[0];
      float a = 1 - std::exp(-2.f * static_cast<float>(pi) * hz / srF);
      sideLow += a * (s - sideLow);
      s = kind == Kind::Width ? (s - sideLow) * smooth[0] * .01f + sideLow
                              : s - sideLow * smooth[1] * .01f;
      l = m + s;
      r = m - s;
      break;
    }
    case Kind::Haas: {
      float dl =
                smooth[0] < .001f ? l : readDelay(0, std::max(.02f, smooth[0])),
            dr =
                smooth[0] < .001f ? r : readDelay(1, std::max(.02f, smooth[0]));
      delay[0][writeIndex] = l;
      delay[1][writeIndex] = r;
      if (smooth[1] < 0)
        l = l * (1 + smooth[1] * .01f) - dl * smooth[1] * .01f;
      else
        r = r * (1 - smooth[1] * .01f) + dr * smooth[1] * .01f;
      break;
    }
    case Kind::Gain: {
      float g = db(smooth[0]), b = smooth[1] * .01f;
      l *= g * (1 - std::max(0.f, b));
      r *= g * (1 + std::min(0.f, b));
      break;
    }
    case Kind::Polarity:
      l *= 1 - 2 * clamp(smooth[0], 0, 1);
      r *= 1 - 2 * clamp(smooth[1], 0, 1);
      break;
    case Kind::DCBlock: {
      float c = std::exp(-2.f * static_cast<float>(pi) * smooth[0] / srF);
      float yl = l - dcInL + c * dcOutL, yr = r - dcInR + c * dcOutR;
      dcInL = l;
      dcInR = r;
      dcOutL = yl;
      dcOutR = yr;
      l = yl;
      r = yr;
      break;
    }
    default:
      break;
    }
    if (phase >= 2 * pi)
      phase = std::fmod(phase, 2 * pi);
    if (usesDelay())
      writeIndex = (writeIndex + 1) % delay[0].size();
    float mixedL=dryL+(l-dryL)*mix,mixedR=dryR+(r-dryR)*mix;
    float outL = finite((mixedL+(kl-mixedL)*listenBlend) * output),
          outR = finite((mixedR+(kr-mixedR)*listenBlend) * output);
    const float mono=outL*.5f+outR*.5f;outL+=monoBlend*(mono-outL);outR+=monoBlend*(mono-outR);
    left[i] = outL;
    if (stereo)
      right[i] = outR;
  }
}
} // namespace hungryghost
