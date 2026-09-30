#pragma once
#include "DSP/SuiteEngine.h"
#include "Licensing/LicenseManager.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
namespace hungryghost {
class SuiteProcessor final : public juce::AudioProcessor {
public:
  explicit SuiteProcessor(int productIndex);
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  void reset() override;
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
                   std::array<float, analysisSize> &,
                   std::array<float, analysisSize>* left=nullptr,
                   std::array<float, analysisSize>* right=nullptr);
  const Product &product;
  LicenseManager licence;
  juce::UndoManager undo;
  juce::AudioProcessorValueTreeState state;
  std::atomic<float> inputPeak{0}, outputPeak{0}, reduction{0};
  std::atomic<float> reductionLeft{0}, reductionRight{0};
  std::atomic<float> effectivePrimary{0},effectiveBpm{120};

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout
  layout(const Product &);
  SuiteEngine engine;
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
  std::array<std::array<float, 256>, 2> bypassDelay{};
  int bypassWrite = 0, latency = 0;
  float processingBlend = 1, blendStep = .005f;
  bool isNonlinear() const;
  void run(juce::AudioBuffer<float> &, bool hostBypass);
  std::array<std::atomic<float> *, 6> controls{};
  std::atomic<float> *wet = nullptr, *out = nullptr, *bypass = nullptr;
  std::atomic<float> *tempoSync=nullptr,*division=nullptr,*fallbackBpm=nullptr,
      *detectorCut=nullptr,*keyListen=nullptr,*inputCut=nullptr,*repeatCut=nullptr,*monoListen=nullptr;
  std::array<std::atomic<float>*,3> bandQ{};
  std::atomic<float> *bondModel=nullptr,*bondTopology=nullptr,*bondDetector=nullptr,
      *bondLink=nullptr,*bondKnee=nullptr,*bondRange=nullptr,*bondKeyLowpass=nullptr,
      *bondAutoRelease=nullptr;
  std::array<juce::ValueTree, 2> banks;
  juce::CriticalSection bankLock;
  std::atomic<int> bank{0};
  std::array<float, analysisSize> preFifo{}, postFifo{}, preFrame{},
      postFrame{};
  std::array<float,analysisSize> leftFifo{},rightFifo{},leftFrame{},rightFrame{};
  int fifoIndex = 0;
  std::atomic<bool> frameReady{false};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuiteProcessor)
};
} // namespace hungryghost
