#pragma once
#include "../../Licensing/LicenseManager.h"
#include "ReverbEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>

class AfterProcessor final : public juce::AudioProcessor {
public:
  AfterProcessor();
  ~AfterProcessor() override = default;
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void reset() override { engine.reset(); }
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &,
                            juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "Hungry Ghost"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override;
  juce::AudioProcessorParameter *getBypassParameter() const override;
  int getNumPrograms() override { return 5; }
  int getCurrentProgram() override { return presetIndex.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  after::Parameters readParameters() const noexcept;
  void selectBank(int);
  void copyBank();
  int selectedBank() const noexcept { return bankIndex.load(); }
  bool isMixLocked() const noexcept { return mixLocked.load(); }
  void setMixLocked(bool value) noexcept { mixLocked.store(value); }
  void setValue(const juce::String &id, float value);
  hungryghost::LicenseManager licence{"reverb"};
  juce::AudioProcessorValueTreeState state;

private:
  struct FloatBinding {
    const char *id;
    float after::Parameters::*member;
    std::atomic<float> *value = nullptr;
  };
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
  std::array<FloatBinding, 20> bindings;
  std::atomic<float> *character = nullptr;
  std::atomic<float> *freeze = nullptr;
  std::atomic<float> *bypass = nullptr;
  after::ReverbEngine engine;
  std::array<juce::ValueTree, 2> banks;
  juce::CriticalSection bankMutex;
  std::array<int, 2> bankPresets{0, 1};
  std::atomic<int> bankIndex{0}, presetIndex{0};
  std::atomic<bool> mixLocked{false};
  void run(juce::AudioBuffer<float> &, bool forceBypass);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfterProcessor)
};
