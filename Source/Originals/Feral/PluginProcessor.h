#pragma once
#include "../../Licensing/LicenseManager.h"
#include "DynamicsEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>

class FeralProcessor final : public juce::AudioProcessor {
public:
  FeralProcessor();
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void reset() override { engine.reset(); }
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &,
                            juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "Hungry Ghost FERAL"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override;
  juce::AudioProcessorParameter *getBypassParameter() const override;
  int getNumPrograms() override { return 5; }
  int getCurrentProgram() override { return preset.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  feral::Parameters readParameters() const noexcept;
  void setValue(const juce::String &, float);
  float value(const juce::String &) const;
  static juce::String bandID(int i, const char *key) {
    return "b" + juce::String(i + 1) + "_" + key;
  }
  void selectBank(int);
  void copyBank();
  int selectedBank() const { return bank.load(); }
  // SPSC analyser transport: GUI is the only consumer; dropping frames cannot
  // stall audio.
  static constexpr int analysisSize = 8192;
  static constexpr int analysisHop = 1024;
  bool popAnalysis(std::array<float, analysisSize> &pre,
                   std::array<float, analysisSize> &post);
  juce::UndoManager undo;
  hungryghost::LicenseManager licence{"feral"};
  juce::AudioProcessorValueTreeState state;
  std::atomic<int> selected{1};
  std::array<std::atomic<float>, feral::maxBands> reductions{};
  std::atomic<float> busReduction{0}, inputPeak{0}, outputPeak{0};

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  struct Binding {
    std::atomic<float> *source;
    float feral::Band::*member;
  };
  struct GlobalBinding {
    std::atomic<float> *source;
    float feral::Parameters::*member;
  };
  std::array<std::array<Binding, 15>, feral::maxBands> bindings;
  std::array<GlobalBinding, 15> globals;
  feral::DynamicsEngine engine;
  std::atomic<int> preset{1}, bank{0};
  std::array<juce::ValueTree, 2> banks;
  std::array<int, 2> bankPresets{1, 0};
  juce::CriticalSection bankMutex;
  std::array<float, analysisSize> inputScratch{}, preFrame{}, postFrame{},
      publishedPre{}, publishedPost{};
  int framePosition = 0, hopPosition = 0, filledSamples = 0;
  std::atomic<bool> frameReady{false};
  void run(juce::AudioBuffer<float> &, bool);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeralProcessor)
};
