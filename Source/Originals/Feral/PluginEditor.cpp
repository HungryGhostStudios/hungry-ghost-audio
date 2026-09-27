#include "PluginEditor.h"

namespace {
using Theme = hungryghost::GhostTheme;
juce::Font font(float h, bool bold = false) {
  return juce::Font(juce::FontOptions(
      "Segoe UI", h, bold ? juce::Font::bold : juce::Font::plain));
}
const std::array<juce::Colour, 8> colours{
    {juce::Colour(0xffd2bd8b), juce::Colour(0xffa2e4cf),
     juce::Colour(0xff9cbcdf), juce::Colour(0xffbdabd4),
     juce::Colour(0xffd69f97), juce::Colour(0xffc5cf99),
     juce::Colour(0xff83c8c8), juce::Colour(0xffd2aa86)}};
float level(float v) { return juce::Decibels::gainToDecibels(v, -100.0f); }
juce::String textValue(double v, const juce::String &unit) {
  if (unit == "Hz")
    return v >= 1000 ? juce::String(v / 1000, 2) + " kHz"
                     : juce::String(v, 0) + " Hz";
  if (unit == ":1")
    return juce::String(v, 2) + ":1";
  if (unit == "Q")
    return juce::String(v, 2);
  if (unit == "ms")
    return juce::String(v, v < 10 ? 2 : 0) + " ms";
  return juce::String(v, unit == "%" ? 0 : 1) + " " + unit;
}
} // namespace
FeralControl::FeralControl(FeralProcessor &p, juce::String name, bool isRotary)
    : processor(p), rotary(isRotary) {
  title.setText(name.toUpperCase(), juce::dontSendNotification);
  title.setJustificationType(juce::Justification::centred);
  title.setColour(juce::Label::textColourId, Theme::muted());
  slider.setSliderStyle(rotary ? juce::Slider::RotaryVerticalDrag
                               : juce::Slider::LinearHorizontal);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f, true);
  slider.getProperties().set("detail", !rotary);
  addAndMakeVisible(title);
  addAndMakeVisible(slider);
}
void FeralControl::bind(const juce::String &id, const juce::String &unit) {
  attachment.reset();
  slider.setComponentID(id);
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.state, id, slider);
  slider.textFromValueFunction = [unit](double v) {
    return textValue(v, unit);
  };
  slider.valueFromTextFunction = [](const juce::String &t) {
    return t.getDoubleValue() * (t.containsIgnoreCase("k") ? 1000 : 1);
  };
  const auto *p = processor.state.getParameter(id);
  slider.setDoubleClickReturnValue(true,
                                   p->convertFrom0to1(p->getDefaultValue()));
  slider.setTooltip(
      title.getText() +
      ": drag to adjust. Click the value to type. Double-click to reset.");
  slider.updateText();
}
void FeralControl::resized() {
  const float sc = getWidth() / (rotary ? 145.0f : 140.0f);
  auto area = getLocalBounds();
  title.setBounds(
      area.removeFromTop(juce::roundToInt((rotary ? 21 : 13) * sc)));
  slider.setBounds(area);
  slider.setTextBoxStyle(
      rotary ? juce::Slider::TextBoxBelow : juce::Slider::TextBoxRight, false,
      juce::roundToInt((rotary ? 145 : 85) * sc),
      std::min(area.getHeight(), juce::roundToInt((rotary ? 33 : 23) * sc)));
}
FrequencyGraph::FrequencyGraph(FeralProcessor &p) : processor(p) {
  // Text line boxes include ascenders, descenders and side bearings. Cache
  // the actual digit outlines, centred on their visible ink at the origin.
  for (int i = 0; i < feral::maxBands; ++i) {
    juce::GlyphArrangement glyph;
    glyph.addLineOfText(font(10, true), juce::String(i + 1), 0, 0);
    glyph.createPath(numberOutlines[i]);
    const auto centre = numberOutlines[i].getBounds().getCentre();
    numberOutlines[i].applyTransform(
        juce::AffineTransform::translation(-centre.x, -centre.y));
  }
  preDB.fill(-100);
  postDB.fill(-100);
  setMouseCursor(juce::MouseCursor::CrosshairCursor);
}
juce::Rectangle<float> FrequencyGraph::plot() const {
  const float sc = getWidth() / 996.0f;
  return getLocalBounds()
      .toFloat()
      .withTrimmedLeft(48 * sc)
      .withTrimmedRight(27 * sc)
      .withTrimmedTop(45 * sc)
      .withTrimmedBottom(45 * sc);
}
float FrequencyGraph::x(float f) const {
  auto r = plot();
  return r.getX() + r.getWidth() * std::log(f / 20) / std::log(1000.0f);
}
float FrequencyGraph::y(float v) const {
  auto r = plot();
  return r.getCentreY() - v / 36 * r.getHeight();
}
float FrequencyGraph::frequency(float xx) const {
  auto r = plot();
  return 20 * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f,
                                             (xx - r.getX()) / r.getWidth()));
}
float FrequencyGraph::gain(float yy) const {
  auto r = plot();
  return juce::jlimit(-18.0f, 18.0f,
                      (r.getCentreY() - yy) / r.getHeight() * 36);
}
void FrequencyGraph::updateSpectrum() {
  if (!processor.popAnalysis(pre, post)) {
    // Wait across normal frame intervals; fade only after the host stops.
    if (juce::Time::getMillisecondCounterHiRes() - lastSpectrumTime > 250) {
      for (auto &v : preDB)
        v = std::max(-100.0f, v - 1.5f);
      for (auto &v : postDB)
        v = std::max(-100.0f, v - 1.5f);
    }
    return;
  }
  spectrumActive = true;
  lastSpectrumTime = juce::Time::getMillisecondCounterHiRes();
  auto analyse = [&](auto &samples, auto &result) {
    fftData.fill(0);
    for (int i = 0; i < FeralProcessor::analysisSize; ++i)
      fftData[i] = samples[i] *
                   (.5f - .5f * std::cos(juce::MathConstants<float>::twoPi * i /
                                         (FeralProcessor::analysisSize - 1)));
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    for (size_t i = 0; i < result.size(); ++i) {
      const float target = level(fftData[i] * 4 / FeralProcessor::analysisSize);
      result[i] += (target - result[i]) * (target > result[i] ? .65f : .18f);
    }
  };
  analyse(pre, preDB);
  analyse(post, postDB);
}
void FrequencyGraph::paint(juce::Graphics &g) {
  const float sc = getWidth() / 996.0f;
  const auto r = plot();
  if (auto *t = dynamic_cast<Theme *>(&getLookAndFeel()))
    t->paintDisplay(g, getLocalBounds().toFloat());
  const auto p = processor.readParameters();
  g.setFont(font(11 * sc, true));
  g.setColour(Theme::muted());
  g.drawText("FREQUENCY / DYNAMICS", (int)(30 * sc), (int)(19 * sc),
             (int)(220 * sc), (int)(17 * sc), juce::Justification::left);
  g.drawText(juce::String::fromUTF8(
                 "PRE / POST (L)   |   -90..0 dBFS   |   EQ ±18 dB"),
             (int)(getWidth() - 420 * sc), (int)(19 * sc), (int)(386 * sc),
             (int)(17 * sc), juce::Justification::right);
  for (float db : {-18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f}) {
    g.setColour(Theme::line().withAlpha(db == 0 ? .9f : .36f));
    g.drawLine(r.getX(), y(db), r.getRight(), y(db), sc);
    g.setColour(Theme::muted().withAlpha(.7f));
    g.drawText((db > 0 ? "+" : "") + juce::String(db, 0), (int)(14 * sc),
               (int)(y(db) - 7 * sc), (int)(28 * sc), (int)(14 * sc),
               juce::Justification::right);
  }
  for (float f : {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f,
                  5000.0f, 10000.0f, 20000.0f}) {
    g.setColour(Theme::line().withAlpha(.3f));
    g.drawLine(x(f), r.getY(), x(f), r.getBottom(), sc);
    g.setColour(Theme::muted().withAlpha(.75f));
    g.drawText(f >= 1000 ? juce::String(f / 1000, 0) + "k" : juce::String(f, 0),
               (int)(x(f) - 20 * sc), (int)(r.getBottom() + 9 * sc),
               (int)(40 * sc), (int)(14 * sc), juce::Justification::centred);
  }
  {
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(r.toNearestInt());
    const double sr =
        processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000;
    if (spectrumActive) {
      auto spectrumPath = [&](auto &bins) {
        juce::Path path;
        path.startNewSubPath(r.getX(), r.getBottom());
        for (int i = 0; i <= 800; ++i) {
          const float f = 20 * std::pow(1000.0f, i / 800.0f);
          const double bin = f / sr * FeralProcessor::analysisSize;
          const int index = juce::jlimit(1, (int)bins.size() - 3, (int)bin);
          const float t = (float)std::clamp(bin - index, 0.0, 1.0);
          const float d0 = bins[index] - bins[index - 1],
                      d1 = bins[index + 1] - bins[index],
                      d2 = bins[index + 2] - bins[index + 1];
          const auto slope = [](float a, float b) {
            return a * b > 0 ? 2 * a * b / (a + b) : 0.0f;
          };
          // Monotone Hermite interpolation rounds joins without inventing
          // overshoot or raising a narrow peak above the measured bins.
          const float v = (2 * t * t * t - 3 * t * t + 1) * bins[index] +
                          (t * t * t - 2 * t * t + t) * slope(d0, d1) +
                          (-2 * t * t * t + 3 * t * t) * bins[index + 1] +
                          (t * t * t - t * t) * slope(d1, d2);
          const float yy =
              r.getBottom() -
              r.getHeight() * juce::jlimit(0.0f, 1.0f, (v + 90) / 90);
          path.lineTo(x(f), yy);
        }
        path.lineTo(r.getRight(), r.getBottom());
        path.closeSubPath();
        return path;
      };
      auto a = spectrumPath(preDB), b = spectrumPath(postDB);
      g.setColour(Theme::ink().withAlpha(.07f));
      g.fillPath(a);
      g.setColour(Theme::ink().withAlpha(.18f));
      g.strokePath(a, juce::PathStrokeType(sc));
      g.setColour(Theme::accent().withAlpha(.045f));
      g.fillPath(b);
      g.setColour(Theme::accent().withAlpha(.28f));
      g.strokePath(b, juce::PathStrokeType(sc));
    }
    std::array<juce::Path, 8> paths;
    std::array<feral::Coefficients, feral::maxBands> staticFilters,
        activeFilters;
    for (int j = 0; j < feral::maxBands; ++j) {
      const auto &b = p.bands[j];
      staticFilters[j] =
          feral::filter((int)b.shape, b.frequency, b.q, b.gain, sr);
      activeFilters[j] =
          feral::filter((int)b.shape, b.frequency, b.q,
                        b.gain - processor.reductions[j].load(), sr);
    }
    juce::Path total, active, area;
    // The aggregate traces show stereo EQ only. M/S and L/R bands retain their
    // individual coloured curves, avoiding a misleading scalar stereo sum.
    for (int i = 0; i <= 320; ++i) {
      const float f = 20 * std::pow(1000.0f, i / 320.0f);
      double db = 0, actual = 0;
      for (int j = 0; j < feral::maxBands; ++j) {
        const auto &band = p.bands[j];
        if (band.enabled < .5f)
          continue;
        const float v =
            (float)(20 * std::log10(std::max(
                             1e-8, staticFilters[j].magnitude(f, sr))));
        const float a =
            (float)(20 * std::log10(std::max(
                             1e-8, activeFilters[j].magnitude(f, sr))));
        if (i == 0)
          paths[j].startNewSubPath(x(f), y(v));
        else
          paths[j].lineTo(x(f), y(v));
        if (band.channel < .5f) {
          db += v;
          actual += a;
        }
      }
      if (i == 0) {
        total.startNewSubPath(x(f), y((float)db));
        active.startNewSubPath(x(f), y((float)actual));
      } else {
        total.lineTo(x(f), y((float)db));
        active.lineTo(x(f), y((float)actual));
      }
    }
    for (int j = 0; j < feral::maxBands; ++j) {
      g.setColour(
          colours[j].withAlpha(j == processor.selected.load() ? .75f : .30f));
      g.strokePath(paths[j],
                   juce::PathStrokeType(
                       sc * (j == processor.selected.load() ? 1.7f : 1)));
    }
    area = active;
    for (int i = 320; i >= 0; --i) {
      const float f = 20 * std::pow(1000.0f, i / 320.0f);
      double v = 0;
      for (int j = 0; j < feral::maxBands; ++j)
        if (p.bands[j].enabled > .5f && p.bands[j].channel < .5f)
          v += 20 *
               std::log10(std::max(1e-8, staticFilters[j].magnitude(f, sr)));
      area.lineTo(x(f), y((float)v));
    }
    area.closeSubPath();
    g.setColour(Theme::accent().withAlpha(.12f));
    g.fillPath(area);
    g.setColour(Theme::ink().withAlpha(.36f));
    g.strokePath(total, juce::PathStrokeType(1.25f * sc));
    g.setColour(Theme::accent().withAlpha(.09f));
    g.strokePath(active, juce::PathStrokeType(7 * sc));
    g.setColour(Theme::accent());
    g.strokePath(active, juce::PathStrokeType(2 * sc));
    for (int j = 0; j < feral::maxBands; ++j) {
      const auto &b = p.bands[j];
      if (b.enabled < .5f)
        continue;
      const float xx = x(b.frequency),
                  yy = y(b.shape >= feral::lowCut ? 0 : b.gain);
      const bool selected = j == processor.selected.load();
      const float radius = (selected ? 8 : 6) * sc;
      g.setColour(colours[j].withAlpha(.08f));
      g.fillEllipse(xx - radius * 2, yy - radius * 2, radius * 4, radius * 4);
      g.setColour(Theme::background());
      g.fillEllipse(xx - radius, yy - radius, radius * 2, radius * 2);
      g.setColour(colours[j]);
      g.drawEllipse(xx - radius, yy - radius, radius * 2, radius * 2,
                    sc * 1.7f);
      g.fillPath(numberOutlines[j],
                 juce::AffineTransform::scale(sc).translated(xx, yy));
      if (b.dynamic > .5f && b.shape < feral::lowCut) {
        g.setColour(colours[j].withAlpha(.35f));
        g.drawLine(xx, yy + radius + 3 * sc, xx, y(b.gain - b.range), sc);
        g.drawLine(xx - 4 * sc, y(b.gain - b.range), xx + 4 * sc,
                   y(b.gain - b.range), sc);
      }
    }
  }
  g.setFont(font(10 * sc));
  g.setColour(Theme::muted().withAlpha(.65f));
  g.drawText(
      juce::String::fromUTF8("Double-click to add   •   Wheel: cutoff for "
                             "cuts, Q for others   •   Shift-wheel: Q"),
      (int)(30 * sc), (int)(getHeight() - 22 * sc), (int)(620 * sc),
      (int)(14 * sc), juce::Justification::left);
  g.drawText("Jade: stereo EQ + live reduction", (int)(getWidth() - 270 * sc),
             (int)(getHeight() - 22 * sc), (int)(235 * sc), (int)(14 * sc),
             juce::Justification::right);
}
int FrequencyGraph::hit(juce::Point<float> point) const {
  const auto p = processor.readParameters();
  float best = 18 * getWidth() / 996.0f;
  int found = -1;
  for (int i = 0; i < feral::maxBands; ++i) {
    const auto &b = p.bands[i];
    if (b.enabled < .5f)
      continue;
    const float distance = point.getDistanceFrom(
        {x(b.frequency), y(b.shape >= feral::lowCut ? 0 : b.gain)});
    if (distance < best) {
      best = distance;
      found = i;
    }
  }
  return found;
}
void FrequencyGraph::mouseDown(const juce::MouseEvent &e) {
  dragging = hit(e.position);
  if (dragging < 0)
    return;
  processor.selected.store(dragging);
  if (onSelection)
    onSelection();
  processor.undo.beginNewTransaction("Move band");
  for (const auto *key : {"frequency", "gain"})
    processor.state.getParameter(FeralProcessor::bandID(dragging, key))
        ->beginChangeGesture();
}
void FrequencyGraph::mouseDrag(const juce::MouseEvent &e) {
  if (dragging < 0)
    return;
  processor.setValue(FeralProcessor::bandID(dragging, "frequency"),
                     frequency(e.position.x));
  if (processor.value(FeralProcessor::bandID(dragging, "shape")) <
      feral::lowCut)
    processor.setValue(FeralProcessor::bandID(dragging, "gain"),
                       gain(e.position.y));
  repaint();
}
void FrequencyGraph::mouseUp(const juce::MouseEvent &) {
  if (dragging >= 0)
    for (const auto *key : {"frequency", "gain"})
      processor.state.getParameter(FeralProcessor::bandID(dragging, key))
          ->endChangeGesture();
  dragging = -1;
}
void FrequencyGraph::addBand(float f, float g) {
  for (int i = 0; i < feral::maxBands; ++i)
    if (processor.value(FeralProcessor::bandID(i, "enabled")) < .5f) {
      processor.undo.beginNewTransaction("Add band");
      processor.setValue(FeralProcessor::bandID(i, "frequency"), f);
      processor.setValue(FeralProcessor::bandID(i, "gain"), g);
      processor.setValue(FeralProcessor::bandID(i, "enabled"), 1);
      processor.selected.store(i);
      if (onSelection)
        onSelection();
      repaint();
      return;
    }
}
void FrequencyGraph::mouseDoubleClick(const juce::MouseEvent &e) {
  if (hit(e.position) < 0 && plot().contains(e.position))
    addBand(frequency(e.position.x), gain(e.position.y));
}
void FrequencyGraph::mouseWheelMove(const juce::MouseEvent &e,
                                    const juce::MouseWheelDetails &wheel) {
  const int i = hit(e.position);
  if (i < 0 || wheel.deltaY == 0)
    return;
  if (processor.selected.load() != i) {
    processor.selected.store(i);
    if (onSelection)
      onSelection();
  }
  const bool cut =
      processor.value(FeralProcessor::bandID(i, "shape")) >= feral::lowCut;
  const bool adjustCutoff = cut && !e.mods.isShiftDown();
  const auto id = FeralProcessor::bandID(i, adjustCutoff ? "frequency" : "q");
  auto *param = processor.state.getParameter(id);
  processor.undo.beginNewTransaction(adjustCutoff ? "Cutoff frequency"
                                                  : "Band Q");
  param->beginChangeGesture();
  processor.setValue(id, processor.value(id) * std::exp(wheel.deltaY * 1.5f));
  param->endChangeGesture();
  repaint();
}
FeralEditor::FeralEditor(FeralProcessor &p)
    : AudioProcessorEditor(p), graph(p), processor(p) {
  setLookAndFeel(&theme);
  addAndMakeVisible(licenceButton);
  addAndMakeVisible(graph);
  graph.onSelection = [this] { bindSelection(); };
  for (int i = 0; i < 8; ++i) {
    bands[i].setButtonText(juce::String(i + 1));
    bands[i].getProperties().set("hardware", true);
    bands[i].onClick = [this, i] { select(i); };
    addAndMakeVisible(bands[i]);
    bands[i].setTooltip("Select band " + juce::String(i + 1));
  }
  for (auto *b : {&bus, &add, &remove, &bankA, &bankB, &copy, &power, &dynamic,
                  &enabled, &external, &undo, &redo}) {
    b->getProperties().set("hardware", true);
    addAndMakeVisible(*b);
  }
  bus.onClick = [this] { select(-1); };
  add.onClick = [this] { graph.addBand(); };
  remove.onClick = [this] {
    const int i = processor.selected.load();
    if (i < 0)
      return;
    processor.undo.beginNewTransaction("Remove band");
    processor.setValue(FeralProcessor::bandID(i, "enabled"), 0);
  };
  bankA.onClick = [this] {
    processor.selectBank(0);
    bindSelection();
  };
  bankB.onClick = [this] {
    processor.selectBank(1);
    bindSelection();
  };
  copy.onClick = [this] { processor.copyBank(); };
  undo.onClick = [this] { processor.undo.undo(); };
  redo.onClick = [this] { processor.undo.redo(); };
  power.setClickingTogglesState(true);
  for (int i = 0; i < processor.getNumPrograms(); ++i)
    presets.addItem(processor.getProgramName(i), i + 1);
  presets.onChange = [this] {
    processor.setCurrentProgram(presets.getSelectedId() - 1);
  };
  addAndMakeVisible(presets);
  shape.addItemList({"Bell", "Low shelf", "High shelf", "Low cut", "High cut"},
                    1);
  channel.addItemList({"Stereo", "Mid", "Side", "Left", "Right"}, 1);
  shape.onChange = [this] {
    if (boundSelection >= 0)
      processor.setValue(FeralProcessor::bandID(boundSelection, "shape"),
                         (float)(shape.getSelectedId() - 1));
  };
  channel.onChange = [this] {
    if (boundSelection >= 0)
      processor.setValue(FeralProcessor::bandID(boundSelection, "channel"),
                         (float)(channel.getSelectedId() - 1));
  };
  addAndMakeVisible(shape);
  addAndMakeVisible(channel);
  const char *names[]{"Threshold", "Ratio", "Attack",
                      "Release",   "Knee",  "Range"};
  for (int i = 0; i < 6; ++i) {
    dynamics[i] = std::make_unique<FeralControl>(processor, names[i], true);
    addAndMakeVisible(*dynamics[i]);
  }
  const char *filters[]{"Frequency", "Gain", "Q"};
  for (int i = 0; i < 3; ++i) {
    filter[i] = std::make_unique<FeralControl>(processor, filters[i], false);
    addAndMakeVisible(*filter[i]);
  }
  const char *globals[]{"Input", "Output", "Mix"};
  const char *ids[]{"input", "output", "mix"};
  for (int i = 0; i < 3; ++i) {
    global[i] = std::make_unique<FeralControl>(processor, globals[i], false);
    global[i]->bind(ids[i], i == 2 ? "%" : "dB");
    addAndMakeVisible(*global[i]);
  }
  detectorHP = std::make_unique<FeralControl>(processor, "Key low cut", false);
  detectorHP->bind("detectorHP", "Hz");
  addAndMakeVisible(*detectorHP);
  bindSelection();
  setResizable(true, true);
  getConstrainer()->setFixedAspectRatio(1120.0 / 800);
  setResizeLimits(896, 640, 1680, 1200);
  setSize(1120, 800);
  refresh();
  startTimerHz(30);
}
FeralEditor::~FeralEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}
void FeralEditor::select(int i) {
  processor.selected.store(juce::jlimit(-1, 7, i));
  bindSelection();
}
void FeralEditor::bindSelection() {
  boundSelection = processor.selected.load();
  const bool band = boundSelection >= 0;
  const auto id = [&](const char *key) {
    return band ? FeralProcessor::bandID(boundSelection, key)
                : juce::String(key);
  };
  const char *ids[]{"threshold", "ratio", "attack", "release", "knee", "range"};
  const char *units[]{"dB", ":1", "ms", "ms", "dB", "dB"};
  for (int i = 0; i < 6; ++i)
    dynamics[i]->bind(id(ids[i]), units[i]);
  for (auto &f : filter)
    f->setVisible(band);
  if (band) {
    filter[0]->bind(id("frequency"), "Hz");
    filter[1]->bind(id("gain"), "dB");
    filter[2]->bind(id("q"), "Q");
  }
  shape.setVisible(band);
  channel.setVisible(band);
  remove.setEnabled(band);
  dynamic.setVisible(band);
  detectorHP->setVisible(!band);
  enabled.setClickingTogglesState(true);
  dynamic.setClickingTogglesState(true);
  external.setClickingTogglesState(true);
  toggles.clear();
  const auto attach = [&](const juce::String &key, juce::TextButton &b) {
    toggles.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.state, key, b));
  };
  attach(id(band ? "enabled" : "busEnabled"), enabled);
  attach(id("external"), external);
  attach("bypass", power);
  if (band)
    attach(id("dynamic"), dynamic);
  resized();
  repaint();
}
void FeralEditor::timerCallback() { refresh(); }
void FeralEditor::refresh() {
  if (processor.selected.load() != boundSelection)
    bindSelection();
  for (int i = 0; i < 8; ++i) {
    bands[i].setToggleState(i == boundSelection, juce::dontSendNotification);
    bands[i].setAlpha(
        processor.value(FeralProcessor::bandID(i, "enabled")) > .5f ? 1.0f
                                                                    : .4f);
  }
  bus.setToggleState(boundSelection < 0, juce::dontSendNotification);
  bankA.setToggleState(processor.selectedBank() == 0,
                       juce::dontSendNotification);
  bankB.setToggleState(processor.selectedBank() == 1,
                       juce::dontSendNotification);
  presets.setSelectedId(processor.getCurrentProgram() + 1,
                        juce::dontSendNotification);
  if (boundSelection >= 0) {
    const int sh =
        (int)processor.value(FeralProcessor::bandID(boundSelection, "shape"));
    shape.setSelectedId(sh + 1, juce::dontSendNotification);
    channel.setSelectedId((int)processor.value(FeralProcessor::bandID(
                              boundSelection, "channel")) +
                              1,
                          juce::dontSendNotification);
    const bool cuts = sh >= feral::lowCut;
    filter[1]->setEnabled(!cuts);
    dynamic.setEnabled(!cuts);
    for (auto &d : dynamics)
      d->setEnabled(!cuts && processor.value(FeralProcessor::bandID(
                                 boundSelection, "dynamic")) > .5f);
  } else
    for (auto &d : dynamics)
      d->setEnabled(true);
  undo.setEnabled(processor.undo.canUndo());
  redo.setEnabled(processor.undo.canRedo());
  add.setEnabled(std::any_of(
      bands.begin(), bands.end(), [this, i = 0](const auto &) mutable {
        return processor.value(FeralProcessor::bandID(i++, "enabled")) < .5f;
      }));
  inputDB = std::max(inputDB - 1.3f, level(processor.inputPeak.exchange(0)));
  outputDB = std::max(outputDB - 1.3f, level(processor.outputPeak.exchange(0)));
  graph.updateSpectrum();
  graph.repaint();
  repaint();
}
void FeralEditor::paint(juce::Graphics &original) {
  theme.paintChassis(original, getLocalBounds().toFloat());
  juce::Graphics::ScopedSaveState save(original);
  original.addTransform(juce::AffineTransform::scale(getWidth() / 1120.0f));
  auto &g = original;
  g.setFont(font(24, true));
  g.setColour(juce::Colours::black.withAlpha(.75f));
  g.drawText("HUNGRY GHOST", 31, 24, 280, 31, juce::Justification::left);
  g.setColour(Theme::ink());
  g.drawText("HUNGRY GHOST", 31, 23, 280, 31, juce::Justification::left);
  g.setFont(font(10));
  g.setColour(Theme::accent().withAlpha(.85f));
  g.drawText("PRECISION DYNAMICS", 33, 58, 250, 17, juce::Justification::left);
  g.setFont(font(35, true));
  g.setColour(Theme::ink());
  g.drawText("FERAL", 321, 25, 165, 44, juce::Justification::left);
  g.setColour(Theme::line().withAlpha(.6f));
  g.drawLine(31, 82, 1088, 82);
  // Inspector lettering occupies its own machined strip, outside the graph
  // glass.
  g.setFont(font(12, true));
  g.setColour(boundSelection >= 0 ? colours[boundSelection] : Theme::accent());
  g.drawText(boundSelection >= 0 ? "BAND " + juce::String(boundSelection + 1)
                                 : "BUS",
             35, 485, 85, 23, juce::Justification::left);
  if (boundSelection < 0) {
    g.setFont(font(13));
    g.setColour(Theme::muted());
    g.drawText(
        juce::String::fromUTF8("Full-band compression  •  stereo linked"), 32,
        524, 495, 24, juce::Justification::left);
  }
  g.setColour(juce::Colours::black.withAlpha(.35f));
  g.fillRoundedRectangle(957, 569, 132, 156, 5);
  g.setColour(Theme::line());
  g.drawRoundedRectangle(957, 569, 132, 156, 5, .8f);
  const float gr = boundSelection >= 0
                       ? processor.reductions[boundSelection].load()
                       : processor.busReduction.load();
  g.setFont(font(11));
  g.setColour(Theme::muted());
  g.drawText("REDUCTION", 964, 581, 118, 18, juce::Justification::centred);
  g.setFont(font(32, true));
  g.setColour(Theme::accent());
  g.drawText(juce::String(gr, 1), 964, 608, 118, 42,
             juce::Justification::centred);
  g.setFont(font(12));
  g.setColour(Theme::muted());
  g.drawText("dB", 964, 652, 118, 18, juce::Justification::centred);
  g.setColour(Theme::line());
  g.fillRoundedRectangle(974, 686, 98, 4, 2);
  g.setColour(Theme::accent());
  g.fillRoundedRectangle(974, 686, 98 * juce::jlimit(0.0f, 1.0f, gr / 24), 4,
                         2);
  g.setFont(font(10));
  g.setColour(Theme::muted());
  g.drawText("LIVE", 964, 700, 118, 14, juce::Justification::centred);
  // Two live level meters share the right edge of the glass.
  for (int i = 0; i < 2; ++i) {
    const float xx = 1045.0f + i * 24;
    g.setColour(juce::Colours::black.withAlpha(.6f));
    g.fillRoundedRectangle(xx, 162, 12, 263, 3);
    const float db = i == 0 ? inputDB : outputDB;
    const float hh = 263 * juce::jlimit(0.0f, 1.0f, (db + 60) / 60);
    g.setGradientFill(juce::ColourGradient(Theme::accent().darker(.4f), xx, 425,
                                           Theme::accent(), xx, 162, false));
    g.fillRoundedRectangle(xx + 2, 425 - hh, 8, hh, 2);
    if (db > 0) {
      g.setColour(juce::Colour(0xffd69f97));
      g.fillRoundedRectangle(xx, 153, 12, 4, 2);
    }
    g.setColour(Theme::muted());
    g.setFont(font(9));
    g.drawText(i == 0 ? "IN" : "OUT", (int)xx - 6, 436, 24, 13,
               juce::Justification::centred);
  }
  g.setColour(Theme::line().withAlpha(.7f));
  g.drawLine(31, 730, 1088, 730);
  g.setFont(font(10));
  g.setColour(Theme::muted().withAlpha(.7f));
  g.drawText("ZERO LATENCY", 941, 749, 142, 17, juce::Justification::right);
}
void FeralEditor::resized() {
  if (!dynamics[0])
    return;
  const float sc = getWidth() / 1120.0f;
  theme.scale = sc;
  const auto bounds = [sc](juce::Component &c, int x, int y, int w, int h) {
    c.setBounds(juce::roundToInt(x * sc), juce::roundToInt(y * sc),
                juce::roundToInt(w * sc), juce::roundToInt(h * sc));
  };
  bounds(presets, 519, 32, 245, 34);
  bounds(bankA, 778, 32, 36, 34);
  bounds(bankB, 820, 32, 36, 34);
  bounds(copy, 862, 32, 71, 34);
  bounds(power, 959, 32, 129, 34);
  bounds(licenceButton, 568, 742, 285, 28);
  for (int i = 0; i < 8; ++i)
    bounds(bands[i], 31 + i * 49, 91, 43, 28);
  bounds(bus, 439, 91, 74, 28);
  bounds(add, 532, 91, 97, 28);
  bounds(remove, 635, 91, 99, 28);
  bounds(undo, 911, 91, 85, 28);
  bounds(redo, 1002, 91, 85, 28);
  bounds(graph, 31, 129, 996, 345);
  bounds(shape, 131, 483, 151, 31);
  bounds(channel, 292, 483, 130, 31);
  bounds(enabled, 678, 483, 107, 31);
  bounds(dynamic, 798, 483, 116, 31);
  bounds(external, 927, 483, 159, 31);
  for (int i = 0; i < 3; ++i)
    bounds(*filter[i], 32 + i * 183, 520, 172, 45);
  bounds(*detectorHP, 530, 516, 230, 48);
  for (int i = 0; i < 6; ++i)
    bounds(*dynamics[i], 30 + i * 153, 571, 145, 153);
  for (int i = 0; i < 3; ++i)
    bounds(*global[i], 32 + i * 177, 734, 163, 36);
}
