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
  void paintPanel(juce::Graphics&,juce::Rectangle<float>,juce::Colour tint) const;
  void paintWordmark(juce::Graphics&,const juce::String&,juce::Rectangle<float>,float fontHeight) const;
  void paintSpectralMark(juce::Graphics&,juce::Rectangle<float>,juce::Colour accent) const;
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
  void drawComboBoxTextWhenNothingSelected(juce::Graphics&,juce::ComboBox&,juce::Label&) override;
  void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider::SliderStyle,
                        juce::Slider &) override;
  juce::Font getComboBoxFont(juce::ComboBox &) override;
  juce::Font getLabelFont(juce::Label &) override;
  void drawLabel(juce::Graphics &, juce::Label &) override;
  juce::Label* createSliderTextBox(juce::Slider&) override;
  int getSliderThumbRadius(juce::Slider&) override;
  void fillTextEditorBackground(juce::Graphics&,int,int,juce::TextEditor&) override;
  void drawTextEditorOutline(juce::Graphics&,int,int,juce::TextEditor&) override;
  juce::Font getPopupMenuFont() override;
  void getIdealPopupMenuItemSize(const juce::String&,bool,int,int&,int&) override;
  void drawPopupMenuBackground(juce::Graphics&,int,int) override;
  void drawPopupMenuItem(juce::Graphics&,const juce::Rectangle<int>&,bool,bool,bool,bool,bool,
      const juce::String&,const juce::String&,const juce::Drawable*,const juce::Colour*) override;
  void drawPopupMenuSectionHeader(juce::Graphics&,const juce::Rectangle<int>&,const juce::String&) override;
  void drawPopupMenuUpDownArrow(juce::Graphics&,int,int,bool) override;

private:
  juce::Image dialAtlas, faceplate, display, keycap, selectedKeycap, reelAtlas;
};
} // namespace hungryghost
