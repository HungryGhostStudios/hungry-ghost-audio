#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout AfterProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout result;
  const auto add = [&](const char *id, const char *name, float lo, float hi,
                       float step, float initial, float centre = 0) {
    juce::NormalisableRange<float> range(lo, hi, step);
    if (centre > lo && centre < hi)
      range.setSkewForCentre(centre);
    result.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id, 1}, name, range, initial));
  };
  result.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{"character", 1}, "Character",
      juce::StringArray{"Room", "Chamber", "Hall", "Plate", "Cloud"}, 2));
  add("decay", "Decay", .15f, 30, .01f, 3.2f, 3.2f);
  add("size", "Size", 0, 100, 1, 68);
  add("tone", "Tone", -100, 100, 1, -20);
  add("motion", "Motion", 0, 100, 1, 22);
  add("predelay", "Pre-delay", 0, 250, 1, 24, 40);
  add("mix", "Mix", 0, 100, 1, 25);
  add("early", "Early / late", 0, 100, 1, 20);
  add("diffusion", "Diffusion", 0, 100, 1, 88);
  add("width", "Stereo width", 0, 150, 1, 100);
  add("low", "Low decay", .25f, 2, .01f, 1.25f);
  add("high", "High decay", .15f, 2, .01f, .65f);
  add("lowCross", "Low crossover", 100, 1000, 1, 250, 300);
  add("highCross", "High crossover", 1000, 12000, 10, 6000, 5000);
  add("lowcut", "Wet low cut", 20, 1000, 1, 120, 150);
  add("highcut", "Wet high cut", 1000, 20000, 10, 12500, 8000);
  add("rate", "Motion rate", .05f, 4, .01f, .35f, .5f);
  add("duck", "Ducking", 0, 100, 1, 0);
  add("release", "Duck release", 50, 1500, 1, 350, 350);
  add("input", "Input trim", -24, 12, .1f, 0);
  add("output", "Output trim", -24, 12, .1f, 0);
  result.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"freeze", 1}, "Freeze", false));
  result.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"bypass", 1}, "Bypass", false));
  return result;
}
AfterProcessor::AfterProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "AFTER_PARAMETERS", layout()),
      bindings{{{"decay", &after::Parameters::decay},
                {"size", &after::Parameters::size},
                {"tone", &after::Parameters::tone},
                {"motion", &after::Parameters::motion},
                {"predelay", &after::Parameters::preDelay},
                {"mix", &after::Parameters::mix},
                {"early", &after::Parameters::early},
                {"diffusion", &after::Parameters::diffusion},
                {"width", &after::Parameters::width},
                {"low", &after::Parameters::lowRatio},
                {"high", &after::Parameters::highRatio},
                {"lowCross", &after::Parameters::lowCrossover},
                {"highCross", &after::Parameters::highCrossover},
                {"lowcut", &after::Parameters::lowCut},
                {"highcut", &after::Parameters::highCut},
                {"rate", &after::Parameters::rate},
                {"duck", &after::Parameters::duck},
                {"release", &after::Parameters::release},
                {"input", &after::Parameters::inputDb},
                {"output", &after::Parameters::outputDb}}} {
  for (auto &binding : bindings)
    binding.value = state.getRawParameterValue(binding.id);
  character = state.getRawParameterValue("character");
  freeze = state.getRawParameterValue("freeze");
  bypass = state.getRawParameterValue("bypass");
  setCurrentProgram(1);
  banks[1] = state.copyState();
  setCurrentProgram(0);
  banks[0] = state.copyState();
}
void AfterProcessor::prepareToPlay(double sampleRate, int) {
  engine.setParameters(readParameters());
  engine.prepare(sampleRate);
  setLatencySamples(0);
}
bool AfterProcessor::isBusesLayoutSupported(const BusesLayout &buses) const {
  const auto in = buses.getMainInputChannelSet(),
             out = buses.getMainOutputChannelSet();
  return in == out && (out == juce::AudioChannelSet::mono() ||
                       out == juce::AudioChannelSet::stereo());
}
after::Parameters AfterProcessor::readParameters() const noexcept {
  after::Parameters p;
  for (const auto &b : bindings)
    p.*(b.member) = b.value->load(std::memory_order_relaxed);
  p.character = static_cast<int>(character->load(std::memory_order_relaxed));
  p.freeze = freeze->load(std::memory_order_relaxed) > .5f;
  p.bypass = bypass->load(std::memory_order_relaxed) > .5f;
  return p;
}
void AfterProcessor::run(juce::AudioBuffer<float> &buffer, bool forceBypass) {
  juce::ScopedNoDenormals noDenormals;
  if (buffer.getNumChannels() == 0)
    return;
  auto p = readParameters();
  p.bypass = p.bypass || forceBypass;
  engine.setParameters(p);
  engine.process(buffer.getWritePointer(0),
                 buffer.getNumChannels() > 1 ? buffer.getWritePointer(1)
                                             : nullptr,
                 buffer.getNumSamples());
  for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
    buffer.clear(c, 0, buffer.getNumSamples());
}
void AfterProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                  juce::MidiBuffer &) {
  run(buffer, false);
}
void AfterProcessor::processBlockBypassed(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &) {
  run(buffer, true);
}
double AfterProcessor::getTailLengthSeconds() const {
  const auto p = readParameters();
  return p.freeze
             ? 3600.0
             : static_cast<double>(
                   p.decay *
                       std::max(p.lowRatio,
                                p.highRatio * std::pow(2.0f, p.tone / 120.0f)) +
                   p.preDelay * .001f + .8f);
}
juce::AudioProcessorParameter *AfterProcessor::getBypassParameter() const {
  return state.getParameter("bypass");
}
void AfterProcessor::setValue(const juce::String &id, float value) {
  if (auto *p = state.getParameter(id)) {
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(value));
    p->endChangeGesture();
  }
}
const juce::String AfterProcessor::getProgramName(int index) {
  static const juce::StringArray names{"Hollow chapel", "Empty temple",
                                       "Silken spirit", "Endless hunger",
                                       "Golden apparition"};
  return names[juce::jlimit(0, 4, index)];
}
void AfterProcessor::setCurrentProgram(int index) {
  index = juce::jlimit(0, 4, index);
  const float mix = state.getRawParameterValue("mix")->load();
  after::Parameters p;
  if (index == 1) {
    p.character = 0;
    p.decay = .65f;
    p.size = 25;
    p.tone = 0;
    p.motion = 5;
    p.preDelay = 8;
    p.mix = 18;
    p.early = 55;
    p.diffusion = 65;
    p.lowRatio = 1.1f;
    p.highRatio = .7f;
  }
  if (index == 2) {
    p.character = 3;
    p.decay = 1.8f;
    p.size = 58;
    p.tone = 18;
    p.motion = 12;
    p.preDelay = 45;
    p.mix = 22;
    p.early = 8;
    p.diffusion = 95;
    p.lowRatio = .8f;
    p.highRatio = .85f;
    p.duck = 25;
  }
  if (index == 3) {
    p.character = 4;
    p.decay = 12;
    p.size = 95;
    p.tone = -35;
    p.motion = 55;
    p.preDelay = 65;
    p.mix = 45;
    p.early = 5;
    p.diffusion = 100;
    p.width = 125;
    p.lowRatio = 1.1f;
    p.highRatio = .75f;
    p.rate = .15f;
  }
  if (index == 4) {
    p.character = 1;
    p.decay = 1.4f;
    p.size = 40;
    p.tone = -15;
    p.motion = 8;
    p.preDelay = 18;
    p.mix = 20;
    p.early = 38;
    p.diffusion = 78;
    p.lowRatio = 1.2f;
    p.highRatio = .55f;
  }
  if (mixLocked.load())
    p.mix = mix;
  for (const auto &binding : bindings)
    setValue(binding.id, p.*(binding.member));
  setValue("character", static_cast<float>(p.character));
  presetIndex.store(index);
}
void AfterProcessor::selectBank(int selected) {
  selected = juce::jlimit(0, 1, selected);
  const juce::ScopedLock guard(bankMutex);
  const int previous = bankIndex.load();
  if (selected == previous)
    return;
  banks[previous] = state.copyState();
  bankPresets[previous] = presetIndex.load();
  state.replaceState(banks[selected].createCopy());
  presetIndex.store(bankPresets[selected]);
  bankIndex.store(selected);
}
void AfterProcessor::copyBank() {
  const juce::ScopedLock guard(bankMutex);
  const int other = 1 - bankIndex.load();
  banks[other] = state.copyState();
  bankPresets[other] = presetIndex.load();
}
void AfterProcessor::getStateInformation(juce::MemoryBlock &destination) {
  const juce::ScopedLock guard(bankMutex);
  const int active = bankIndex.load();
  banks[active] = state.copyState();
  bankPresets[active] = presetIndex.load();
  juce::ValueTree root("AFTER_SESSION");
  root.setProperty("version", 1, nullptr);
  root.setProperty("bank", active, nullptr);
  root.setProperty("mixLock", mixLocked.load(), nullptr);
  for (int i = 0; i < 2; ++i) {
    auto tree = banks[i].createCopy();
    tree.setProperty("preset", bankPresets[i], nullptr);
    root.addChild(tree, -1, nullptr);
  }
  if (auto xml = root.createXml())
    copyXmlToBinary(*xml, destination);
}
void AfterProcessor::setStateInformation(const void *data, int length) {
  if (auto xml = getXmlFromBinary(data, length)) {
    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.hasType("AFTER_SESSION") || root.getNumChildren() != 2)
      return;
    for (int i = 0; i < 2; ++i)
      if (!root.getChild(i).hasType("AFTER_PARAMETERS"))
        return;
    const juce::ScopedLock guard(bankMutex);
    for (int i = 0; i < 2; ++i) {
      banks[i] = root.getChild(i).createCopy();
      bankPresets[i] = juce::jlimit(
          0, 4, static_cast<int>(banks[i].getProperty("preset", 0)));
    }
    bankIndex.store(
        juce::jlimit(0, 1, static_cast<int>(root.getProperty("bank", 0))));
    mixLocked.store(static_cast<bool>(root.getProperty("mixLock", false)));
    presetIndex.store(bankPresets[bankIndex.load()]);
    state.replaceState(banks[bankIndex.load()].createCopy());
  }
}
juce::AudioProcessorEditor *AfterProcessor::createEditor() {
  return new AfterEditor(*this);
}
#ifndef HG_SUITE_BUILD
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AfterProcessor();
}

#endif
