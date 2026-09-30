#include "SuiteEditor.h"
#include <cmath>
namespace hungryghost {
namespace {
juce::Font font(float h, bool bold = false) {
  return juce::Font(juce::FontOptions(
      "Segoe UI", h, bold ? juce::Font::bold : juce::Font::plain));
}
float decibel(float x) { return juce::Decibels::gainToDecibels(x, -90.f); }
} // namespace
SuiteEditor::SuiteEditor(SuiteProcessor &p)
    : AudioProcessorEditor(p), processor(p),design(designFor(p.product.kind)),presets(presetsFor(p.product)) {
  setLookAndFeel(&theme);
  addAndMakeVisible(licenceButton);
  setSize(design.width, design.height);
  setResizable(true, true);
  setResizeLimits(juce::roundToInt(design.width*.85),juce::roundToInt(design.height*.85),
                 juce::roundToInt(design.width*1.5),juce::roundToInt(design.height*1.5));
  getConstrainer()->setFixedAspectRatio(double(design.width)/design.height);
  presetMenu.setComponentID("preset");
  presetMenu.setTooltip("Musical starting points. Parameters remain fully editable and automatable.");
  for(int i=0;i<3;++i) presetMenu.addItem(presets[i].name,i+1);
  presetMenu.setSelectedId(1,juce::dontSendNotification);
  if(processor.product.kind==Kind::BusCompressor){
    presetMenu.setTitle("Starting point");
    presetMenu.setTextWhenNothingSelected("Choose a starting point");
    presetMenu.setSelectedId(0,juce::dontSendNotification);
  }
  presetMenu.onChange=[this]{applyPreset(presetMenu.getSelectedId()-1);};
  addAndMakeVisible(presetMenu);
  for (int i = 0; i < 8; ++i) {
    const bool parameter = i < processor.product.controlCount || i >= 6;
    if (!parameter)
      continue;
    const juce::String id = i < 6    ? "control" + juce::String(i)
                            : i == 6 ? "mix"
                                     : "output";
    const auto *control = i < 6 ? &processor.product.controls[i] : nullptr;
    knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knobs[i].setComponentID(id);
    knobs[i].setName(control?control->name:i==6?"Dry / wet":"Output");
    knobs[i].setColour(juce::Slider::thumbColourId,design.accent);
    knobs[i].setColour(juce::Slider::trackColourId,design.accent);
    knobs[i].getProperties().set("suite", true);
    knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 28);
    knobs[i].setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    knobs[i].setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    knobs[i].setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                                 juce::MathConstants<float>::pi * 2.8f, true);
    knobs[i].setDoubleClickReturnValue(
        true, control ? control->initial
              : i == 6
                  ? processor.state.getParameter("mix")->convertFrom0to1(
                        processor.state.getParameter("mix")->getDefaultValue())
                  : 0.f);
    knobs[i].setTooltip(control  ? control->name
                        : i == 6 ? "Dry / wet balance"
                                 : "Final output gain");
    addAndMakeVisible(knobs[i]);
    attachments[i] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, id, knobs[i]);
    const bool polarity = processor.product.kind == Kind::Polarity && i < 2;
    knobs[i].textFromValueFunction = [this,control, i, polarity](double v) {
      if(i==0)if(auto* sync=processor.state.getRawParameterValue("tempo_sync"))if(sync->load()>.5f&&processor.effectivePrimary.load()>0)v=processor.effectivePrimary.load();
      if (polarity)
        return juce::String(v >= .5 ? "Inverted" : "Normal");
      if (i == 6)
        return juce::String(v * 100, 0) + " %";
      if (i == 7)
        return juce::String(v, 1) + " dB";
      if (!control)
        return juce::String(v);
      if (juce::String(control->unit) == "Hz")
        return v >= 1000 ? juce::String(v / 1000., 2) + " kHz"
                         : juce::String(v, 0) + " Hz";
      return juce::String(v, v >= 100 ? 0 : 1) +
             (juce::String(control->unit).isEmpty()
                  ? ""
                  : " " + juce::String(control->unit));
    };
    knobs[i].valueFromTextFunction = [control, i,
                                      polarity](const juce::String &text) {
      auto value = text.trim().toLowerCase();
      if (polarity) {
        if (value == "inverted" || value == "on")
          return 1.;
        if (value == "normal" || value == "off")
          return 0.;
      }
      auto result = value.getDoubleValue();
      if (i == 6)
        return result * .01;
      if (control && juce::String(control->unit) == "Hz" &&
          value.containsChar('k'))
        result *= 1000.;
      return result;
    };
    knobs[i].updateText();
    labels[i].setText(control  ? control->name
                      : i == 6 ? "Dry / wet"
                               : "Output",
                      juce::dontSendNotification);
    labels[i].setJustificationType(juce::Justification::centred);
    addAndMakeVisible(labels[i]);
  }
  for (auto *b : {&bankA, &bankB, &copy, &resetButton, &bypassButton})
    addAndMakeVisible(*b);
  bankA.setClickingTogglesState(true);
  bankB.setClickingTogglesState(true);
  bankA.onClick = [this] { processor.selectBank(0); };
  bankB.onClick = [this] { processor.selectBank(1); };
  copy.onClick = [this] { processor.copyBank(); };
  resetButton.onClick = [this] {
    processor.factoryReset();
    presetMenu.setSelectedId(processor.product.kind==Kind::BusCompressor?0:1,juce::dontSendNotification);
    bondPresetValues.clear();bondPresetBank=-1;
  };
  bypassButton.setClickingTogglesState(true);
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor.state, "bypass", bypassButton);
  if(processor.product.kind==Kind::Polarity) for(int i=0;i<2;++i){
    polarityButtons[i].setComponentID("polarity"+juce::String(i));
    polarityButtons[i].setClickingTogglesState(true);
    polarityButtons[i].setTooltip("Invert this channel's polarity. This is not a time-alignment control.");
    polarityButtons[i].onClick=[this,i]{auto* p=processor.state.getParameter("control"+juce::String(i));p->beginChangeGesture();p->setValueNotifyingHost(polarityButtons[i].getToggleState()?1.f:0.f);p->endChangeGesture();};
    addAndMakeVisible(polarityButtons[i]);
  }
  if(processor.product.kind!=Kind::BusCompressor && processor.getParameters().size()>processor.product.controlCount+3){
    advancedPanel=std::make_unique<AdvancedPanel>(processor);addChildComponent(*advancedPanel);
    advancedButton.setComponentID("advanced");addAndMakeVisible(advancedButton);
    advancedButton.setTooltip("Focused extra controls for this processor. Existing parameters and automation are preserved.");
    advancedButton.onClick=[this]{advancedPanel->setVisible(!advancedPanel->isVisible());advancedPanel->toFront(true);};
  }
  for (int i = 0; i < 4096; ++i)
    window[i] = .5f - .5f * std::cos(juce::MathConstants<float>::twoPi *
                                     static_cast<float>(i) / 4095.f);
  spectrumPre.fill(-90);
  spectrumPost.fill(-90);
  if(processor.product.kind==Kind::BusCompressor){
    bondPanel=std::make_unique<BondPanel>(processor);
    addAndMakeVisible(*bondPanel);
    bondPanel->toBack();
    setLookAndFeel(&bondPanel->lookAndFeel());
  }
  lastFrame = juce::Time::getMillisecondCounterHiRes();
  startTimerHz(30);
  resized();
  timerCallback();
}
SuiteEditor::~SuiteEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void SuiteEditor::drawSpectrum(juce::Graphics &g,
                               const std::array<float, 2048> &spectrum,
                               juce::Colour colour) {
  auto plot = display.reduced(22 * theme.scale, 25 * theme.scale);
  juce::Path path;
  const float sr = static_cast<float>(
      processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000);
  for (int x = 0; x <= 400; ++x) {
    float hz = 20.f * std::pow(std::min(20000.f, sr * .45f) / 20.f,
                               static_cast<float>(x) / 400.f);
    float bin = hz * 4096.f / sr;
    int lo = juce::jlimit(0, 2046, static_cast<int>(bin));
    float value = spectrum[lo] + (spectrum[lo + 1] - spectrum[lo]) * (bin - lo);
    float px = plot.getX() + plot.getWidth() * static_cast<float>(x) / 400.f,
          py = plot.getBottom() -
               plot.getHeight() * juce::jlimit(0.f, 1.f, (value + 90) / 90.f);
    if (x == 0)
      path.startNewSubPath(px, py);
    else
      path.lineTo(px, py);
  }
  g.setColour(colour);
  g.strokePath(path, juce::PathStrokeType(1.5f * theme.scale,
                                          juce::PathStrokeType::curved));
}
} // namespace hungryghost
