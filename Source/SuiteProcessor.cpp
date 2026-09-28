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
  auto extra=[&](const char* id,const char* name,float lo,float hi,float start,float skew=1.f){result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(id,1),name,juce::NormalisableRange<float>(lo,hi,0,skew),start));};
  if(hasTempoSync(p.kind)){
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("tempo_sync",1),"Tempo sync",false));
    result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("beat_division",1),"Note division",juce::StringArray{"1/1","1/2","1/4","1/8","1/16","1/32","1/4 dotted","1/8 dotted","1/16 dotted","1/4 triplet","1/8 triplet","1/16 triplet"},2));
    extra("fallback_bpm","Fallback tempo",20,400,120);
  }
  if(hasDetector(p.kind)){
    if(p.kind!=Kind::BusCompressor)extra("key_highpass","Detector high-pass",0,1000,0,.5f);
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("key_listen",1),"Audition detector",false));
  }
  if(p.kind==Kind::ParametricEQ||p.kind==Kind::TiltEQ){
    extra("band_q0",p.kind==Kind::TiltEQ?"Shelf shape Q":"Low band Q",.1f,12,.707f,.4f);
    if(p.kind==Kind::ParametricEQ){extra("band_q1","Mid band Q",.1f,12,.707f,.4f);extra("band_q2","High band Q",.1f,12,.707f,.4f);}
  }
  if(hasColourFilter(p.kind))extra("input_lowcut","Pre-drive low cut",0,1000,0,.5f);
  if(hasRepeatFilter(p.kind))extra("repeat_lowcut","Repeat low cut",0,1000,0,.5f);
  if(p.kind==Kind::Width||p.kind==Kind::MonoBass||p.kind==Kind::Haas||p.kind==Kind::Polarity)
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("mono_listen",1),"Audition mono",false));
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
  tempoSync=state.getRawParameterValue("tempo_sync");division=state.getRawParameterValue("beat_division");fallbackBpm=state.getRawParameterValue("fallback_bpm");
  detectorCut=state.getRawParameterValue("key_highpass");keyListen=state.getRawParameterValue("key_listen");inputCut=state.getRawParameterValue("input_lowcut");repeatCut=state.getRawParameterValue("repeat_lowcut");monoListen=state.getRawParameterValue("mono_listen");
  for(int i=0;i<3;++i)bandQ[i]=state.getRawParameterValue("band_q"+juce::String(i));
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
    const double time = static_cast<double>(tempoSync&&tempoSync->load()>.5f?(effectivePrimary.load()>0?effectivePrimary.load():syncedControl(product.kind,fallbackBpm->load(),static_cast<int>(division->load()))):controls[0]->load()) * .001;
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
  AdvancedSettings advanced;
  for(int i=0;i<3;++i)if(bandQ[i])advanced.bandQ[i]=bandQ[i]->load();
  if(detectorCut)advanced.detectorCut=detectorCut->load();
  if(inputCut)advanced.inputCut=inputCut->load();
  if(repeatCut)advanced.repeatCut=repeatCut->load();
  if(keyListen)advanced.listenKey=keyListen->load()>.5f;
  if(monoListen)advanced.monoListen=monoListen->load()>.5f;
  if(tempoSync&&tempoSync->load()>.5f){
    double bpm=fallbackBpm->load();
    if(auto* head=getPlayHead())if(auto position=head->getPosition())if(auto tempo=position->getBpm())bpm=*tempo;
    effectiveBpm.store(static_cast<float>(bpm));advanced.syncedValue=syncedControl(product.kind,bpm,juce::roundToInt(division->load()));
  }
  effectivePrimary.store(advanced.syncedValue>0?advanced.syncedValue:values[0]);
  engine.setAdvanced(advanced);
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
      leftFifo[fifoIndex]=audio.getSample(0,offset+i);
      rightFifo[fifoIndex]=audio.getSample(audio.getNumChannels()>1?1:0,offset+i);
      if (++fifoIndex == analysisSize) {
        fifoIndex = 0;
        if (!frameReady.load(std::memory_order_acquire)) {
          preFrame = preFifo;
          postFrame = postFifo;
          leftFrame=leftFifo;
          rightFrame=rightFifo;
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
                                 std::array<float, analysisSize> &post,
                                 std::array<float,analysisSize>* left,
                                 std::array<float,analysisSize>* right) {
  if (!frameReady.load(std::memory_order_acquire))
    return false;
  pre = preFrame;
  post = postFrame;
  if(left) *left=leftFrame;
  if(right) *right=rightFrame;
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
  for (int i = 0; i < 2; ++i) {
    banks[i] = root.getChild(i).createCopy();
    // Older sessions have no advanced parameters. Restore neutral defaults,
    // rather than retaining whatever advanced settings were previously loaded.
    for (auto* parameter : getParameters()) if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(parameter)) {
      bool found = false;
      for (auto child : banks[i]) if (child["id"].toString() == p->paramID) { found = true; break; }
      if (!found) {
        juce::ValueTree child("PARAM");
        child.setProperty("id", p->paramID, nullptr);
        child.setProperty("value", p->convertFrom0to1(p->getDefaultValue()), nullptr);
        banks[i].addChild(child, -1, nullptr);
      }
    }
  }
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
