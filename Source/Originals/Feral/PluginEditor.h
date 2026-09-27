#pragma once
#include "PluginProcessor.h"
#include "UI/GhostTheme.h"
#include <juce_dsp/juce_dsp.h>

class FeralControl final : public juce::Component {
public:
  FeralControl(FeralProcessor &, juce::String title, bool rotary);
  void bind(const juce::String &id, const juce::String &unit);
  void resized() override;
  juce::Slider slider;
  juce::Label title;

private:
  FeralProcessor &processor;
  bool rotary;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};
class FrequencyGraph final : public juce::Component {
public:
  explicit FrequencyGraph(FeralProcessor &);
  void paint(juce::Graphics &) override;
  void updateSpectrum();
  void mouseDown(const juce::MouseEvent &) override;
  void mouseDrag(const juce::MouseEvent &) override;
  void mouseUp(const juce::MouseEvent &) override;
  void mouseDoubleClick(const juce::MouseEvent &) override;
  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &) override;
  void addBand(float frequency = 1000, float gain = 0);
  std::function<void()> onSelection;

private:
  FeralProcessor &processor;
  juce::dsp::FFT fft{13};
  std::array<float, FeralProcessor::analysisSize> pre{}, post{};
  std::array<float, FeralProcessor::analysisSize * 2> fftData{};
  std::array<float, FeralProcessor::analysisSize / 2> preDB{}, postDB{};
  std::array<juce::Path, feral::maxBands> numberOutlines;
  juce::Rectangle<float> plot() const;
  float x(float frequency) const;
  float y(float gain) const;
  float frequency(float x) const;
  float gain(float y) const;
  int hit(juce::Point<float>) const;
  int dragging = -1;
  bool spectrumActive = false;
  double lastSpectrumTime = 0;
};
class FeralEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer {
public:
  explicit FeralEditor(FeralProcessor &);
  ~FeralEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  void select(int);
  void refresh();
  FrequencyGraph graph;

private:
  FeralProcessor &processor;
  hungryghost::GhostTheme theme;
  hungryghost::LicenseButton licenceButton{processor.licence};
  juce::TooltipWindow tooltips{this, 600};
  std::array<juce::TextButton, 8> bands;
  juce::TextButton bus{"Bus"}, add{"+ Band"}, remove{"Remove"}, bankA{"A"},
      bankB{"B"}, copy{"Copy"}, power{"Bypass"}, dynamic{"Dynamic"},
      enabled{"Active"}, external{"Ext key"}, undo{"Undo"}, redo{"Redo"};
  juce::ComboBox presets, shape, channel;
  std::array<std::unique_ptr<FeralControl>, 6> dynamics;
  std::array<std::unique_ptr<FeralControl>, 3> filter;
  std::array<std::unique_ptr<FeralControl>, 3> global;
  std::unique_ptr<FeralControl> detectorHP;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>
      toggles;
  float inputDB = -100, outputDB = -100;
  int boundSelection = -2;
  void timerCallback() override;
  void bindSelection();
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeralEditor)
};
