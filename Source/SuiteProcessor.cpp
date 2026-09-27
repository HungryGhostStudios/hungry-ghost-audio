#include "SuiteProcessor.h"
#include "SuiteEditor.h"
namespace hungryghost {
namespace {
float initialMix(const Product &p) {
  return std::string_view(p.family) == "Space" ? .3f
         : p.kind == Kind::ParallelCompressor  ? .5f
         : p.kind == Kind::Chorus || p.kind == Kind::Flanger ||
                 p.kind == Kind::Phaser
             ? .5f
             : 1.f;
}
} // namespace
juce::AudioProcessorValueTreeState::ParameterLayout
SuiteProcessor::layout(const Product &p) {
  juce::AudioProcessorValueTreeState::ParameterLayout result;
  for (int i = 0; i < p.controlCount; ++i) {
    const auto &c = p.controls[i];
    juce::NormalisableRange<float> range(
        c.min, c.max, p.kind == Kind::Polarity ? 1.f : 0.f, c.skew);
    result.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("control" + juce::String(i), 1), c.name, range,
        c.initial, juce::AudioParameterFloatAttributes().withLabel(c.unit)));
  }
  result.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("mix", 1), "Mix", juce::NormalisableRange<float>(0, 1),
      initialMix(p)));
  result.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("output", 1), "Output",
      juce::NormalisableRange<float>(-24, 24), 0,
      juce::AudioParameterFloatAttributes().withLabel("dB")));
  result.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("bypass", 1), "Bypass", false));
  return result;
}
SuiteProcessor::SuiteProcessor(int index)
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withInput("External key", juce::AudioChannelSet::stereo(),
                         false)),
      product(products.at(static_cast<std::size_t>(index))),
      licence(product.id),
      state(*this, &undo, juce::Identifier("HG_" + juce::String(product.id)),
            layout(product)),
      engine(product.kind) {
  for (int i = 0; i < product.controlCount; ++i)
    controls[i] = state.getRawParameterValue("control" + juce::String(i));
  wet = state.getRawParameterValue("mix");
  out = state.getRawParameterValue("output");
  bypass = state.getRawParameterValue("bypass");
  banks[0] = state.copyState();
  banks[1] = banks[0].createCopy();
}
bool SuiteProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  auto main = b.getMainInputChannelSet();
  if (main != b.getMainOutputChannelSet() ||
      (main != juce::AudioChannelSet::mono() &&
       main != juce::AudioChannelSet::stereo()))
    return false;
  auto key = b.getChannelSet(true, 1);
  return key.isDisabled() || key == juce::AudioChannelSet::mono() ||
         key == juce::AudioChannelSet::stereo();
}
bool SuiteProcessor::isNonlinear() const {
  return product.kind == Kind::Clipper ||
         product.kind == Kind::SoftSaturation ||
         product.kind == Kind::AsymmetricSaturation ||
         product.kind == Kind::TubeSaturation ||
         product.kind == Kind::Wavefolder || product.kind == Kind::Rectifier;
}
void SuiteProcessor::prepareToPlay(double sr, int) {
  oversampling.reset();
  latency = 0;
  if (isNonlinear()) {
    const std::size_t power = sr <= 96000 ? 2 : sr <= 192000 ? 1 : 0;
    if (power > 0) {
      oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
          static_cast<std::size_t>(getMainBusNumOutputChannels()), power,
          juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true,
          true);
      oversampling->initProcessing(analysisSize);
      latency = juce::roundToInt(oversampling->getLatencyInSamples());
    }
  }
  setLatencySamples(latency);
  engine.prepare(sr * (oversampling ? static_cast<double>(
                                          oversampling->getOversamplingFactor())
                                    : 1.));
  blendStep = static_cast<float>(1. - std::exp(-1. / (.003 * sr)));
  reset();
  fifoIndex = 0;
}
void SuiteProcessor::reset() {
  engine.reset();
  if (oversampling)
    oversampling->reset();
  for (auto &channel : bypassDelay)
    channel.fill(0);
  bypassWrite = 0;
  processingBlend = bypass && bypass->load() > .5f ? 0.f
                    : licence.canProcess()         ? 1.f
                                                   : 0.f;
}
double SuiteProcessor::getTailLengthSeconds() const {
  if (product.kind >= Kind::Delay && product.kind <= Kind::Comb) {
    const double time = static_cast<double>(controls[0]->load()) * .001;
    const double feedback =
        std::min(.95, std::abs(static_cast<double>(controls[1]->load()) * .01));
    return time *
           (feedback > 0 ? std::max(1., std::log(.001) / std::log(feedback))
                         : 1.) *
           (product.kind == Kind::SlapDelay ? 1.12 : 1.);
  }
  if (product.kind == Kind::Chorus || product.kind == Kind::Flanger ||
      product.kind == Kind::Vibrato || product.kind == Kind::Haas)
    return .1;
  return 0.;
}
juce::AudioProcessorParameter *SuiteProcessor::getBypassParameter() const {
  return state.getParameter("bypass");
}
void SuiteProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                  juce::MidiBuffer &) {
  run(buffer, false);
}
void SuiteProcessor::run(juce::AudioBuffer<float> &buffer, bool hostBypass) {
  juce::ScopedNoDenormals noDenormals;
  auto audio = getBusBuffer(buffer, false, 0);
  const int n = audio.getNumSamples();
  if (n == 0 || audio.getNumChannels() == 0)
    return;
  float peakIn = 0;
  for (int i = 0; i < n; ++i) {
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
      float x = audio.getSample(ch, i);
      peakIn = std::max(peakIn, std::isfinite(x) ? std::abs(x) : 0.f);
    }
  }
  // Capture input in a fixed stack buffer, in bounded chunks, before in-place
  // processing.
  std::array<float, analysisSize> input{};
  std::array<std::array<float, analysisSize>, 2> delayedDry{};
  std::array<float, 6> values{};
  for (int j = 0; j < product.controlCount; ++j)
    values[j] = controls[j]->load();
  engine.setControls(values, wet->load(), out->load());
  auto key = getBusBuffer(buffer, true, 1);
  const float *keyL =
      key.getNumChannels() > 0 ? key.getReadPointer(0) : nullptr;
  const float *keyR = key.getNumChannels() > 1 ? key.getReadPointer(1) : keyL;
  float peakOut = 0;
  const bool processing =
      !hostBypass && bypass->load() < .5f && licence.canProcess();
  for (int offset = 0; offset < n; offset += analysisSize) {
    const int count = std::min(analysisSize, n - offset);
    for (int i = 0; i < count; ++i) {
      float sum = 0;
      for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float x = audio.getSample(ch, offset + i);
        if (!std::isfinite(x)) {
          x = 0.f;
          audio.setSample(ch, offset + i, x);
        }
        sum += std::isfinite(x) ? x : 0.f;
      }
      input[i] = sum / static_cast<float>(audio.getNumChannels());
      for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        const float x = audio.getSample(ch, offset + i);
        bypassDelay[ch][bypassWrite] = std::isfinite(x) ? x : 0.f;
        delayedDry[ch][i] =
            bypassDelay[ch][(bypassWrite + 256 - latency) % 256];
      }
      bypassWrite = (bypassWrite + 1) % 256;
    }
    if (oversampling) {
      auto block = juce::dsp::AudioBlock<float>(audio).getSubBlock(
          static_cast<std::size_t>(offset), static_cast<std::size_t>(count));
      auto up = oversampling->processSamplesUp(block);
      engine.process(up.getChannelPointer(0),
                     up.getNumChannels() > 1 ? up.getChannelPointer(1)
                                             : nullptr,
                     nullptr, nullptr, static_cast<int>(up.getNumSamples()));
      oversampling->processSamplesDown(block);
    } else
      engine.process(audio.getWritePointer(0) + offset,
                     audio.getNumChannels() > 1
                         ? audio.getWritePointer(1) + offset
                         : nullptr,
                     keyL ? keyL + offset : nullptr,
                     keyR ? keyR + offset : nullptr, count);
    for (int i = 0; i < count; ++i) {
      processingBlend +=
          blendStep * ((processing ? 1.f : 0.f) - processingBlend);
      float sum = 0;
      for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float x = audio.getSample(ch, offset + i);
        x = delayedDry[ch][i] + processingBlend * (x - delayedDry[ch][i]);
        audio.setSample(ch, offset + i, x);
        sum += x;
        peakOut = std::max(peakOut, std::abs(x));
      }
      preFifo[fifoIndex] = input[i];
      postFifo[fifoIndex] = sum / static_cast<float>(audio.getNumChannels());
      if (++fifoIndex == analysisSize) {
        fifoIndex = 0;
        if (!frameReady.load(std::memory_order_acquire)) {
          preFrame = preFifo;
          postFrame = postFifo;
          frameReady.store(true, std::memory_order_release);
        }
      }
    }
  }
  inputPeak.store(peakIn);
  outputPeak.store(peakOut);
  reduction.store(processing ? engine.gainReduction() : 0);
}
void SuiteProcessor::processBlockBypassed(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &) {
  run(buffer, true);
}
bool SuiteProcessor::popAnalysis(std::array<float, analysisSize> &pre,
                                 std::array<float, analysisSize> &post) {
  if (!frameReady.load(std::memory_order_acquire))
    return false;
  pre = preFrame;
  post = postFrame;
  frameReady.store(false, std::memory_order_release);
  return true;
}
juce::AudioProcessorEditor *SuiteProcessor::createEditor() {
  return new SuiteEditor(*this);
}
void SuiteProcessor::getStateInformation(juce::MemoryBlock &dest) {
  juce::ScopedLock lock(bankLock);
  banks[bank.load()] = state.copyState();
  juce::ValueTree root("HG_SUITE_STATE");
  root.setProperty("product", product.id, nullptr);
  root.setProperty("version", 1, nullptr);
  root.setProperty("bank", bank.load(), nullptr);
  for (auto &b : banks)
    root.addChild(b.createCopy(), -1, nullptr);
  auto xml = root.createXml();
  copyXmlToBinary(*xml, dest);
}
void SuiteProcessor::setStateInformation(const void *data, int size) {
  auto xml = getXmlFromBinary(data, size);
  if (!xml || !xml->hasTagName("HG_SUITE_STATE"))
    return;
  auto root = juce::ValueTree::fromXml(*xml);
  if (root["product"].toString() != product.id || root.getNumChildren() != 2)
    return;
  for (int i = 0; i < 2; ++i)
    if (!root.getChild(i).hasType(state.state.getType()))
      return;
  juce::ScopedLock lock(bankLock);
  for (int i = 0; i < 2; ++i)
    banks[i] = root.getChild(i).createCopy();
  bank.store(juce::jlimit(0, 1, static_cast<int>(root["bank"])));
  state.replaceState(banks[bank.load()].createCopy());
}
void SuiteProcessor::selectBank(int index) {
  juce::ScopedLock lock(bankLock);
  index = juce::jlimit(0, 1, index);
  if (bank.load() == index)
    return;
  banks[bank.load()] = state.copyState();
  bank.store(index);
  state.replaceState(banks[index].createCopy());
}
void SuiteProcessor::copyBank() {
  juce::ScopedLock lock(bankLock);
  banks[1 - bank.load()] = state.copyState();
}
void SuiteProcessor::factoryReset() {
  undo.beginNewTransaction("Factory reset");
  for (auto *p : getParameters()) {
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->getDefaultValue());
    p->endChangeGesture();
  }
}
} // namespace hungryghost
