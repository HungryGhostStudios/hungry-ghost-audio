#include "GhostTheme.h"
#include <BinaryData.h>
#include <cmath>

namespace hungryghost {
namespace {
juce::Font type(float height, bool bold = false) {
  return juce::Font(juce::FontOptions(
      "Segoe UI", height, bold ? juce::Font::bold : juce::Font::plain));
}
} // namespace
void GhostTheme::paintReel(juce::Graphics& g, juce::Rectangle<float> bounds, int frame) const {
  if (!reelAtlas.isValid()) return;
  frame = ((frame % 24) + 24) % 24;
  const auto cell = reelAtlas.getWidth() / 6;
  g.drawImage(reelAtlas, juce::roundToInt(bounds.getX()), juce::roundToInt(bounds.getY()),
              juce::roundToInt(bounds.getWidth()), juce::roundToInt(bounds.getHeight()),
              (frame % 6) * cell, (frame / 6) * cell, cell, cell);
}
juce::Colour GhostTheme::background() { return juce::Colour(0xff0c100f); }
juce::Colour GhostTheme::panel() { return juce::Colour(0xff1c2421); }
juce::Colour GhostTheme::ink() { return juce::Colour(0xffebe7db); }
juce::Colour GhostTheme::muted() { return juce::Colour(0xffa5ada7); }
juce::Colour GhostTheme::accent() { return juce::Colour(0xffa2e4cf); }
juce::Colour GhostTheme::line() { return juce::Colour(0xff34403a); }
GhostTheme::GhostTheme()
    : dialAtlas(juce::ImageCache::getFromMemory(
          BinaryData::monolithdial_png, BinaryData::monolithdial_pngSize)),
      faceplate(juce::ImageCache::getFromMemory(
          BinaryData::monolithfaceplate_png,
          BinaryData::monolithfaceplate_pngSize)),
      display(
          juce::ImageCache::getFromMemory(BinaryData::monolithdisplay_png,
                                          BinaryData::monolithdisplay_pngSize)),
      keycap(juce::ImageCache::getFromMemory(
          BinaryData::monolithbutton_png, BinaryData::monolithbutton_pngSize)),
      selectedKeycap(juce::ImageCache::getFromMemory(
          BinaryData::monolithbuttonactive_png,
          BinaryData::monolithbuttonactive_pngSize)),
      reelAtlas(juce::ImageCache::getFromMemory(BinaryData::reelspool_png, BinaryData::reelspool_pngSize)) {
  setColour(juce::Slider::textBoxTextColourId, ink());
  setColour(juce::Slider::textBoxBackgroundColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxHighlightColourId, accent().withAlpha(.3f));
  setColour(juce::Slider::thumbColourId, accent());
  setColour(juce::Slider::trackColourId, accent());
  setColour(juce::Slider::backgroundColourId, line());
  setColour(juce::ComboBox::backgroundColourId, panel());
  setColour(juce::ComboBox::textColourId, ink());
  setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
  setColour(juce::ComboBox::arrowColourId, muted());
  setColour(juce::PopupMenu::backgroundColourId, panel());
  setColour(juce::PopupMenu::textColourId, ink());
  setColour(juce::PopupMenu::highlightedBackgroundColourId, line());
  setColour(juce::PopupMenu::highlightedTextColourId, accent());
  setColour(juce::Label::textColourId, muted());
  setColour(juce::TooltipWindow::backgroundColourId, panel());
  setColour(juce::TooltipWindow::textColourId, ink());
}
bool GhostTheme::hasAssets() const {
  return dialAtlas.getWidth() == 3072 && dialAtlas.getHeight() == 2048 &&
         faceplate.isValid() && display.isValid() && keycap.isValid() &&
         selectedKeycap.isValid();
}
void GhostTheme::paintChassis(juce::Graphics &g,
                              juce::Rectangle<float> bounds) const {
  g.fillAll(background());
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  g.drawImage(faceplate, bounds, juce::RectanglePlacement::stretchToFit);
  g.setColour(juce::Colours::black.withAlpha(.12f));
  g.fillRect(bounds);
  // A fine inner edge keeps the rendered chassis crisp at every host size.
  g.setColour(juce::Colours::white.withAlpha(.10f));
  g.drawRoundedRectangle(bounds.reduced(14 * scale), 5 * scale, .7f * scale);
}
void GhostTheme::paintDisplay(juce::Graphics &g,
                              juce::Rectangle<float> bounds) const {
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  g.drawImage(display, bounds, juce::RectanglePlacement::stretchToFit);
}
void GhostTheme::drawRotarySlider(juce::Graphics &g, int x, int y, int width,
                                  int height, float position, float start,
                                  float end, juce::Slider &slider) {
  const auto area =
      juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height)
          .reduced(2 * scale);
  const float size = std::min(area.getWidth(), area.getHeight());
  const float cx = area.getCentreX(), cy = area.getCentreY(),
              radius = size * .46f;
  const float angle = start + position * (end - start);
  const auto body =
      juce::Rectangle<float>(cx - size / 2, cy - size / 2, size, size);
  const bool enabled = slider.isEnabled();
  const auto dialAccent=slider.findColour(juce::Slider::thumbColourId);
  juce::Path shadow;
  shadow.addEllipse(body.reduced(size * .09f));
  juce::DropShadow(juce::Colours::black.withAlpha(.65f),
                   juce::roundToInt(7 * scale),
                   {0, juce::roundToInt(5 * scale)})
      .drawForPath(g, shadow);
  juce::Path track, active;
  track.addCentredArc(cx, cy, radius, radius, 0, start, end, true);
  active.addCentredArc(cx, cy, radius, radius, 0, start, angle, true);
  g.setColour(juce::Colours::black.withAlpha(.8f));
  g.strokePath(track, juce::PathStrokeType(7 * scale));
  g.setColour(line());
  g.strokePath(track, juce::PathStrokeType(3.4f * scale));
  g.setColour(dialAccent.withAlpha(enabled ? .09f : .03f));
  g.strokePath(active, juce::PathStrokeType(8 * scale));
  g.setColour(dialAccent.withAlpha(enabled ? .95f : .25f));
  g.strokePath(active, juce::PathStrokeType(3.2f * scale));
  // Select a camera render; never rotate a baked bitmap and its lighting.
  const int frame = juce::jlimit(0, 95, juce::roundToInt(position * 95));
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  g.setOpacity(enabled ? 1.0f : .4f);
  g.drawImage(dialAtlas, juce::roundToInt(body.getX()),
              juce::roundToInt(body.getY()), juce::roundToInt(size),
              juce::roundToInt(size), (frame % 12) * 256, (frame / 12) * 256,
              256, 256);
  g.setOpacity(1);
}
void GhostTheme::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                      const juce::Colour &, bool over,
                                      bool down) {
  if (static_cast<bool>(b.getProperties()["hardware"])) {
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    const auto r = b.getLocalBounds().toFloat();
    g.drawImage(b.getToggleState() ? selectedKeycap : keycap, r,
                juce::RectanglePlacement::stretchToFit);
    if (over || down) {
      g.setColour(
          (down ? juce::Colours::black : ink()).withAlpha(down ? .18f : .04f));
      g.fillRoundedRectangle(r.reduced(3 * scale), 3 * scale);
    }
    return;
  }
  if (!(b.getToggleState() || over || down))
    return;
  const auto r = b.getLocalBounds().toFloat().reduced(1.5f * scale);
  juce::Path outline;
  outline.addRoundedRectangle(r, 4 * scale);
  juce::DropShadow(juce::Colours::black.withAlpha(.75f),
                   juce::roundToInt(3 * scale),
                   {0, juce::roundToInt(2 * scale)})
      .drawForPath(g, outline);
  g.setGradientFill(juce::ColourGradient(
      panel().brighter(down ? .03f : .12f), r.getX(), r.getY(),
      panel().darker(.15f), r.getX(), r.getBottom(), false));
  g.fillPath(outline);
  g.setColour(
      (b.getToggleState() ? accent() : ink()).withAlpha(over ? .35f : .18f));
  g.strokePath(outline, juce::PathStrokeType(scale));
}
void GhostTheme::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool,
                                bool) {
  g.setFont(type(17 * scale, b.getToggleState()));
  g.setColour(juce::Colours::black.withAlpha(.7f));
  g.drawFittedText(
      b.getButtonText(),
      b.getLocalBounds().reduced(4).translated(0, juce::roundToInt(scale)),
      juce::Justification::centred, 1);
  g.setColour(b.getToggleState() ? accent().withAlpha(.90f)
                                 : ink().withAlpha(.84f));
  g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4),
                   juce::Justification::centred, 1);
}
void GhostTheme::drawComboBox(juce::Graphics &g, int width, int height, bool,
                              int, int, int, int, juce::ComboBox &) {
  const auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  g.drawImage(keycap, bounds, juce::RectanglePlacement::stretchToFit);
  g.setColour(juce::Colours::black.withAlpha(.22f));
  g.fillRoundedRectangle(bounds.reduced(3 * scale), 2 * scale);
  const float x = width - 19 * scale, y = height * .48f;
  juce::Path arrow;
  arrow.startNewSubPath(x - 5 * scale, y - 2 * scale);
  arrow.lineTo(x, y + 3 * scale);
  arrow.lineTo(x + 5 * scale, y - 2 * scale);
  g.setColour(ink());
  g.strokePath(arrow, juce::PathStrokeType(1.8f * scale));
}
void GhostTheme::positionComboBoxText(juce::ComboBox &box, juce::Label &label) {
  label.setBounds(juce::roundToInt(10 * scale), 0,
                  box.getWidth() - juce::roundToInt(38 * scale),
                  box.getHeight());
  label.setFont(getComboBoxFont(box));
}
void GhostTheme::drawLinearSlider(juce::Graphics &g, int x, int y, int width,
                                  int height, float position, float, float,
                                  juce::Slider::SliderStyle style, juce::Slider &slider) {
  const auto tint=slider.findColour(juce::Slider::trackColourId);
  if(style==juce::Slider::LinearVertical||style==juce::Slider::LinearBarVertical){
    const float cx=x+width*.5f;
    auto track=juce::Rectangle<float>(cx-5*scale,(float)y,10*scale,(float)height);
    g.setColour(juce::Colours::black.withAlpha(.8f));g.fillRoundedRectangle(track,3*scale);
    g.setColour(line());g.drawRoundedRectangle(track,3*scale,scale);
    g.setColour(tint.withAlpha(.5f));g.fillRect(cx-2*scale,position,4*scale,y+height-position);
    for(int i=0;i<=10;++i){float py=y+height*i/10.f;g.setColour(muted().withAlpha(.4f));g.drawLine(cx-22*scale,py,cx-12*scale,py,scale);g.drawLine(cx+12*scale,py,cx+22*scale,py,scale);}
    auto cap=juce::Rectangle<float>(cx-20*scale,position-11*scale,40*scale,22*scale);
    g.setColour(juce::Colours::black.withAlpha(.7f));g.fillRoundedRectangle(cap.translated(0,3*scale),3*scale);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffaab1aa),cx,cap.getY(),juce::Colour(0xff303933),cx,cap.getBottom(),false));g.fillRoundedRectangle(cap,3*scale);
    g.setColour(juce::Colours::black.withAlpha(.5f));g.drawRoundedRectangle(cap,3*scale,scale);g.drawLine(cap.getX()+5*scale,position,cap.getRight()-5*scale,position,2*scale);
    g.setColour(tint);g.drawLine(cap.getX()+5*scale,position-scale,cap.getRight()-5*scale,position-scale,scale);
    return;
  }
  const float cy = y + height * .5f;
  g.setColour(juce::Colours::black.withAlpha(.65f));
  g.drawLine((float)x, cy, (float)(x + width), cy, 4 * scale);
  g.setColour(line());
  g.drawLine((float)x, cy + scale, (float)(x + width), cy + scale, scale);
  g.setColour(tint.withAlpha(.65f));
  g.drawLine((float)x, cy, position, cy, 2 * scale);
  const float r = 4 * scale;
  g.setColour(juce::Colours::black.withAlpha(.7f));
  g.fillEllipse(position - r - scale, cy - r + scale, 2 * r + 2 * scale,
                2 * r + 2 * scale);
  g.setGradientFill(juce::ColourGradient(tint.brighter(.15f), position,
                                         cy - r, tint.darker(.25f),
                                         position, cy + r, false));
  g.fillEllipse(position - r, cy - r, 2 * r, 2 * r);
}
juce::Font GhostTheme::getComboBoxFont(juce::ComboBox &) {
  return type(17 * scale);
}
juce::Font GhostTheme::getLabelFont(juce::Label &label) {
  if (auto *slider = dynamic_cast<juce::Slider *>(label.getParentComponent())) {
    if (static_cast<bool>(slider->getProperties()["suite"]))
      return type((static_cast<bool>(slider->getProperties()["compact"])?14.f:
                   static_cast<bool>(slider->getProperties()["primary"])?28.f:18.f)*scale,true);
    const bool detail = static_cast<bool>(slider->getProperties()["detail"]);
    return type(static_cast<bool>(slider->getProperties()["large"])
                    ? 42 * scale
                    : (detail ? 14 * scale : 28 * scale),
                !detail);
  }
  return type(15 * scale);
}
void GhostTheme::drawLabel(juce::Graphics &g, juce::Label &label) {
  const auto *slider = dynamic_cast<juce::Slider *>(label.getParentComponent());
  const bool trim = slider != nullptr && (slider->getComponentID() == "input" ||
                                          slider->getComponentID() == "output");
  if (trim)
    g.drawImage(keycap, label.getLocalBounds().toFloat(),
                juce::RectanglePlacement::stretchToFit);
  else
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
  const float alpha = label.isEnabled() ? 1.0f : .5f;
  if (!label.isBeingEdited()) {
    g.setFont(getLabelFont(label));
    const auto area =
        getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
    // A restrained print shadow integrates native lettering with the lit
    // chassis.
    g.setColour(juce::Colours::black.withAlpha(.7f * alpha));
    g.drawFittedText(
        label.getText(), area.translated(0, juce::roundToInt(scale)),
        label.getJustificationType(), 1, label.getMinimumHorizontalScale());
    g.setColour(label.findColour(juce::Label::textColourId)
                    .withMultipliedAlpha(alpha * .92f));
    g.drawFittedText(label.getText(), area, label.getJustificationType(), 1,
                     label.getMinimumHorizontalScale());
  }
  if (!trim) {
    g.setColour(label.findColour(juce::Label::outlineColourId)
                    .withMultipliedAlpha(alpha));
    g.drawRect(label.getLocalBounds());
  }
}
} // namespace hungryghost
