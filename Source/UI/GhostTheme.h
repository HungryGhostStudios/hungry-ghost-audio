#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace hungryghost {
// Shared suite presentation. No processor, parameter or audio-engine
// dependency.
class GhostTheme final : public juce::LookAndFeel_V4 {
public:
  GhostTheme();
  float scale = 1;
  static juce::Colour background(), panel(), ink(), muted(), accent(), line();
  void paintChassis(juce::Graphics &, juce::Rectangle<float>) const;
  void paintDisplay(juce::Graphics &, juce::Rectangle<float>) const;
  void paintReel(juce::Graphics &, juce::Rectangle<float>, int frame) const;
  bool hasAssets() const;
  void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider &) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool,
                      bool) override;
  void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int,
                    juce::ComboBox &) override;
  void positionComboBoxText(juce::ComboBox &, juce::Label &) override;
  void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider::SliderStyle,
                        juce::Slider &) override;
  juce::Font getComboBoxFont(juce::ComboBox &) override;
  juce::Font getLabelFont(juce::Label &) override;
  void drawLabel(juce::Graphics &, juce::Label &) override;

private:
  juce::Image dialAtlas, faceplate, display, keycap, selectedKeycap, reelAtlas;
};
} // namespace hungryghost
