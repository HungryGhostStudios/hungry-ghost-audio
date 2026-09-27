#pragma once
#include "SuiteProcessor.h"
#include "UI/GhostTheme.h"
#include <juce_dsp/juce_dsp.h>
namespace hungryghost {
class SuiteEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer {
public:
  explicit SuiteEditor(SuiteProcessor &);
  ~SuiteEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  SuiteProcessor &processor;
  GhostTheme theme;
  LicenseButton licenceButton{processor.licence};
  std::array<juce::Slider, 8> knobs;
  std::array<juce::Label, 8> labels;
  std::array<
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8>
      attachments;
  juce::TextButton bankA{"A"}, bankB{"B"}, copy{"Copy"}, resetButton{"Factory"},
      bypassButton{"Bypass"};
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      bypassAttachment;
  juce::dsp::FFT fft{12};
  std::array<float, 8192> fftPre{}, fftPost{};
  std::array<float, 2048> spectrumPre{}, spectrumPost{};
  std::array<float, 4096> window{};
  juce::Rectangle<float> display;
  float peakIn = 0, peakOut = 0, gr = 0;
  double lastFrame = 0;
  void timerCallback() override;
  void drawSpectrum(juce::Graphics &, const std::array<float, 2048> &,
                    juce::Colour);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuiteEditor)
};
} // namespace hungryghost
