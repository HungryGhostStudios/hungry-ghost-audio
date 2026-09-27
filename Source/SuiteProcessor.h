#pragma once
#include "DSP/SuiteEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>
namespace hungryghost {
class SuiteProcessor final : public juce::AudioProcessor {
public:
  explicit SuiteProcessor(int productIndex);
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void reset() override { engine.reset(); }
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &,
                            juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override {
    return "Hungry Ghost " + juce::String(product.name);
  }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override;
  juce::AudioProcessorParameter *getBypassParameter() const override;
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return "Factory"; }
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void selectBank(int);
  void copyBank();
  void factoryReset();
  int selectedBank() const { return bank.load(); }
  static constexpr int analysisSize = 4096;
  bool popAnalysis(std::array<float, analysisSize> &,
                   std::array<float, analysisSize> &);
  const Product &product;
  juce::UndoManager undo;
  juce::AudioProcessorValueTreeState state;
  std::atomic<float> inputPeak{0}, outputPeak{0}, reduction{0};

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout
  layout(const Product &);
  SuiteEngine engine;
  std::array<std::atomic<float> *, 6> controls{};
  std::atomic<float> *wet = nullptr, *out = nullptr, *bypass = nullptr;
  std::array<juce::ValueTree, 2> banks;
  juce::CriticalSection bankLock;
  std::atomic<int> bank{0};
  std::array<float, analysisSize> preFifo{}, postFifo{}, preFrame{},
      postFrame{};
  int fifoIndex = 0;
  std::atomic<bool> frameReady{false};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuiteProcessor)
};
} // namespace hungryghost
