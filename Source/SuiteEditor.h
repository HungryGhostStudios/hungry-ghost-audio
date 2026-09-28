#pragma once
#include "SuiteProcessor.h"
#include "UI/GhostTheme.h"
#include "UI/ProductDesign.h"
#include "UI/ProductPresets.h"
#include "UI/AdvancedPanel.h"
#include <juce_dsp/juce_dsp.h>
namespace hungryghost {
class SuiteEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer {
public:
  explicit SuiteEditor(SuiteProcessor &);
  ~SuiteEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;

private:
  SuiteProcessor &processor;
  GhostTheme theme;
  ProductDesign design;
  std::array<ProductPreset,3> presets;
  juce::ComboBox presetMenu;
  juce::TextButton advancedButton{"Shape +"};
  std::unique_ptr<AdvancedPanel> advancedPanel;
  juce::TooltipWindow tips{this,600};
  std::array<juce::TextButton,2> polarityButtons;
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
  juce::Rectangle<float> secondaryDisplay;
  std::array<float,4096> waveformPre{},waveformPost{},stereoL{},stereoR{};
  std::array<float,200> reductionHistory{};
  int historyWrite=0,dragParameter=-1,reelFrame=0;
  float peakIn = 0, peakOut = 0, gr = 0;
  double lastFrame = 0;
  void timerCallback() override;
  void drawSpectrum(juce::Graphics &, const std::array<float, 2048> &,
                    juce::Colour);
  void panel(juce::Graphics&,juce::Rectangle<float>,const juce::String&,bool screen=false);
  void text(juce::Graphics&,const juce::String&,juce::Rectangle<float>,float,juce::Colour,bool bold=false,juce::Justification=juce::Justification::left);
  void waveform(juce::Graphics&,juce::Rectangle<float>,bool comparison=true);
  void meter(juce::Graphics&,juce::Rectangle<float>,float,const juce::String&,bool reduction=false);
  void needle(juce::Graphics&,juce::Rectangle<float>,float,const juce::String&,bool cream=false);
  void response(juce::Graphics&);
  void timing(juce::Graphics&,bool stereo=false);
  void motion(juce::Graphics&);
  void stereo(juce::Graphics&);
  void applyPreset(int);
  void place(int,juce::Rectangle<float>,bool fader=false);
  void row(const std::vector<int>&,juce::Rectangle<float>,bool fader=false);
  float value(int) const;
  void editGraph(const juce::MouseEvent&);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuiteEditor)
};
} // namespace hungryghost
