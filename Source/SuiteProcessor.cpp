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
    juce::NormalisableRange<float> range(c.min, c.max, 0, c.skew);
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
void SuiteProcessor::prepareToPlay(double sr, int) {
  engine.prepare(sr);
  fifoIndex = 0;
}
double SuiteProcessor::getTailLengthSeconds() const {
  if (product.kind >= Kind::Delay && product.kind <= Kind::Comb)
    return 60.;
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
  juce::ScopedNoDenormals noDenormals;
  auto audio = getBusBuffer(buffer, false, 0);
  const int n = audio.getNumSamples();
  if (n == 0)
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
  std::array<float, 6> values{};
  for (int j = 0; j < product.controlCount; ++j)
    values[j] = controls[j]->load();
  engine.setControls(values, wet->load(), out->load());
  auto key = getBusBuffer(buffer, true, 1);
  const float *keyL =
      key.getNumChannels() > 0 ? key.getReadPointer(0) : nullptr;
  const float *keyR = key.getNumChannels() > 1 ? key.getReadPointer(1) : keyL;
  float peakOut = 0;
  for (int offset = 0; offset < n; offset += analysisSize) {
    const int count = std::min(analysisSize, n - offset);
    for (int i = 0; i < count; ++i) {
      float sum = 0;
      for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float x = audio.getSample(ch, offset + i);
        sum += std::isfinite(x) ? x : 0.f;
      }
      input[i] = sum / static_cast<float>(audio.getNumChannels());
    }
    if (bypass->load() < .5f)
      engine.process(audio.getWritePointer(0) + offset,
                     audio.getNumChannels() > 1
                         ? audio.getWritePointer(1) + offset
                         : nullptr,
                     keyL ? keyL + offset : nullptr,
                     keyR ? keyR + offset : nullptr, count);
    for (int i = 0; i < count; ++i) {
      float sum = 0;
      for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float x = audio.getSample(ch, offset + i);
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
  reduction.store(bypass->load() < .5f ? engine.gainReduction() : 0);
}
void SuiteProcessor::processBlockBypassed(juce::AudioBuffer<float> &,
                                          juce::MidiBuffer &) {
  reduction.store(0);
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
