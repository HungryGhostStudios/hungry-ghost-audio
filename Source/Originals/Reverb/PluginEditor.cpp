#include "PluginEditor.h"
#include <cmath>

namespace {
juce::Font font(float size, bool bold = false) {
  return juce::Font(juce::FontOptions(
      "Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain));
}
juce::String valueText(double value, const juce::String &id,
                       const juce::String &unit) {
  if (id == "decay")
    return juce::String(value, 2) + " s";
  if (id == "tone")
    return (value > 0 ? "+" : "") + juce::String(value, 0);
  if (id == "low" || id == "high")
    return juce::String(value, 2) + " x";
  if (unit == "Hz" && value >= 1000)
    return juce::String(value / 1000, 1) + " kHz";
  return juce::String(
             value,
             (id == "rate" ? 2 : (id == "input" || id == "output" ? 1 : 0))) +
         (unit.isEmpty() ? "" : " " + unit);
}
} // namespace
AfterKnob::AfterKnob(AfterProcessor &p, const char *id,
                     const juce::String &name, const juce::String &hint,
                     const juce::String &unit, bool isLarge)
    : large(isLarge) {
  title.setText(name.toUpperCase(), juce::dontSendNotification);
  title.setJustificationType(juce::Justification::centred);
  title.setColour(juce::Label::textColourId,
                  isLarge ? AfterLook::accent() : AfterLook::ink());
  caption.setText(hint, juce::dontSendNotification);
  caption.setJustificationType(juce::Justification::centred);
  slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f, true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, isLarge ? 160 : 125,
                         isLarge ? 48 : 32);
  slider.getProperties().set("large", isLarge);
  slider.setTooltip(
      hint +
      ". Drag to adjust; double-click to reset; click the value to type.");
  const juce::String key(id);
  addAndMakeVisible(title);
  addAndMakeVisible(slider);
  addAndMakeVisible(caption);
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.state, id, slider);
  // JUCE attachments install parameter formatters; apply the UI units after
  // them.
  slider.textFromValueFunction = [key, unit](double v) {
    return valueText(v, key, unit);
  };
  slider.valueFromTextFunction = [](const juce::String &text) {
    return text.getDoubleValue() * (text.containsIgnoreCase("kHz") ? 1000 : 1);
  };
  slider.setComponentID(id);
  slider.updateText();
  slider.setDoubleClickReturnValue(
      true, p.state.getParameter(id)->convertFrom0to1(
                p.state.getParameter(id)->getDefaultValue()));
}
void AfterKnob::resized() {
  auto area = getLocalBounds();
  const float sc = getWidth() / (large ? 177.0f : 165.0f);
  title.setBounds(area.removeFromTop(juce::roundToInt(23 * sc)));
  caption.setBounds(area.removeFromBottom(juce::roundToInt(22 * sc)));
  slider.setBounds(area);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, getWidth(),
                         juce::roundToInt((large ? 48 : 34) * sc));
}
void TailView::paint(juce::Graphics &g) {
  const auto p = processor.readParameters();
  const float viewScale = getWidth() / 624.0f;
  if (auto *theme = dynamic_cast<AfterLook *>(&getLookAndFeel()))
    theme->paintDisplay(g, getLocalBounds().toFloat());
  const juce::Graphics::ScopedSaveState save(g);
  g.addTransform(juce::AffineTransform::scale(viewScale));
  const float viewHeight = getHeight() / viewScale;
  const auto outer = juce::Rectangle<float>(0, 0, 624, viewHeight);
  constexpr float sc = 1;
  g.setFont(font(14 * sc));
  g.setColour(AfterLook::muted());
  g.drawText("TAIL SHAPE",
             outer.reduced(31 * sc, 22 * sc).removeFromTop(18 * sc),
             juce::Justification::left);
  g.setColour(AfterLook::ink());
  g.drawText(p.freeze ? "Tail held" : juce::String(p.decay, 2) + " s to -60 dB",
             outer.reduced(31 * sc, 22 * sc).removeFromTop(18 * sc),
             juce::Justification::right);
  const auto plot = outer.withTrimmedTop(52)
                        .withTrimmedBottom(61)
                        .withTrimmedLeft(62)
                        .withTrimmedRight(31);
  // Continuous fit keeps the curve from snapping at whole-second boundaries.
  const float maxTime = std::max(2.0f, p.decay * 1.5f);
  const auto x = [&](float t) {
    return plot.getX() + plot.getWidth() * t / maxTime;
  };
  const auto y = [&](float db) {
    return plot.getY() + plot.getHeight() * juce::jlimit(0.0f, 1.0f, -db / 60);
  };
  for (const float db : {0.0f, -30.0f, -60.0f}) {
    g.setColour(AfterLook::line().withAlpha(.7f));
    g.drawHorizontalLine(static_cast<int>(y(db)), plot.getX(), plot.getRight());
    g.setColour(AfterLook::muted());
    g.drawText(juce::String(db, 0), static_cast<int>(outer.getX() + 31),
               static_cast<int>(y(db) - 7), 30, 14, juce::Justification::left);
  }
  for (int tick = 0; tick < 4; ++tick) {
    const float t = maxTime * tick / 3.0f;
    const int xx = static_cast<int>(x(t));
    g.drawText(juce::String(t, 1) + " s", xx - (tick == 3 ? 48 : 0),
               static_cast<int>(plot.getBottom() + 4), 48, 16,
               tick == 3 ? juce::Justification::right
                         : juce::Justification::left);
  }
  std::array<juce::Path, 3> paths;
  for (int i = 0; i <= 160; ++i) {
    const float t = maxTime * i / 160.0f;
    const float db =
        p.freeze ? -12.0f
                 : -60 * std::max(0.0f, t - p.preDelay * .001f) / p.decay;
    const std::array<float, 3> ratios{
        p.lowRatio, 1.0f, p.highRatio * std::pow(2.0f, p.tone / 120.0f)};
    for (int j = 0; j < 3; ++j) {
      const auto point = juce::Point<float>(x(t), y(db / ratios[j]));
      if (i == 0)
        paths[j].startNewSubPath(point);
      else
        paths[j].lineTo(point);
    }
  }
  auto fill = paths[1];
  fill.lineTo(plot.getRight(), plot.getBottom());
  fill.lineTo(plot.getX(), plot.getBottom());
  fill.closeSubPath();
  g.setGradientFill(juce::ColourGradient(AfterLook::accent().withAlpha(.10f),
                                         plot.getX(), plot.getY(),
                                         AfterLook::accent().withAlpha(0.0f),
                                         plot.getX(), plot.getBottom(), false));
  g.fillPath(fill);
  for (int j = 0; j < 8; ++j) {
    const float xx = plot.getX() + j * (22 + p.size * .4f) / 8;
    g.setColour(AfterLook::muted().withAlpha(.2f));
    g.drawVerticalLine(static_cast<int>(xx),
                       plot.getBottom() - plot.getHeight() *
                                              (.25f + p.early / 180.0f) *
                                              std::exp(-j * .22f),
                       plot.getBottom());
  }
  for (int j = 0; j < 3; ++j) {
    g.setColour(
        AfterLook::accent().withAlpha(j == 1 ? 1.0f : (j == 0 ? .4f : .7f)));
    if (j == 1)
      g.strokePath(paths[j], juce::PathStrokeType(1.7f * sc));
    else {
      juce::Path dashed;
      const float dashes[2] = {j == 0 ? 5.0f : 1.0f, 4.0f};
      juce::PathStrokeType(1.1f).createDashedStroke(dashed, paths[j], dashes,
                                                    2);
      g.fillPath(dashed);
    }
  }
  g.setColour(AfterLook::muted());
  g.drawText("Low  - -      Mid  __      High  . .", 31,
             juce::roundToInt(viewHeight - 38), 240, 16,
             juce::Justification::left);
  g.drawText(juce::String(p.preDelay, 0) + " ms pre-delay", 624 - 175,
             juce::roundToInt(viewHeight - 38), 144, 16,
             juce::Justification::right);
}
AfterEditor::AfterEditor(AfterProcessor &p)
    : AudioProcessorEditor(p), processor(p), tail(p) {
  setLookAndFeel(&look);
  addAndMakeVisible(licenceButton);
  setOpaque(true);
  const char *ids[] = {"decay", "size", "tone", "motion", "predelay", "mix"};
  const char *titles[] = {"Decay",  "Size",      "Tone",
                          "Motion", "Pre-delay", "Mix"};
  const char *hints[] = {"Tail length",   "Room scale",        "Warm / bright",
                         "Tail movement", "Source separation", "Dry / wet"};
  const char *units[] = {"s", "%", "", "%", "ms", "%"};
  for (int i = 0; i < 6; ++i) {
    knobs[i] = std::make_unique<AfterKnob>(p, ids[i], titles[i], hints[i],
                                           units[i], i == 0);
    addAndMakeVisible(*knobs[i]);
  }
  addAndMakeVisible(tail);
  const char *names[] = {"Room", "Chamber", "Hall", "Plate", "Cloud"};
  for (int i = 0; i < 5; ++i) {
    characters[i].setButtonText(names[i]);
    characters[i].getProperties().set("hardware", true);
    characters[i].onClick = [this, i] {
      processor.setValue("character", static_cast<float>(i));
    };
    addAndMakeVisible(characters[i]);
  }
  for (int i = 0; i < 5; ++i)
    presets.addItem(processor.getProgramName(i), i + 1);
  presets.onChange = [this] {
    if (presets.getSelectedId() > 0)
      processor.setCurrentProgram(presets.getSelectedId() - 1);
  };
  addAndMakeVisible(presets);
  for (auto *button :
       {&bankA, &bankB, &copy, &power, &freeze, &mixLock, &shape})
    addAndMakeVisible(button);
  bankA.getProperties().set("hardware", true);
  bankB.getProperties().set("hardware", true);
  bankA.onClick = [this] { processor.selectBank(0); };
  bankB.onClick = [this] { processor.selectBank(1); };
  copy.onClick = [this] { processor.copyBank(); };
  copy.setTooltip("Copy current settings to the other comparison bank");
  power.setClickingTogglesState(true);
  freeze.setClickingTogglesState(true);
  mixLock.setClickingTogglesState(true);
  mixLock.setTooltip("Keep Mix unchanged when loading a preset");
  mixLock.onClick = [this] {
    processor.setMixLocked(mixLock.getToggleState());
  };
  shape.onClick = [this] { showShape(!shapeOpen); };
  freeze.setTooltip(
      "Capture the current late tail, stop input to the reverb, and hold it");
  freezeAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, "freeze", freeze);
  powerAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, "bypass", power);
  const char *tabNames[] = {"Space", "Color", "Motion"};
  for (int i = 0; i < 3; ++i) {
    tabs[i].setButtonText(tabNames[i]);
    tabs[i].onClick = [this, i] { selectTab(i); };
    addChildComponent(tabs[i]);
  }
  const auto addDetail = [&](const char *id, const char *name, int category,
                             const char *unit) {
    auto d = std::make_unique<Detail>();
    d->category = category;
    d->label.setText(name, juce::dontSendNotification);
    d->slider.setSliderStyle(juce::Slider::LinearHorizontal);
    d->slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 85, 24);
    d->slider.getProperties().set("detail", true);
    addChildComponent(d->label);
    addChildComponent(d->slider);
    d->attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.state, id, d->slider);
    d->slider.textFromValueFunction = [key = juce::String(id),
                                       suffix = juce::String(unit)](double v) {
      return valueText(v, key, suffix);
    };
    d->slider.valueFromTextFunction = [](const juce::String &t) {
      return t.getDoubleValue() * (t.containsIgnoreCase("kHz") ? 1000 : 1);
    };
    d->slider.setComponentID(id);
    d->slider.updateText();
    details.push_back(std::move(d));
  };
  addDetail("early", "Early reflections", 0, "%");
  addDetail("diffusion", "Diffusion", 0, "%");
  addDetail("width", "Stereo width", 0, "%");
  addDetail("low", "Low decay", 1, "");
  addDetail("high", "High decay", 1, "");
  addDetail("lowCross", "Low crossover", 1, "Hz");
  addDetail("highCross", "High crossover", 1, "Hz");
  addDetail("lowcut", "Wet low cut", 1, "Hz");
  addDetail("highcut", "Wet high cut", 1, "Hz");
  addDetail("rate", "Motion rate", 2, "Hz");
  addDetail("duck", "Ducking", 2, "%");
  addDetail("release", "Duck release", 2, "ms");
  for (int i = 0; i < 2; ++i) {
    trims[i].setSliderStyle(juce::Slider::LinearHorizontal);
    trims[i].setTextBoxStyle(juce::Slider::TextBoxLeft, false, 74, 24);
    trims[i].setColour(juce::Slider::textBoxBackgroundColourId,
                       juce::Colour(0xff0d1210));
    trims[i].setColour(juce::Slider::textBoxOutlineColourId,
                       AfterLook::muted().withAlpha(.5f));
    trims[i].getProperties().set("detail", true);
    addAndMakeVisible(trims[i]);
    trimAttachments[i] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.state, i == 0 ? "input" : "output", trims[i]);
    trims[i].textFromValueFunction = [](double v) {
      return juce::String(v, 1) + " dB";
    };
    trims[i].setComponentID(i == 0 ? "input" : "output");
    trims[i].updateText();
  }
  // Constraining the initial size can invoke resized() synchronously. Install
  // all controls before enabling host resizing.
  setResizable(true, true);
  setResizeLimits(760, 562, 1400, 1035);
  getConstrainer()->setFixedAspectRatio(920.0 / 680.0);
  setSize(920, 680);
  timerCallback();
  startTimerHz(24);
}
AfterEditor::~AfterEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void AfterEditor::showShape(bool show) {
  if (shapeOpen == show)
    return;
  shapeOpen = show;
  shape.setButtonText(show ? "Shape -" : "Shape +");
  const double ratio = 920.0 / (show ? 870.0 : 680.0);
  const int previousWidth = getWidth();
  getConstrainer()->setFixedAspectRatio(ratio);
  setResizeLimits(760, show ? 719 : 562, 1400, show ? 1324 : 1035);
  setSize(previousWidth, juce::roundToInt(previousWidth / ratio));
  selectTab(shapeTab);
  resized();
  repaint();
}
void AfterEditor::selectTab(int index) {
  shapeTab = index;
  for (int i = 0; i < 3; ++i) {
    tabs[i].setVisible(shapeOpen);
    tabs[i].setToggleState(i == index, juce::dontSendNotification);
  }
  for (auto &detail : details) {
    detail->label.setVisible(shapeOpen && detail->category == index);
    detail->slider.setVisible(shapeOpen && detail->category == index);
  }
  resized();
  repaint();
}
void AfterEditor::timerCallback() {
  const auto p = processor.readParameters();
  for (int i = 0; i < 5; ++i)
    characters[i].setToggleState(i == p.character, juce::dontSendNotification);
  bankA.setToggleState(processor.selectedBank() == 0,
                       juce::dontSendNotification);
  bankB.setToggleState(processor.selectedBank() == 1,
                       juce::dontSendNotification);
  copy.setButtonText(processor.selectedBank() == 0 ? "A > B" : "B > A");
  mixLock.setToggleState(processor.isMixLocked(), juce::dontSendNotification);
  power.setButtonText(p.bypass ? "BYPASS" : "ON");
  presets.setSelectedId(processor.getCurrentProgram() + 1,
                        juce::dontSendNotification);
  tail.repaint();
}
void AfterEditor::paint(juce::Graphics &g) {
  const float s = look.scale;
  look.paintChassis(g, getLocalBounds().toFloat());
  const auto copper = juce::Colour(0xffbd825e);
  const auto rect = [s](float x, float y, float w, float h) {
    return juce::Rectangle<float>(x * s, y * s, w * s, h * s);
  };
  // Plates follow the live control bounds, including the expanded Shape view.
  // Their edges sit outside the controls and never cross the tail display.
  auto characterPlate = characters.front().getBounds();
  for (const auto &button : characters)
    characterPlate = characterPlate.getUnion(button.getBounds());
  look.paintPanel(g, characterPlate.toFloat().expanded(7 * s, 5 * s), copper);
  look.paintPanel(g, knobs[0]->getBounds().toFloat().expanded(9 * s, 3 * s),
                  copper);
  auto controlPlate = knobs[1]->getBounds();
  for (int i = 2; i < 6; ++i)
    controlPlate = controlPlate.getUnion(knobs[i]->getBounds());
  look.paintPanel(g, controlPlate.toFloat().expanded(4 * s, 4 * s),
                  AfterLook::accent());
  const auto seam = [&](float y) {
    g.setColour(juce::Colours::black.withAlpha(.8f));
    g.drawLine(20 * s, y, getWidth() - 20 * s, y, 2 * s);
    g.setColour(juce::Colours::white.withAlpha(.10f));
    g.drawLine(20 * s, y + 2 * s, getWidth() - 20 * s, y + 2 * s, .8f * s);
  };
  seam(86 * s);
  g.setFont(font(9.5f * s, true));
  g.setColour(AfterLook::muted());
  g.drawText("HUNGRY GHOST AUDIO", rect(44, 15, 285, 12),
             juce::Justification::left);
  look.paintWordmark(g, "REVERB", rect(41, 28, 288, 39), 36 * s);
  g.setFont(font(9 * s, true));
  g.setColour(AfterLook::muted().interpolatedWith(copper, .3f));
  g.drawText("ALGORITHMIC SPACE / DEVOUR THE SILENCE", rect(44, 65, 306, 12),
             juce::Justification::left);
  look.paintSpectralMark(g, rect(359, 18, 77, 59), copper);
  const int footer = getHeight() - static_cast<int>(52 * s);
  look.paintPanel(g, {20 * s, (float)footer, getWidth() - 40 * s, 38 * s},
                  copper);
  seam((float)footer);
  g.setColour(AfterLook::muted());
  g.setFont(font(11 * s));
  g.drawText("IN", static_cast<int>(28 * s), footer, 25,
             static_cast<int>(40 * s), juce::Justification::left);
  g.drawText("OUT", getWidth() - static_cast<int>(179 * s), footer, 35,
             static_cast<int>(40 * s), juce::Justification::left);
  const int actionY = shape.getY() - static_cast<int>(12 * s);
  seam((float)actionY);
  if (shapeOpen) {
    const float panelY = shape.getBottom() + 13 * s;
    look.paintPanel(g, {20 * s, panelY, getWidth() - 40 * s,
                       juce::jmax(0.f, footer - panelY - 4 * s)},
                    AfterLook::accent());
    seam(shape.getBottom() + 12 * s);
  }
}
void AfterEditor::resized() {
  look.scale = static_cast<float>(getWidth()) / 920.0f;
  const float s = look.scale;
  const auto rect = [s](float x, float y, float w, float h) {
    return juce::Rectangle<int>(
        juce::roundToInt(x * s), juce::roundToInt(y * s),
        juce::roundToInt(w * s), juce::roundToInt(h * s));
  };
  presets.setBounds(rect(460, 26, 225, 34));
  bankA.setBounds(rect(701, 24, 31, 34));
  bankB.setBounds(rect(738, 24, 31, 34));
  copy.setBounds(rect(776, 24, 70, 34));
  power.setBounds(rect(851, 24, 57, 34));
  for (int i = 0; i < 5; ++i)
    characters[i].setBounds(rect(37 + i * 79.0f, 103, 76, 34));
  const int reserve = static_cast<int>((shapeOpen ? 190 : 0) * s);
  const int baseHeight = getHeight() - reserve;
  const float available = (baseHeight - 80 * s - 40 * s) / s;
  const float stretch = std::max(.82f, available / 560.0f);
  const float heroY = 148, heroH = 219 * stretch;
  tail.setBounds(rect(28, heroY, 624, heroH));
  knobs[0]->setBounds(rect(692, heroY - 7, 177, heroH + 18));
  const float smallY = heroY + heroH + 22;
  for (int i = 1; i < 6; ++i)
    knobs[i]->setBounds(
        rect(24 + (i - 1) * 176.0f, smallY, 165, 174 * stretch));
  const int actionTop = baseHeight - static_cast<int>(101 * s);
  freeze.setBounds(static_cast<int>(28 * s), actionTop,
                   static_cast<int>(95 * s), static_cast<int>(35 * s));
  mixLock.setBounds(static_cast<int>(132 * s), actionTop,
                    static_cast<int>(110 * s), static_cast<int>(35 * s));
  shape.setBounds(getWidth() - static_cast<int>(130 * s), actionTop,
                  static_cast<int>(104 * s), static_cast<int>(35 * s));
  if (shapeOpen) {
    const int detailY = baseHeight - static_cast<int>(43 * s);
    for (int i = 0; i < 3; ++i)
      tabs[i].setBounds(static_cast<int>((28 + i * 80) * s), detailY,
                        static_cast<int>(75 * s), static_cast<int>(33 * s));
    int count = 0;
    for (auto &d : details)
      if (d->category == shapeTab) {
        const int col = count % 3, row = count / 3;
        const int x = static_cast<int>((28 + col * 293) * s),
                  y = detailY + static_cast<int>((43 + row * 68) * s);
        d->label.setBounds(x, y, static_cast<int>(263 * s),
                           static_cast<int>(22 * s));
        d->slider.setBounds(x, y + static_cast<int>(25 * s),
                            static_cast<int>(263 * s),
                            static_cast<int>(28 * s));
        ++count;
      }
  }
  for (auto &trim : trims)
    trim.setTextBoxStyle(juce::Slider::TextBoxLeft, false,
                         juce::roundToInt(74 * s), juce::roundToInt(24 * s));
  const int footer = getHeight() - static_cast<int>(52 * s);
  licenceButton.setBounds(getWidth() / 2 - juce::roundToInt(125 * s),
                          footer + juce::roundToInt(8 * s),
                          juce::roundToInt(250 * s), juce::roundToInt(25 * s));
  trims[0].setBounds(static_cast<int>(59 * s), footer + static_cast<int>(8 * s),
                     static_cast<int>(116 * s), static_cast<int>(25 * s));
  trims[1].setBounds(getWidth() - static_cast<int>(140 * s),
                     footer + static_cast<int>(8 * s),
                     static_cast<int>(116 * s), static_cast<int>(25 * s));
}
