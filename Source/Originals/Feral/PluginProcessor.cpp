#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
struct Spec {
  const char *key;
  float feral::Band::*member;
  float lo, hi, step, initial, centre;
};
constexpr std::array<Spec, 15> specs{
    {{"enabled", &feral::Band::enabled, 0, 1, 1, 0, 0},
     {"frequency", &feral::Band::frequency, 20, 20000, .01f, 1000, 1000},
     {"gain", &feral::Band::gain, -18, 18, .01f, 0, 0},
     {"q", &feral::Band::q, .1f, 18, .01f, 1, 1},
     {"threshold", &feral::Band::threshold, -60, 0, .1f, -24, 0},
     {"ratio", &feral::Band::ratio, 1, 20, .01f, 3, 3},
     {"attack", &feral::Band::attack, .1f, 200, .01f, 10, 20},
     {"release", &feral::Band::release, 10, 2000, .1f, 150, 200},
     {"knee", &feral::Band::knee, 0, 24, .1f, 6, 0},
     {"range", &feral::Band::range, 0, 24, .1f, 6, 0},
     {"dynamic", &feral::Band::dynamic, 0, 1, 1, 1, 0},
     {"shape", &feral::Band::shape, 0, 4, 1, 0, 0},
     {"channel", &feral::Band::channel, 0, 4, 1, 0, 0},
     {"external", &feral::Band::external, 0, 1, 1, 0, 0},
     // Reserved for future detector mix, kept out of processing for now.
     {"reserved", nullptr, 0, 1, 1, 0, 0}}};
