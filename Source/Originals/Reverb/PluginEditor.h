#pragma once
#include "PluginProcessor.h"
#include "UI/GhostTheme.h"
using AfterLook = hungryghost::GhostTheme;

class AfterKnob final : public juce::Component {
public:
  AfterKnob(AfterProcessor &, const char *id, const juce::String &title,
            const juce::String &caption, const juce::String &unit,
            bool large = false);
  void resized() override;
  juce::Slider slider;

private:
  juce::Label title, caption;
  bool large;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};

class TailView final : public juce::Component {
public:
  explicit TailView(AfterProcessor &p) : processor(p) {}
  void paint(juce::Graphics &) override;

private:
  AfterProcessor &processor;
};

class AfterEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer {
public:
  explicit AfterEditor(AfterProcessor &);
  ~AfterEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  // Used by the integration harness to inspect the actual native UI.
  void showShape(bool);

private:
  AfterProcessor &processor;
  AfterLook look;
  juce::TooltipWindow tooltips{this, 500};
  TailView tail;
  std::array<std::unique_ptr<AfterKnob>, 6> knobs;
  std::array<juce::TextButton, 5> characters;
  juce::ComboBox presets;
  juce::TextButton bankA{"A"}, bankB{"B"}, copy{"A > B"}, power{"ON"},
      freeze{"Freeze"}, mixLock{"Mix lock"}, shape{"Shape +"};
  std::array<juce::TextButton, 3> tabs;
  struct Detail {
    juce::Label label;
    juce::Slider slider;
    int category = 0;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attachment;
  };
  std::vector<std::unique_ptr<Detail>> details;
  std::array<juce::Slider, 2> trims;
  std::array<
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 2>
      trimAttachments;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      freezeAttachment, powerAttachment;
  bool shapeOpen = false;
  int shapeTab = 0;
  void timerCallback() override;
  void selectTab(int);
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfterEditor)
};
