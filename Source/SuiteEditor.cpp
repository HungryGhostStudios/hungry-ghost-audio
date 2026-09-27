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
    : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&theme);
  setSize(940, 660);
  setResizable(true, true);
  setResizeLimits(760, 534, 1410, 990);
  getConstrainer()->setFixedAspectRatio(940. / 660.);
  for (int i = 0; i < 8; ++i) {
    const bool parameter = i < processor.product.controlCount || i >= 6;
    if (!parameter)
      continue;
    const juce::String id = i < 6    ? "control" + juce::String(i)
                            : i == 6 ? "mix"
                                     : "output";
    const auto *control = i < 6 ? &processor.product.controls[i] : nullptr;
    knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 28);
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
    knobs[i].textFromValueFunction = [control, i](double v) {
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
  resetButton.onClick = [this] { processor.factoryReset(); };
  bypassButton.setClickingTogglesState(true);
  bypassAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor.state, "bypass", bypassButton);
  for (int i = 0; i < 4096; ++i)
    window[i] = .5f - .5f * std::cos(juce::MathConstants<float>::twoPi *
                                     static_cast<float>(i) / 4095.f);
  spectrumPre.fill(-90);
  spectrumPost.fill(-90);
  lastFrame = juce::Time::getMillisecondCounterHiRes();
  startTimerHz(30);
}
SuiteEditor::~SuiteEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void SuiteEditor::resized() {
  float s = static_cast<float>(getWidth()) / 940.f;
  theme.scale = s;
  display = {35 * s, 105 * s, 870 * s, 235 * s};
  bankA.setBounds(650 * s, 39 * s, 42 * s, 30 * s);
  bankB.setBounds(695 * s, 39 * s, 42 * s, 30 * s);
  copy.setBounds(742 * s, 39 * s, 65 * s, 30 * s);
  bypassButton.setBounds(815 * s, 39 * s, 88 * s, 30 * s);
  resetButton.setBounds(785 * s, 567 * s, 110 * s, 31 * s);
  int count = processor.product.controlCount + 2;
  float gap = 830.f / static_cast<float>(count);
  int n = 0;
  for (int i = 0; i < 8; ++i) {
    if (!attachments[i])
      continue;
    float x = 55.f + gap * n++;
    knobs[i].setBounds(juce::roundToInt(x * s), juce::roundToInt(387 * s),
                       juce::roundToInt(gap * s), juce::roundToInt(146 * s));
    labels[i].setBounds(juce::roundToInt(x * s), juce::roundToInt(540 * s),
                        juce::roundToInt(gap * s), juce::roundToInt(22 * s));
  }
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
void SuiteEditor::paint(juce::Graphics &g) {
  float s = theme.scale;
  theme.paintChassis(g, getLocalBounds().toFloat());
  g.setColour(GhostTheme::muted());
  g.setFont(font(11 * s));
  g.drawText("HUNGRY GHOST / " +
                 juce::String(processor.product.family).toUpperCase(),
             42 * s, 25 * s, 580 * s, 18 * s, juce::Justification::left);
  g.setColour(GhostTheme::ink());
  g.setFont(font(30 * s, true));
  g.drawText(processor.product.name, 42 * s, 44 * s, 570 * s, 40 * s,
             juce::Justification::left);
  theme.paintDisplay(g, display);
  auto plot = display.reduced(22 * s, 25 * s);
  g.setColour(GhostTheme::line().withAlpha(.5f));
  for (int j = 1; j < 4; ++j)
    g.drawHorizontalLine(
        static_cast<int>(plot.getY() + plot.getHeight() * j / 4.f), plot.getX(),
        plot.getRight());
  for (float hz : {100.f, 1000.f, 10000.f}) {
    float x =
        plot.getX() + plot.getWidth() * std::log(hz / 20.f) / std::log(1000.f);
    g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
  }
  drawSpectrum(g, spectrumPre, GhostTheme::muted().withAlpha(.4f));
  drawSpectrum(g, spectrumPost, GhostTheme::accent());
  g.setFont(font(10 * s));
  g.setColour(GhostTheme::muted());
  g.drawText("20 Hz", plot.getX(), display.getBottom() - 21 * s, 80 * s, 15 * s,
             juce::Justification::left);
  g.drawText("INPUT / OUTPUT SPECTRUM", plot.getCentreX() - 120 * s,
             display.getY() + 8 * s, 240 * s, 15 * s,
             juce::Justification::centred);
  g.drawText("20 kHz", plot.getRight() - 80 * s, display.getBottom() - 21 * s,
             80 * s, 15 * s, juce::Justification::right);
  g.setFont(font(12 * s));
  g.drawText("IN  " + juce::String(decibel(peakIn), 1) + " dBFS", 47 * s,
             357 * s, 215 * s, 20 * s, juce::Justification::left);
  g.drawText("GR  " + juce::String(gr, 1) + " dB", 362 * s, 357 * s, 215 * s,
             20 * s, juce::Justification::centred);
  g.drawText("OUT  " + juce::String(decibel(peakOut), 1) + " dBFS", 677 * s,
             357 * s, 215 * s, 20 * s, juce::Justification::right);
  g.setColour(GhostTheme::muted());
  g.setFont(font(11 * s));
  g.drawText(processor.product.description, 45 * s, 572 * s, 715 * s, 26 * s,
             juce::Justification::left);
  g.setFont(font(10 * s));
  g.drawText("HUNGRY GHOST AUDIO / 0.1.0", 42 * s, 620 * s, 845 * s, 18 * s,
             juce::Justification::centred);
}
void SuiteEditor::timerCallback() {
  peakIn = std::max(processor.inputPeak.load(), peakIn * .87f);
  peakOut = std::max(processor.outputPeak.load(), peakOut * .87f);
  gr = processor.reduction.load();
  bankA.setToggleState(processor.selectedBank() == 0,
                       juce::dontSendNotification);
  bankB.setToggleState(processor.selectedBank() == 1,
                       juce::dontSendNotification);
  std::array<float, 4096> pre{}, post{};
  if (processor.popAnalysis(pre, post)) {
    fftPre.fill(0);
    fftPost.fill(0);
    for (int i = 0; i < 4096; ++i) {
      fftPre[i] = pre[i] * window[i];
      fftPost[i] = post[i] * window[i];
    }
    fft.performFrequencyOnlyForwardTransform(fftPre.data());
    fft.performFrequencyOnlyForwardTransform(fftPost.data());
    for (int i = 0; i < 2048; ++i) {
      float a = decibel(fftPre[i] * 4.f / 4096.f),
            b = decibel(fftPost[i] * 4.f / 4096.f);
      spectrumPre[i] +=
          (a > spectrumPre[i] ? .7f : .16f) * (a - spectrumPre[i]);
      spectrumPost[i] +=
          (b > spectrumPost[i] ? .7f : .16f) * (b - spectrumPost[i]);
    }
    lastFrame = juce::Time::getMillisecondCounterHiRes();
  } else if (juce::Time::getMillisecondCounterHiRes() - lastFrame > 300) {
    for (auto &x : spectrumPre)
      x += .1f * (-90 - x);
    for (auto &x : spectrumPost)
      x += .1f * (-90 - x);
  }
  repaint();
}
} // namespace hungryghost