struct GlobalSpec {
  const char *key;
  float feral::Parameters::*member;
  float lo, hi, step, initial, centre;
};
constexpr std::array<GlobalSpec, 15> globalSpecs{
    {{"busEnabled", &feral::Parameters::busEnabled, 0, 1, 1, 0, 0},
     {"threshold", &feral::Parameters::threshold, -60, 0, .1f, -18, 0},
     {"ratio", &feral::Parameters::ratio, 1, 20, .01f, 2, 3},
     {"attack", &feral::Parameters::attack, .1f, 200, .01f, 30, 20},
     {"release", &feral::Parameters::release, 10, 2000, .1f, 150, 200},
     {"knee", &feral::Parameters::knee, 0, 24, .1f, 6, 0},
     {"range", &feral::Parameters::range, 0, 36, .1f, 18, 0},
     {"input", &feral::Parameters::input, -24, 24, .1f, 0, 0},
     {"output", &feral::Parameters::output, -24, 24, .1f, 0, 0},
     {"mix", &feral::Parameters::mix, 0, 100, .1f, 100, 0},
     {"external", &feral::Parameters::external, 0, 1, 1, 0, 0},
     {"detectorHP", &feral::Parameters::detectorHP, 20, 1000, 1, 20, 120},
     {"bypass", &feral::Parameters::bypass, 0, 1, 1, 0, 0},
     {"reserved1", nullptr, 0, 1, 1, 0, 0},
     {"reserved2", nullptr, 0, 1, 1, 0, 0}}};
} // namespace
juce::AudioProcessorValueTreeState::ParameterLayout FeralProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout result;
  auto add = [&](const juce::String &id, const juce::String &name, float lo,
                 float hi, float step, float initial, float centre) {
    juce::NormalisableRange<float> range(lo, hi, step);
    if (centre > lo && centre < hi)
      range.setSkewForCentre(centre);
    result.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id, 1}, name, range, initial));
  };
  for (int i = 0; i < feral::maxBands; ++i)
    for (const auto &s : specs)
      if (s.member)
        add(bandID(i, s.key), "Band " + juce::String(i + 1) + " " + s.key, s.lo,
            s.hi, s.step, s.initial, s.centre);
  for (const auto &s : globalSpecs) {
    if (!s.member)
      continue;
    if (juce::String(s.key) == "bypass")
      result.add(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{"bypass", 1}, "Bypass", false));
    else
      add(s.key, juce::String(s.key).replaceCharacter('_', ' '), s.lo, s.hi,
          s.step, s.initial, s.centre);
  }
  return result;
}
FeralProcessor::FeralProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      state(*this, &undo, "FERAL_PARAMETERS", layout()) {
  for (int i = 0; i < feral::maxBands; ++i)
    for (size_t j = 0; j < specs.size(); ++j)
      bindings[i][j] = {
          specs[j].member ? state.getRawParameterValue(bandID(i, specs[j].key))
                          : nullptr,
          specs[j].member};
  for (size_t j = 0; j < globalSpecs.size(); ++j)
    globals[j] = {globalSpecs[j].member
                      ? state.getRawParameterValue(globalSpecs[j].key)
                      : nullptr,
                  globalSpecs[j].member};
  setCurrentProgram(1);
  banks[0] = state.copyState();
  banks[1] = banks[0].createCopy();
  undo.clearUndoHistory();
}
void FeralProcessor::prepareToPlay(double sr, int) {
  engine.prepare(sr, readParameters());
  setLatencySamples(0);
  framePosition = hopPosition = filledSamples = 0;
  // Only the GUI acknowledges a published frame. Clearing frameReady here
  // could let the producer overwrite it while an open editor is copying.
}
double FeralProcessor::getTailLengthSeconds() const {
  double seconds = .1;
  for (const auto &b : readParameters().bands)
    if (b.enabled > .5f)
      seconds = std::max(seconds, 7.0 * b.q /
                                      (juce::MathConstants<double>::pi *
                                       std::max(20.0f, b.frequency)));
  return seconds;
}
bool FeralProcessor::isBusesLayoutSupported(const BusesLayout &buses) const {
  const auto in = buses.getMainInputChannelSet(),
             out = buses.getMainOutputChannelSet();
  if (in != out || (out != juce::AudioChannelSet::mono() &&
                    out != juce::AudioChannelSet::stereo()))
    return false;
  if (buses.inputBuses.size() > 1) {
    const auto sc = buses.inputBuses[1];
    if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono() &&
        sc != juce::AudioChannelSet::stereo())
      return false;
  }
  return true;
}
feral::Parameters FeralProcessor::readParameters() const noexcept {
  feral::Parameters p;
  for (int i = 0; i < feral::maxBands; ++i)
    for (const auto &b : bindings[i])
      if (b.source)
        p.bands[i].*(b.member) = b.source->load(std::memory_order_relaxed);
  for (const auto &b : globals)
    if (b.source)
      p.*(b.member) = b.source->load(std::memory_order_relaxed);
  return p;
}
float FeralProcessor::value(const juce::String &id) const {
  return state.getRawParameterValue(id)->load();
}
void FeralProcessor::setValue(const juce::String &id, float v) {
  if (auto *parameter = state.getParameter(id))
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}
void FeralProcessor::run(juce::AudioBuffer<float> &buffer, bool bypassed) {
  juce::ScopedNoDenormals noDenormals;
  auto main = getBusBuffer(buffer, false, 0);
  const int channels = main.getNumChannels();
  if (channels == 0)
    return;
  auto p = readParameters();
  if (bypassed || !licence.canProcess())
    p.bypass = 1;
  const float *scL = nullptr;
  const float *scR = nullptr;
  if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
    auto sc = getBusBuffer(buffer, true, 1);
    if (sc.getNumChannels() > 0)
      scL = sc.getReadPointer(0);
    if (sc.getNumChannels() > 1)
      scR = sc.getReadPointer(1);
  }
  for (int offset = 0; offset < main.getNumSamples();) {
    const int count = std::min(analysisSize, main.getNumSamples() - offset);
    std::copy_n(main.getReadPointer(0) + offset, count, inputScratch.data());
    auto *l = main.getWritePointer(0) + offset;
    auto *r = channels > 1 ? main.getWritePointer(1) + offset : nullptr;
    engine.process(l, r, count, scL ? scL + offset : nullptr,
                   scR ? scR + offset : nullptr, p);
    for (int n = 0; n < count; ++n) {
      preFrame[framePosition] = inputScratch[n];
      postFrame[framePosition] = l[n];
      framePosition = (framePosition + 1) % analysisSize;
      filledSamples = std::min(analysisSize, filledSamples + 1);
      if (++hopPosition == analysisHop) {
        if (filledSamples == analysisSize &&
            !frameReady.load(std::memory_order_acquire)) {
          // Publish the latest overlapping window in chronological order.
          const int tail = analysisSize - framePosition;
          std::copy_n(preFrame.data() + framePosition, tail,
                      publishedPre.data());
          std::copy_n(preFrame.data(), framePosition,
                      publishedPre.data() + tail);
          std::copy_n(postFrame.data() + framePosition, tail,
                      publishedPost.data());
          std::copy_n(postFrame.data(), framePosition,
                      publishedPost.data() + tail);
          frameReady.store(true, std::memory_order_release);
        }
        hopPosition = 0;
      }
    }
    offset += count;
  }
  for (int i = 0; i < feral::maxBands; ++i)
    reductions[i].store(engine.gainReduction[i], std::memory_order_relaxed);
  busReduction.store(engine.busReduction, std::memory_order_relaxed);
  inputPeak.store(
      std::max(inputPeak.load(std::memory_order_relaxed), engine.inputPeak),
      std::memory_order_relaxed);
  outputPeak.store(
      std::max(outputPeak.load(std::memory_order_relaxed), engine.outputPeak),
      std::memory_order_relaxed);
}
void FeralProcessor::processBlock(juce::AudioBuffer<float> &b,
                                  juce::MidiBuffer &) {
  run(b, false);
}
void FeralProcessor::processBlockBypassed(juce::AudioBuffer<float> &b,
                                          juce::MidiBuffer &) {
  run(b, true);
}
bool FeralProcessor::popAnalysis(std::array<float, analysisSize> &pre,
                                 std::array<float, analysisSize> &post) {
  if (!frameReady.load(std::memory_order_acquire))
    return false;
  pre = publishedPre;
  post = publishedPost;
  frameReady.store(false, std::memory_order_release);
  return true;
}
juce::AudioProcessorParameter *FeralProcessor::getBypassParameter() const {
  return state.getParameter("bypass");
}
juce::AudioProcessorEditor *FeralProcessor::createEditor() {
  return new FeralEditor(*this);
}
const juce::String FeralProcessor::getProgramName(int i) {
  static const char *names[]{"Clean slate", "Vocal restraint",
                             "Drum discipline", "Master pressure",
                             "Low-end grip"};
  return names[juce::jlimit(0, 4, i)];
}
void FeralProcessor::setCurrentProgram(int index) {
  index = juce::jlimit(0, 4, index);
  undo.beginNewTransaction("Load preset");
  for (int i = 0; i < feral::maxBands; ++i)
    for (const auto &s : specs)
      if (s.member)
        setValue(bandID(i, s.key), s.initial);
  for (const auto &s : globalSpecs)
    if (s.member && juce::String(s.key) != "bypass")
      setValue(s.key, s.initial);
  const auto band = [&](int i, float frequency, float gain, float q,
                        float threshold, float range) {
    setValue(bandID(i, "enabled"), 1);
    setValue(bandID(i, "frequency"), frequency);
    setValue(bandID(i, "gain"), gain);
    setValue(bandID(i, "q"), q);
    setValue(bandID(i, "threshold"), threshold);
    setValue(bandID(i, "range"), range);
  };
  if (index == 1) {
    band(0, 130, -.8f, .8f, -26, 3);
    band(1, 340, -1.2f, 1.3f, -30, 4);
    band(2, 3200, .7f, 1.4f, -28, 4);
    band(3, 7200, -.4f, 2.1f, -33, 6);
    setValue("busEnabled", 1);
    setValue("threshold", -20);
    setValue("ratio", 2);
  } else if (index == 2) {
    band(0, 80, 1.2f, .8f, -22, 3);
    band(1, 430, -2, 1.2f, -26, 4);
    band(2, 4000, 1, .8f, -18, 3);
    setValue("busEnabled", 1);
    setValue("ratio", 4);
    setValue("attack", 20);
    setValue("release", 90);
  } else if (index == 3) {
    band(0, 90, 0, .7f, -28, 2);
    band(1, 2800, 0, .7f, -30, 2);
    setValue("busEnabled", 1);
    setValue("ratio", 1.5f);
    setValue("range", 3);
    setValue("detectorHP", 100);
  } else if (index == 4) {
    band(0, 75, 0, .7f, -30, 9);
    band(1, 240, -1.5f, 1, -28, 5);
    setValue(bandID(0, "attack"), 25);
    setValue(bandID(0, "release"), 200);
  }
  preset.store(index);
}
void FeralProcessor::selectBank(int next) {
  next = juce::jlimit(0, 1, next);
  const juce::ScopedLock lock(bankMutex);
  const int old = bank.load();
  if (old == next)
    return;
  banks[old] = state.copyState();
  bankPresets[old] = preset.load();
  state.replaceState(banks[next].createCopy());
  preset.store(bankPresets[next]);
  bank.store(next);
  undo.clearUndoHistory();
}
void FeralProcessor::copyBank() {
  const juce::ScopedLock lock(bankMutex);
  const int current = bank.load();
  banks[1 - current] = state.copyState();
  bankPresets[1 - current] = preset.load();
}
void FeralProcessor::getStateInformation(juce::MemoryBlock &output) {
  const juce::ScopedLock lock(bankMutex);
  banks[bank.load()] = state.copyState();
  bankPresets[bank.load()] = preset.load();
  juce::ValueTree root("FERAL_STATE");
  root.setProperty("version", 1, nullptr);
  root.setProperty("bank", bank.load(), nullptr);
  root.setProperty("selected", selected.load(), nullptr);
  for (int i = 0; i < 2; ++i) {
    juce::ValueTree b("BANK");
    b.setProperty("preset", bankPresets[i], nullptr);
    b.addChild(banks[i].createCopy(), -1, nullptr);
    root.addChild(b, -1, nullptr);
  }
  if (auto xml = root.createXml())
    copyXmlToBinary(*xml, output);
}
void FeralProcessor::setStateInformation(const void *data, int size) {
  const auto xml = getXmlFromBinary(data, size);
  if (!xml || !xml->hasTagName("FERAL_STATE"))
    return;
  const auto root = juce::ValueTree::fromXml(*xml);
  if (root.getNumChildren() != 2)
    return;
  for (int i = 0; i < 2; ++i)
    if (!root.getChild(i).getChild(0).hasType("FERAL_PARAMETERS"))
      return;
  const juce::ScopedLock lock(bankMutex);
  for (int i = 0; i < 2; ++i) {
    banks[i] = root.getChild(i).getChild(0).createCopy();
    bankPresets[i] = juce::jlimit(0, 4, (int)root.getChild(i)["preset"]);
  }
  bank.store(juce::jlimit(0, 1, (int)root["bank"]));
  selected.store(juce::jlimit(-1, 7, (int)root["selected"]));
  preset.store(bankPresets[bank.load()]);
  state.replaceState(banks[bank.load()].createCopy());
  undo.clearUndoHistory();
}
#ifndef HG_SUITE_BUILD
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new FeralProcessor;
}

#endif
