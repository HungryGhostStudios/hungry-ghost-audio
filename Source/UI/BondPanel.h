#pragma once
#include "../SuiteProcessor.h"
#include <cmath>

namespace hungryghost {

// BOND has its own control surface; the other instruments retain their themes.
class BondLookAndFeel final : public juce::LookAndFeel_V4 {
public:
  float scale = 1.f;
  static juce::Colour ink() { return juce::Colour(0xffe9dfcd); }
  static juce::Colour muted() { return juce::Colour(0xffaeb6ab); }
  static juce::Colour amber() { return juce::Colour(0xffdca16c); }
  static juce::Colour sage() { return juce::Colour(0xffa1c8b6); }
  static juce::Colour copper() { return juce::Colour(0xff986a4e); }
  static juce::Font font(float size, bool bold = false) {
    return juce::Font(juce::FontOptions("Segoe UI", size * 1.2f,
        bold ? juce::Font::bold : juce::Font::plain));
  }
  BondLookAndFeel() {
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff202927));
    setColour(juce::PopupMenu::textColourId, ink());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff46554b));
    setColour(juce::PopupMenu::highlightedTextColourId, ink());
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0d1412));
    setColour(juce::TextEditor::textColourId, ink());
    setColour(juce::TextEditor::highlightColourId, juce::Colour(0xff435d4d));
    setColour(juce::TextEditor::outlineColourId, sage());
  }
  void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
      float position, float start, float end, juce::Slider& slider) override {
    const float s = scale;
    const float cx = x + w * .5f, cy = y + h * .49f;
    const float radius = juce::jmin(w * .29f, h * .31f);
    const bool compact = w < 100*s || h < 85*s;
    const float alpha = slider.isEnabled() ? 1.f : .35f;
    const bool main = bool(slider.getProperties()["bondPrimary"]);
    const auto colour = main ? amber() : sage();
    auto point = [&](float a, float r) {
      return juce::Point<float>(cx + std::sin(a) * r, cy - std::cos(a) * r);
    };
    // Calibrations are positioned from the parameter's actual normalisable range.
    if (auto* ticks = slider.getProperties()["bondTicks"].getArray()) {
      for (const auto& t : *ticks) {
        auto* values = t.getArray();
        if (values == nullptr || values->size() < 2) continue;
        const float a = start + (end - start) *
            float(slider.valueToProportionOfLength(double((*values)[0])));
        g.setColour(muted().withAlpha(.8f * alpha));
        g.drawLine({point(a, radius + 6*s), point(a, radius + 11*s)}, .85f*s);
        const auto p = point(a, radius + (compact?16.f:22.f)*s);
        const float labelWidth=(compact?28.f:38.f)*s;
        g.setFont(font((compact?8.4f:9.4f)*s));
        auto caption=juce::Rectangle<float>(p.x-labelWidth*.5f,p.y-7*s,labelWidth,14*s);
        if(caption.getY()<y+s){
          // Short secondary dials put the upper caption beside its tick so it
          // remains inside the slider without covering the machined collar.
          caption.setY(y+s);caption.setX(p.x+5*s);
        }
        g.drawText((*values)[1].toString(),caption,juce::Justification::centred);
      }
    }
    juce::Path arc, progress;
    arc.addCentredArc(cx, cy, radius+5*s, radius+5*s, 0, start, end, true);
    progress.addCentredArc(cx, cy, radius+5*s, radius+5*s, 0,
        start, start+(end-start)*position, true);
    g.setColour(juce::Colours::black.withAlpha(.8f));
    g.strokePath(arc, juce::PathStrokeType(2.4f*s));
    g.setColour(colour.withAlpha(.62f*alpha));
    g.strokePath(progress, juce::PathStrokeType(1.2f*s));
    const juce::Rectangle<float> rim(cx-radius,cy-radius,radius*2,radius*2);
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.fillEllipse(rim.expanded(2*s).translated(0,3*s));
    juce::ColourGradient edge(juce::Colour(0xffb9b9aa),cx-radius,cy-radius,
        juce::Colour(0xff0b1012),cx+radius,cy+radius,false);
    edge.addColour(.20,juce::Colour(0xff606b63));
    edge.addColour(.48,juce::Colour(0xff171e1f));
    edge.addColour(.76,juce::Colour(0xff7a857a));
    g.setGradientFill(edge); g.fillEllipse(rim);
    // Fine radial fluting describes a machined collar, without raster scaling.
    for(int i=0;i<64;++i) {
      const float a = juce::MathConstants<float>::twoPi*i/64.f;
      const float light = (.5f+.5f*std::cos(a+.65f))*.20f;
      g.setColour(juce::Colours::white.withAlpha(light));
      g.drawLine({point(a,radius-1.5f*s),point(a,radius-4*s)},.7f*s);
    }
    auto face = rim.reduced(5*s);
    g.setGradientFill(juce::ColourGradient(main?juce::Colour(0xff424b44):juce::Colour(0xff34413c),
        face.getX(),face.getY(),juce::Colour(0xff0d1718),face.getRight(),face.getBottom(),false));
    g.fillEllipse(face);
    g.setColour(juce::Colour(0xffd2e0da).withAlpha(.38f));
    g.drawEllipse(face,.7f*s);
    g.setColour(juce::Colours::black.withAlpha(.25f));
    g.drawEllipse(face.reduced(2*s),.8f*s);
    const float a = start + (end-start)*position;
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.drawLine({point(a,radius*.45f).translated(s,s),
        point(a,radius-9*s).translated(s,s)},3*s);
    g.setColour(colour.withAlpha(alpha));
    g.drawLine({point(a,radius*.45f),point(a,radius-9*s)},2.6f*s);
    if(slider.isMouseOverOrDragging()) {
      g.setColour(colour.withAlpha(.34f));
      g.drawEllipse(rim.expanded(2*s),s);
    }
  }
  void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,
      float,float,juce::Slider::SliderStyle,juce::Slider& slider) override {
    const float s = scale, cy = y+h*.5f;
    const bool enabled=slider.isEnabled(),over=enabled&&slider.isMouseOverOrDragging();
    // The slot carries no coloured progress fill: position is read from its
    // engraved scale and the cap's central index, like a console fader.
    auto slot=juce::Rectangle<float>(float(x)-3*s,cy-4*s,float(w)+6*s,8*s);
    mount(g,slot.expanded(1.5f*s,1.5f*s),s);
    g.setColour(juce::Colour(0xff04090c));g.fillRoundedRectangle(slot,2*s);
    g.setColour(juce::Colour(0xff798d8b).withAlpha(.3f));
    g.drawLine(slot.getX()+2*s,slot.getBottom(),slot.getRight()-2*s,slot.getBottom(),.7f*s);
    for(int i=0;i<5;++i){
      const float at=x+w*i*.25f;
      g.setColour(muted().withAlpha(enabled?.72f:.28f));
      g.drawLine(at,cy-13*s,at,cy-(i==0||i==4?7.f:9.f)*s,.8f*s);
    }
    auto cap=juce::Rectangle<float>(pos-11*s,cy-14*s,22*s,28*s);
    g.setColour(juce::Colours::black.withAlpha(.56f));
    g.fillRoundedRectangle(cap.expanded(2*s,1*s).translated(0,3*s),3*s);
    g.beginTransparencyLayer(enabled?1.f:.45f);
    juce::ColourGradient edge(juce::Colour(0xffcad6cf),cap.getX(),cap.getY(),
        juce::Colour(0xff101b23),cap.getRight(),cap.getBottom(),false);
    edge.addColour(.30,juce::Colour(0xff6c807e));edge.addColour(.64,juce::Colour(0xff26373c));
    g.setGradientFill(edge);g.fillRoundedRectangle(cap,2.4f*s);
    const auto face=cap.reduced(2*s).translated(0,slider.isMouseButtonDown()?.7f*s:0.f);
    juce::ColourGradient metal(over?juce::Colour(0xff7d8b7d):juce::Colour(0xff617168),
        face.getX(),face.getY(),juce::Colour(0xff1b2a2c),face.getX(),face.getBottom(),false);
    metal.addColour(.42,juce::Colour(0xff465c53));metal.addColour(.52,juce::Colour(0xff2b403d));
    g.setGradientFill(metal);g.fillRoundedRectangle(face,1.4f*s);
    for(float offset:{-7.f,7.f}){
      g.setColour(juce::Colour(0xff11232b));g.drawLine(face.getX()+3*s,cy+offset*s,face.getRight()-3*s,cy+offset*s,1.4f*s);
      g.setColour(juce::Colour(0xffb3c3b7).withAlpha(.35f));g.drawLine(face.getX()+3*s,cy+(offset+.8f)*s,face.getRight()-3*s,cy+(offset+.8f)*s,.6f*s);
    }
    g.setColour(juce::Colour(0xff111b21));g.fillRoundedRectangle(pos-2*s,cy-4.7f*s,4*s,9.4f*s,.7f*s);
    g.setColour(ink());g.fillRect(pos-.7f*s,cy-3.5f*s,1.4f*s,7*s);
    if(enabled&&slider.hasKeyboardFocus(true)){
      g.setColour(amber().withAlpha(.9f));g.drawRoundedRectangle(cap.expanded(2*s),3*s,s);
    }
    g.endTransparencyLayer();
  }
  int getSliderThumbRadius(juce::Slider& slider) override {
    // Extra inset keeps the complete cap and its shadow visible at 0 and 100%.
    return slider.isHorizontal()?juce::roundToInt(16*scale):juce::LookAndFeel_V4::getSliderThumbRadius(slider);
  }
  juce::Label* createSliderTextBox(juce::Slider& slider) override {
    auto* label=juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->getProperties().set("bondReadout",true);
    label->setTitle(slider.getName()+" value");
    label->setColour(juce::Label::textColourId,ink());
    label->setColour(juce::Label::backgroundColourId,juce::Colour(0xff091419));
    label->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label->setColour(juce::Label::textWhenEditingColourId,ink());
    label->setColour(juce::Label::backgroundWhenEditingColourId,juce::Colour(0xff091419));
    label->setBorderSize(juce::BorderSize<int>(0));
    return label;
  }
  juce::Font getLabelFont(juce::Label& label) override {
    if(bool(label.getProperties()["bondReadout"]))
      return juce::Font(juce::FontOptions("Consolas",15.f*scale,juce::Font::plain));
    return font(12.5f*scale);
  }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return font(12.f*scale); }
  void drawLabel(juce::Graphics& g,juce::Label& label) override {
    auto r=label.getLocalBounds().toFloat();
    if(bool(label.getProperties()["bondReadout"])){
      const bool parentFocus=label.getParentComponent()!=nullptr&&label.getParentComponent()->hasKeyboardFocus(true);
      readout(g,r,scale,label.isEnabled(),label.isMouseOver(true)||label.hasKeyboardFocus(true)||parentFocus);
    }
    if(label.isBeingEdited())return;
    g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(label.isEnabled()?1.f:.4f));
    g.setFont(getLabelFont(label));
    g.drawText(label.getText(),r.reduced(4*scale,0),label.getJustificationType());
  }
  void fillTextEditorBackground(juce::Graphics& g,int w,int h,juce::TextEditor& editor) override {
    readout(g,{0,0,float(w),float(h)},scale,editor.isEnabled(),true);
  }
  void drawTextEditorOutline(juce::Graphics& g,int w,int h,juce::TextEditor& editor) override {
    if(editor.hasKeyboardFocus(true)){
      g.setColour(amber().withAlpha(.9f));
      g.drawRoundedRectangle(juce::Rectangle<float>(0,0,float(w),float(h)).reduced(1.5f*scale),1.5f*scale,.8f*scale);
    }
  }
  void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool over,bool down) override {
    const float s=scale;const bool enabled=button.isEnabled();
    auto r=button.getLocalBounds().toFloat().reduced(.6f*s);
    const bool on=button.getToggleState(),latching=isLatchingControl(button);
    mount(g,r,s);
    const float travel=enabled&&(down||on)?1.1f*s:0.f;
    auto face=r.reduced(2.4f*s,2.5f*s).translated(0,travel);
    actuator(g,face,s,enabled&&over,enabled&&down,enabled);
    if(latching){
      const float diameter=(r.getWidth()<50*s?3.8f:4.8f)*s;
      const auto point=juce::Point<float>(face.getX()+(r.getWidth()<50*s?5.2f:10.f)*s,face.getCentreY());
      const bool caution=button.getComponentID()=="key_listen"||button.getButtonText().equalsIgnoreCase("Bypass");
      lamp(g,point,diameter,on&&enabled,caution?amber():sage());
    }
    if(enabled&&button.hasKeyboardFocus(true)){
      g.setColour(amber().withAlpha(.8f));g.drawRoundedRectangle(r.reduced(1.2f*s),2*s,.9f*s);
    }
  }
  void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool over,bool down) override {
    const float s=scale;auto r=b.getLocalBounds().toFloat().reduced(6*s,0);
    if(isLatchingControl(b))r.removeFromLeft((b.getWidth()<50*s?5.f:13.f)*s);
    if(b.isEnabled()&&(down||b.getToggleState()))r=r.translated(0,1.1f*s);
    g.setFont(font(10.9f*s,true));
    const auto text=b.getButtonText().toUpperCase();
    g.setColour(juce::Colours::black.withAlpha(.7f));g.drawText(text,r.translated(0,-.6f*s),juce::Justification::centred);
    g.setColour((over||b.getToggleState()?ink():muted()).withMultipliedAlpha(b.isEnabled()?1.f:.35f));
    g.drawText(text,r,juce::Justification::centred);
  }
  void drawComboBox(juce::Graphics& g,int w,int h,bool down,int,int,int,int,juce::ComboBox& box) override {
    const float s=scale;const bool enabled=box.isEnabled(),open=down||box.isPopupActive();
    auto r=juce::Rectangle<float>(.6f*s,.6f*s,w-1.2f*s,h-1.2f*s);
    mount(g,r,s);
    const auto window=r.reduced(2.5f*s);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff030709),window.getX(),window.getY(),juce::Colour(0xff0d1718),window.getX(),window.getBottom(),false));
    g.fillRoundedRectangle(window,1.8f*s);
    g.setColour(juce::Colour(0xffd6e6db).withAlpha(.06f));
    g.fillRect(window.withHeight(window.getHeight()*.46f));
    auto key=window.withWidth(25*s).withX(window.getRight()-25*s);
    g.setColour(juce::Colours::black.withAlpha(.9f));g.fillRect(key.expanded(1*s,0));
    actuator(g,key.reduced(.6f*s).translated(0,open?.65f*s:0.f),s,
        enabled&&box.isMouseOver(true),enabled&&open,enabled);
    const float cx=key.getCentreX(),cy=key.getCentreY()+(open?.8f:0.f)*s;
    juce::Path arrow;const float direction=open?-1.f:1.f;
    arrow.startNewSubPath(cx-4*s,cy-direction*1.5f*s);
    arrow.lineTo(cx,cy+direction*2.5f*s);arrow.lineTo(cx+4*s,cy-direction*1.5f*s);
    g.setColour(juce::Colours::black.withAlpha(.8f));g.strokePath(arrow,juce::PathStrokeType(2.8f*s));
    g.setColour(ink().withAlpha(enabled?.88f:.25f));g.strokePath(arrow,juce::PathStrokeType(1.2f*s));
    if(enabled&&(box.hasKeyboardFocus(true)||open)){
      g.setColour(amber().withAlpha(open?.82f:.62f));g.drawRoundedRectangle(r.reduced(1.3f*s),2*s,.8f*s);
    }
  }
  void positionComboBoxText(juce::ComboBox& box,juce::Label& label) override {
    label.setBounds(juce::roundToInt(10*scale),0,box.getWidth()-juce::roundToInt(46*scale),box.getHeight());
    label.getProperties().set("bondSelectorLabel",true);
    label.setColour(juce::Label::textColourId,ink());
    label.setColour(juce::Label::backgroundColourId,juce::Colours::transparentBlack);
    label.setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label.setFont(getComboBoxFont(box));
  }
  void drawComboBoxTextWhenNothingSelected(juce::Graphics& g,juce::ComboBox& box,juce::Label& label) override {
    // JUCE calls this in the selector's coordinate space, not the label's.
    g.setFont(getLabelFont(label));
    g.setColour(muted().withAlpha(box.isEnabled()?.72f:.3f));
    g.drawText(box.getTextWhenNothingSelected(),label.getBounds().toFloat().reduced(4*scale,0),label.getJustificationType());
  }
  juce::Font getPopupMenuFont() override {return font(12.f*scale);}
  void getIdealPopupMenuItemSize(const juce::String& text,bool separator,int standardHeight,int& w,int& h) override {
    h=separator?juce::roundToInt(9*scale):juce::jmax(standardHeight,juce::roundToInt(31*scale));
    w=separator?juce::roundToInt(80*scale):juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(),text)+juce::roundToInt(62*scale);
  }
  void drawPopupMenuBackground(juce::Graphics& g,int w,int h) override {
    const float s=scale;auto r=juce::Rectangle<float>(0,0,float(w),float(h));
    g.fillAll(juce::Colour(0xff080e0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222b29),0,0,juce::Colour(0xff10191a),0,float(h),false));g.fillRect(r.reduced(s));
    g.setColour(juce::Colour(0xff9baea6).withAlpha(.7f));g.drawRect(r.reduced(.5f*s),s);
    g.setColour(juce::Colour(0xff060f15));g.drawRect(r.reduced(2*s),s);
  }
  void drawPopupMenuItem(juce::Graphics& g,const juce::Rectangle<int>& area,bool separator,
      bool active,bool highlighted,bool ticked,bool submenu,const juce::String& text,
      const juce::String& shortcut,const juce::Drawable* icon,const juce::Colour*) override {
    const float s=scale;auto r=area.toFloat().reduced(3*s,0);
    if(separator){
      g.setColour(juce::Colour(0xff07121a));g.drawLine(r.getX()+11*s,r.getCentreY(),r.getRight()-11*s,r.getCentreY(),s);
      g.setColour(juce::Colour(0xff92a79c).withAlpha(.17f));g.drawLine(r.getX()+11*s,r.getCentreY()+s,r.getRight()-11*s,r.getCentreY()+s,.6f*s);return;
    }
    if(active&&highlighted){
      auto face=r.reduced(2*s,2*s);
      g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3c4b42),face.getX(),face.getY(),juce::Colour(0xff21322d),face.getX(),face.getBottom(),false));g.fillRoundedRectangle(face,1.5f*s);
      g.setColour(juce::Colour(0xffa6bab0).withAlpha(.45f));g.drawLine(face.getX(),face.getY(),face.getRight(),face.getY(),.7f*s);
      g.setColour(amber().withAlpha(.82f));g.fillRect(face.getX(),face.getY()+4*s,2*s,face.getHeight()-8*s);
    }
    const auto marker=juce::Point<float>(r.getX()+15*s,r.getCentreY());
    if(icon!=nullptr)icon->drawWithin(g,{r.getX()+7*s,r.getCentreY()-8*s,16*s,16*s},juce::RectanglePlacement::centred,active?1.f:.35f);
    else if(ticked)lamp(g,marker,4.6f*s,active,sage());
    auto bounds=r.withTrimmedLeft(29*s).withTrimmedRight((submenu?25.f:12.f)*s);
    if(shortcut.isNotEmpty()){
      const float sw=juce::GlyphArrangement::getStringWidth(font(9.5f*s),shortcut)+12*s;
      auto shortcutBounds=bounds.removeFromRight(sw);
      g.setFont(font(9.5f*s));g.setColour(muted().withAlpha(active?.65f:.28f));g.drawText(shortcut,shortcutBounds,juce::Justification::centredRight);
    }
    g.setFont(ticked?getPopupMenuFont().boldened():getPopupMenuFont());
    g.setColour((highlighted?ink():muted()).withAlpha(active?1.f:.35f));g.drawText(text,bounds,juce::Justification::centredLeft);
    if(submenu){
      const float x=r.getRight()-13*s,y=r.getCentreY();juce::Path arrow;
      arrow.startNewSubPath(x-2*s,y-3*s);arrow.lineTo(x+2*s,y);arrow.lineTo(x-2*s,y+3*s);
      g.setColour(ink().withAlpha(active?.8f:.3f));g.strokePath(arrow,juce::PathStrokeType(s));
    }
  }
  void drawPopupMenuSectionHeader(juce::Graphics& g,const juce::Rectangle<int>& area,const juce::String& title) override {
    g.setFont(font(10.f*scale,true));g.setColour(sage());
    g.drawText(title.toUpperCase(),area.toFloat().reduced(14*scale,0),juce::Justification::centredLeft);
  }
  void drawPopupMenuUpDownArrow(juce::Graphics& g,int w,int h,bool up) override {
    g.setColour(juce::Colour(0xff162421));g.fillRect(0,0,w,h);
    const float x=w*.5f,y=h*.5f,d=up?-1.f:1.f;juce::Path p;
    p.startNewSubPath(x-4*scale,y-d*2*scale);p.lineTo(x,y+d*2*scale);p.lineTo(x+4*scale,y-d*2*scale);
    g.setColour(ink());g.strokePath(p,juce::PathStrokeType(scale));
  }
private:
  static bool isLatchingControl(const juce::Button& b){
    return b.getClickingTogglesState()||b.getButtonText()=="A"||b.getButtonText()=="B";
  }
  static void mount(juce::Graphics& g,juce::Rectangle<float> r,float s){
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff060c11),r.getX(),r.getY(),juce::Colour(0xff748783),r.getX(),r.getBottom(),false));
    g.fillRoundedRectangle(r,3*s);
    g.setColour(juce::Colour(0xff071116));g.fillRoundedRectangle(r.reduced(1.2f*s),2*s);
    g.setColour(juce::Colours::black.withAlpha(.8f));g.drawLine(r.getX()+3*s,r.getY()+s,r.getRight()-3*s,r.getY()+s,s);
  }
  static void actuator(juce::Graphics& g,juce::Rectangle<float> r,float s,bool over,bool down,bool enabled){
    const float alpha=enabled?1.f:.4f;
    g.beginTransparencyLayer(alpha);
    if(!down){g.setColour(juce::Colours::black.withAlpha(.78f));g.fillRoundedRectangle(r.translated(0,1.2f*s),2*s);}
    auto top=over?juce::Colour(0xff646c61):juce::Colour(0xff434d46);
    auto bottom=juce::Colour(0xff0c191a);
    if(down){top=juce::Colour(0xff2c3b34);bottom=juce::Colour(0xff1b2b27);}
    juce::ColourGradient metal(top,r.getX(),r.getY(),bottom,r.getX(),r.getBottom(),false);
    metal.addColour(.14,down?juce::Colour(0xff1c2e2a):juce::Colour(0xff2b3b34));
    metal.addColour(.82,juce::Colour(0xff172924));
    g.setGradientFill(metal);g.fillRoundedRectangle(r,2*s);
    g.setColour(juce::Colour(0xffc9d8cf).withAlpha(down?.16f:.53f));g.drawLine(r.getX()+2*s,r.getY()+.7f*s,r.getRight()-2*s,r.getY()+.7f*s,.65f*s);
    g.setColour(juce::Colour(0xff9eafaa).withAlpha(.17f));g.drawLine(r.getX()+s,r.getY()+2*s,r.getX()+s,r.getBottom()-2*s,.65f*s);
    g.setColour(juce::Colour(0xff09141c).withAlpha(.9f));g.drawLine(r.getX()+2*s,r.getBottom()-.7f*s,r.getRight()-2*s,r.getBottom()-.7f*s,s);
    g.endTransparencyLayer();
  }
  static void lamp(juce::Graphics& g,juce::Point<float> centre,float diameter,bool lit,juce::Colour colour){
    const auto r=juce::Rectangle<float>(diameter,diameter).withCentre(centre);
    g.setColour(juce::Colour(0xff050b0e));g.fillEllipse(r.expanded(diameter*.3f));
    g.setColour(juce::Colour(0xff93a39b).withAlpha(.24f));g.drawEllipse(r.expanded(diameter*.23f),diameter*.13f);
    if(lit){g.setColour(colour.withAlpha(.12f));g.fillEllipse(r.expanded(diameter*.65f));}
    g.setGradientFill(juce::ColourGradient(lit?colour.brighter(.22f):juce::Colour(0xff4b5c58),r.getX(),r.getY(),lit?colour.darker(.28f):juce::Colour(0xff172b2c),r.getRight(),r.getBottom(),false));g.fillEllipse(r);
    g.setColour(juce::Colours::white.withAlpha(lit?.55f:.16f));g.fillEllipse(r.withSizeKeepingCentre(diameter*.28f,diameter*.22f).translated(-diameter*.13f,-diameter*.17f));
  }
  static void readout(juce::Graphics& g,juce::Rectangle<float> r,float s,bool enabled,bool focus){
    r=r.reduced(.4f*s);mount(g,r,s);
    auto glass=r.reduced(2*s);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff020608),glass.getX(),glass.getY(),juce::Colour(0xff0b1715),glass.getX(),glass.getBottom(),false));g.fillRoundedRectangle(glass,1.4f*s);
    g.setColour(juce::Colour(0xffacbeb7).withAlpha(.10f));g.drawLine(glass.getX()+2*s,glass.getBottom(),glass.getRight()-2*s,glass.getBottom(),.65f*s);
    if(enabled&&focus){g.setColour(amber().withAlpha(.65f));g.drawRoundedRectangle(r.reduced(1.2f*s),2*s,.7f*s);}
  }
};

class BondDial final : public juce::Component {
public:
  using Attachment=juce::AudioProcessorValueTreeState::SliderAttachment;
  struct Tick { double value; const char* label; };
  BondDial(juce::AudioProcessorValueTreeState& state,const juce::String& id,
      const juce::String& title,const juce::String& tooltip,
      std::initializer_list<Tick> ticks,bool primary=false)
      : name(title) {
    slider.setComponentID(id);slider.setName(title);slider.setTitle(title);slider.setTooltip(tooltip);
    slider.setWantsKeyboardFocus(true);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi*1.2f,
        juce::MathConstants<float>::pi*2.8f,true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,24);
    slider.setMouseDragSensitivity(180);
    juce::Array<juce::var> points;
    for(auto t:ticks){juce::Array<juce::var> item;item.add(t.value);item.add(juce::String(t.label));points.add(juce::var(item));}
    slider.getProperties().set("bondTicks",juce::var(points));
    slider.getProperties().set("bondPrimary",primary);
    if(auto* p=state.getParameter(id)) {
      attachment=std::make_unique<Attachment>(state,id,slider);
      slider.setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));
    }
    addAndMakeVisible(slider);
  }
  void format(std::function<juce::String(double)> toText,std::function<double(const juce::String&)> fromText={}) {
    slider.textFromValueFunction=std::move(toText);
    slider.valueFromTextFunction=fromText?std::move(fromText):[](const juce::String& t){return t.getDoubleValue();};
    slider.updateText();
  }
  void paint(juce::Graphics& g) override {
    g.setFont(BondLookAndFeel::font(11.f*scale,true));
    g.setColour(BondLookAndFeel::ink().withAlpha(isEnabled()?1.f:.4f));
    g.drawText(name,getLocalBounds().toFloat().withHeight(19*scale),juce::Justification::centred);
  }
  void resized() override {
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,juce::roundToInt(juce::jmin(90.f,getWidth()/scale-8)*scale),juce::roundToInt(24*scale));
    slider.setBounds(getLocalBounds().withTrimmedTop(juce::roundToInt(20*scale)));
  }
  float scale=1.f;
  juce::Slider slider;
private:
  juce::String name;
  std::unique_ptr<Attachment> attachment;
};

class BondPanel final : public juce::Component, private juce::Timer {
public:
  static constexpr int width=1160,height=720;
  explicit BondPanel(SuiteProcessor& p):processor(p),
      threshold(p.state,"control0","THRESHOLD","Compression begins above this input level.",{{-60,"-60"},{-40,"-40"},{-20,"-20"},{0,"0"}},true),
      ratio(p.state,"control1","RATIO","Compression ratio above the threshold.",{{1,"1"},{2,"2"},{4,"4"},{6,"6"},{10,"10"}},true),
      attack(p.state,"control2","ATTACK","Time taken to apply gain reduction. Slower settings preserve the initial transient.",{{.1,".1"},{1,"1"},{10,"10"},{30,"30"},{100,"100"}}),
      release(p.state,"control3","RELEASE","Base release time. Auto adapts the recovery to the programme.",{{10,"10"},{50,"50"},{120,"120"},{300,"300"},{1000,"1k"}}),
      knee(p.state,"bond_knee","KNEE","Width of the gradual transition into compression. Zero gives a hard knee.",{{0,"0"},{6,"6"},{12,"12"},{24,"24"}}),
      range(p.state,"bond_range","RANGE","Maximum compression in decibels. This limits gain reduction, not the output peak.",{{0,"0"},{12,"12"},{30,"30"},{60,"60"}}),
      highpass(p.state,"control4","HIGH-PASS","Remove bass from the detector so low frequencies trigger less compression. The audio remains full range.",{{20,"20"},{60,"60"},{150,"150"},{500,"500"}}),
      lowpass(p.state,"bond_key_lowpass","LOW-PASS","Reduce high-frequency sensitivity in the detector. The audio remains full range.",{{1000,"1k"},{4000,"4k"},{10000,"10k"},{20000,"20k"}}),
      makeup(p.state,"control5","MAKEUP","Gain applied to the compressed signal before the dry/wet blend.",{{-12,"-12"},{0,"0"},{12,"12"},{24,"24"}}),
      mix(p.state,"mix","DRY / WET","Blend the original signal with the compressed signal.",{{0,"0"},{.5,"50"},{1,"100"}}),
      output(p.state,"output","OUTPUT","Final gain after the dry/wet blend.",{{-24,"-24"},{0,"0"},{24,"24"}}) {
    setLookAndFeel(&look);
    for(auto* dial:dials())addAndMakeVisible(*dial);
    auto db=[](double v){return juce::String(v,1)+" dB";};
    for(auto* dial:{&threshold,&knee,&range,&makeup,&output})dial->format(db);
    ratio.format([](double v){return juce::String(v,1)+" : 1";});
    for(auto* dial:{&attack,&release})dial->format([](double v){return juce::String(v,v<10?1:0)+" ms";});
    highpass.format([](double v){return juce::String(v,0)+" Hz";});
    lowpass.format([](double v){return juce::String(v/1000.,1)+" kHz";},
      [](const juce::String& text){auto t=text.trim().toLowerCase();return t.containsChar('k')?t.getDoubleValue()*1000.:t.getDoubleValue();});
    mix.format([](double v){return juce::String(v*100,0)+" %";},[](const juce::String& t){return t.getDoubleValue()/100.;});
    choice(model,"bond_model",{"Original","Precision"},"Original preserves the response of earlier BOND sessions. Precision enables the new detector, stereo and envelope controls.");
    choice(topology,"bond_topology",{"Feed-forward","Feedback"},"Feed-forward responds to the input; Feedback responds to the compressed signal. A connected external key always uses feed-forward detection.");
    choice(detector,"bond_detector",{"Peak","RMS"},"Peak follows transients. RMS responds to average signal energy for gentler levelling.");
    toggle(autoRelease,"bond_auto_release","AUTO RELEASE","Adapt the release to the programme while retaining the Release control as its base timing.");
    toggle(listen,"key_listen","LISTEN TO KEY","Audition the filtered signal that drives compression. Turn off to hear the processed output.");
    link.setComponentID("bond_link");link.setName("Stereo link");link.setTitle("Stereo link");
    link.setWantsKeyboardFocus(true);
    link.setTooltip("0% allows independent left and right compression; 100% applies the greater reduction to both channels to preserve the stereo image.");
    link.setSliderStyle(juce::Slider::LinearHorizontal);
    link.setTextBoxStyle(juce::Slider::TextBoxBelow,false,80,23);
    if(auto* param=processor.state.getParameter("bond_link")){
      linkAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"bond_link",link);
      link.setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
    }
    link.textFromValueFunction=[](double v){return juce::String(v,0)+" %";};
    link.valueFromTextFunction=[](const juce::String& t){return t.getDoubleValue();};link.updateText();
    addAndMakeVisible(link);
    startTimerHz(30);timerCallback();
  }
  ~BondPanel() override {stopTimer();setLookAndFeel(nullptr);}
  BondLookAndFeel& lookAndFeel(){return look;}
  void resized() override {
    const float s=getWidth()/float(width);look.scale=s;
    link.setTextBoxStyle(juce::Slider::TextBoxBelow,false,juce::roundToInt(80*s),juce::roundToInt(23*s));
    auto place=[s](juce::Component& c,float x,float y,float w,float h){c.setBounds(juce::Rectangle<float>(x,y,w,h).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());};
    for(auto* d:dials())d->scale=s;
    place(threshold,42,328,131,190);place(ratio,175,328,126,190);
    place(attack,303,328,126,190);place(release,432,328,131,190);
    place(knee,58,536,116,112);place(range,191,536,116,112);
    place(highpass,605,393,112,154);place(lowpass,722,393,112,154);
    place(makeup,873,489,78,159);place(mix,958,489,78,159);place(output,1043,489,78,159);
    place(model,902,131,205,30);place(topology,902,183,205,30);
    place(detector,613,337,214,31);place(autoRelease,350,548,196,31);place(listen,613,578,214,32);
    place(link,888,369,215,66);
  }
  void paint(juce::Graphics& g) override {
    g.addTransform(juce::AffineTransform::scale(getWidth()/float(width)));
    const auto ink=BondLookAndFeel::ink(),muted=BondLookAndFeel::muted(),amber=BondLookAndFeel::amber();
    g.fillAll(juce::Colour(0xff070a0b));
    const juce::Rectangle<float> plate(8,8,width-16.f,height-16.f);
    juce::ColourGradient body(juce::Colour(0xff43473f),8,8,juce::Colour(0xff0c1113),width-8,height-8,false);
    body.addColour(.15,juce::Colour(0xff282e2b));body.addColour(.52,juce::Colour(0xff1b2322));
    g.setGradientFill(body);g.fillRoundedRectangle(plate,7);
    g.setColour(juce::Colour(0xff7d887c));g.drawRoundedRectangle(plate.reduced(.5f),7,1);
    g.setColour(juce::Colour(0xff070d0f));g.drawRoundedRectangle(plate.reduced(3),5,2);
    g.setColour(juce::Colour(0xffd1cebb).withAlpha(.14f));g.drawRoundedRectangle(plate.reduced(5),4,.6f);
    // Directional rolled-metal grain, never isotropic noise or changing texture.
    for(int y=14;y<height-14;y+=2){
      const float a=(y%7==0?.016f:.006f);g.setColour(juce::Colours::white.withAlpha(a));
      g.drawLine(14,float(y),width-14.f,float(y),.5f);
    }
    g.setColour(juce::Colours::white.withAlpha(.14f));g.drawLine(19,18,width-19.f,18,.7f);
    edgeWear(g,plate,1.f);
    for(float x:{20.f,width-20.f})for(float y:{20.f,height-20.f})screw(g,x,y);
    label(g,"HUNGRY GHOST AUDIO",{34,24,270,16},10.5f,muted,true);
    stampedBrand(g);
    spectralEtching(g);
    label(g,"STEREO BUS COMPRESSOR",{226,56,300,17},12,ink,true);
    label(g,"CONTROL THE MOVEMENT. KEEP THE FEEL.",{226,76,342,14},9.5f,muted);
    g.setColour(juce::Colour(0xff080e0f));g.drawLine(30,98,1130,98,2);
    g.setColour(BondLookAndFeel::copper().withAlpha(.47f));g.drawLine(30,100,1130,100,.7f);
    glass(g,{30,113,690,153});
    label(g,"GAIN REDUCTION",{46,123,238,16},10,muted,true);
    label(g,"dB",{608,123,30,16},10,muted);
    grMeter(g,154,"L",grLeft);
    grMeter(g,202,"R",grRight);
    // Peak readings deliberately name the combined bus, not fictitious L/R inputs.
    g.setColour(juce::Colour(0xff596551).withAlpha(.35f));g.drawVerticalLine(589,140,247);
    label(g,"BUS PEAK",{605,143,101,15},9,muted,true);
    label(g,"IN",{605,170,24,16},9,muted);
    label(g,peakText(peakIn),{631,167,70,21},14,ink,true,juce::Justification::right);
    label(g,"OUT",{605,208,27,16},9,muted);
    label(g,peakText(peakOut),{632,205,69,21},14,ink,true,juce::Justification::right);
    label(g,"dBFS",{647,238,54,13},9,muted,false,juce::Justification::right);
    bay(g,{738,113,392,153},"");
    label(g,"ENGINE",{758,139,125,16},10,ink,true);
    label(g,"TOPOLOGY",{758,191,132,16},10,precision?ink:muted,true);
    label(g,precision?"PRECISION DYNAMICS":"ORIGINAL SESSION RESPONSE",{758,236,352,16},10,
        precision?BondLookAndFeel::sage():amber,true);
    bay(g,{30,283,550,380},"01  COMPRESSION");
    bay(g,{595,283,252,380},"02  DETECTOR");
    bay(g,{862,283,268,380},"03  STEREO / OUTPUT");
    line(g,49,527,561,527);line(g,882,473,1110,473);
    label(g,"PROGRAMME RECOVERY",{350,591,196,15},9,muted,true,juce::Justification::centred);
    label(g,precision?(autoRelease.getToggleState()?"Adapts to the signal":"Fixed release time"):"Precision engine only",{328,613,240,18},10.5f,muted,false,juce::Justification::centred);
    label(g,"FILTERS AFFECT THE KEY ONLY",{604,554,234,14},9,muted,false,juce::Justification::centred);
    label(g,listen.getToggleState()?"KEY MONITOR ACTIVE":"Monitor the compressor's detector",{604,620,234,16},9.5f,listen.getToggleState()?amber:muted,false,juce::Justification::centred);
    label(g,"STEREO LINK",{884,337,222,18},10.5f,ink,true,juce::Justification::centred);
    label(g,"INDEPENDENT",{885,353,105,16},8.5f,muted);
    label(g,"LINKED",{1004,353,99,16},8.5f,muted,false,juce::Justification::right);
    label(g,precision?"Preserve the stereo image":"Original: fully linked",{883,442,225,16},10,muted,false,juce::Justification::centred);
    label(g,"BOND  /  HUNGRY GHOST AUDIO",{449,687,317,14},9,muted,false,juce::Justification::centred);
    if(!precision){
      label(g,"Original response retained / switch Engine for expanded control",{329,667,640,15},10,amber,false,juce::Justification::centred);
    }
  }
private:
  SuiteProcessor& processor;
  BondLookAndFeel look;
  BondDial threshold,ratio,attack,release,knee,range,highpass,lowpass,makeup,mix,output;
  juce::Slider link;
  juce::ComboBox model,topology,detector;
  juce::TextButton autoRelease,listen;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> linkAttachment;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> choices;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
  float grLeft=0,grRight=0,peakIn=0,peakOut=0;
  bool precision=true;
  std::array<BondDial*,11> dials(){return {&threshold,&ratio,&attack,&release,&knee,&range,&highpass,&lowpass,&makeup,&mix,&output};}
  void choice(juce::ComboBox& box,const char* id,juce::StringArray names,const juce::String& tooltip){
    const juce::String title=juce::String(id)=="bond_model"?"Engine":juce::String(id)=="bond_topology"?"Topology":"Detector";
    box.setComponentID(id);box.setName(title);box.setTitle(title);box.setTooltip(tooltip);box.addItemList(names,1);addAndMakeVisible(box);
    if(processor.state.getParameter(id))choices.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,id,box));
  }
  void toggle(juce::TextButton& b,const char* id,const juce::String& text,const juce::String& tooltip){
    b.setComponentID(id);b.setButtonText(text);b.setName(text);b.setTitle(text);b.setTooltip(tooltip);b.setClickingTogglesState(true);addAndMakeVisible(b);
    if(processor.state.getParameter(id))buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,id,b));
  }
  static void label(juce::Graphics& g,const juce::String& text,juce::Rectangle<float> bounds,
      float size,juce::Colour colour,bool bold=false,juce::Justification alignment=juce::Justification::left){
    g.setFont(BondLookAndFeel::font(size,bold));g.setColour(colour);g.drawText(text,bounds,alignment);
  }
  static void line(juce::Graphics& g,float x,float y,float endX,float endY){
    g.setColour(juce::Colour(0xff101e27).withAlpha(.8f));g.drawLine(x,y,endX,endY,1);
    g.setColour(juce::Colour(0xffb1c8c4).withAlpha(.22f));g.drawLine(x,y+1,endX,endY+1,.5f);
  }
  static void edgeWear(juce::Graphics& g,juce::Rectangle<float> r,float amount){
    // Deterministic, directional wear is confined to the four-pixel metal lip.
    // There are no random samples per frame and no scratches across legends.
    const auto copper=BondLookAndFeel::copper();
    const int seed=juce::roundToInt(r.getX()+r.getY()*3+r.getWidth());
    for(int i=0;i<14;++i){
      const float t=((i*47+seed)%101)/101.f;
      const float length=3.f+float((i*11+seed)%13);
      const float xx=r.getX()+8+t*(r.getWidth()-34);
      const float yy=r.getY()+1.3f+(i%3)*.65f;
      g.setColour((i%3==0?copper:juce::Colour(0xffc0b7a0)).withAlpha(amount*(i%3==0?.38f:.16f)));
      g.drawLine(xx,yy,xx+length,yy-.25f,.55f+(i%2)*.3f);
    }
    g.setColour(copper.withAlpha(.52f*amount));
    g.drawLine(r.getX()+7,r.getY()+1,r.getX()+39,r.getY()+1,.85f);
    g.drawLine(r.getRight()-49,r.getBottom()-1.1f,r.getRight()-10,r.getBottom()-1.1f,.85f);
    g.setColour(juce::Colour(0xffa9ad98).withAlpha(.22f*amount));
    g.drawLine(r.getX()+1.2f,r.getY()+8,r.getX()+1.2f,r.getY()+25,.7f);
    g.drawLine(r.getRight()-1.3f,r.getBottom()-23,r.getRight()-1.3f,r.getBottom()-8,.7f);
  }
  static void stampedBrand(juce::Graphics& g){
    // Reuse the vector die on every paint; the enamel sits inside its pressed edge.
    static const juce::Path die=[] {
      juce::GlyphArrangement glyphs;
      glyphs.addLineOfText(BondLookAndFeel::font(42,true),"BOND",0,0);
      juce::Path path;glyphs.createPath(path);
      path.applyTransform(path.getTransformToScaleToFit(33,51,163,35,false));
      return path;
    }();
    g.setColour(juce::Colour(0xffc2c5b1).withAlpha(.34f));
    g.strokePath(die,juce::PathStrokeType(1.7f),juce::AffineTransform::translation(.2f,1.1f));
    g.setColour(juce::Colour(0xff020707));
    g.strokePath(die,juce::PathStrokeType(2.1f),juce::AffineTransform::translation(-.2f,-.65f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd9ccb3),33,51,juce::Colour(0xffb3a48b),170,88,false));
    g.fillPath(die);
    g.setColour(BondLookAndFeel::copper().withAlpha(.66f));
    g.drawLine(34,91,91,91,1.1f);g.drawLine(99,91,123,91,.7f);
    g.setColour(juce::Colour(0xff0b1010));g.drawLine(34,92.2f,123,92.2f,.7f);
  }
  static void spectralEtching(juce::Graphics& g){
    // An unfilled, faceless spectral impression etched into the header itself.
    // Static interference contours are decorative, separate from live metering.
    static const std::array<juce::Path,7> traces=[] {
      std::array<juce::Path,7> paths;
      for(int i=0;i<7;++i){
        const float inset=i*2.6f,left=582+inset,right=685-inset,
            top=26+i*2.8f,foot=85-i*.7f,centre=633.5f;
        auto& p=paths[static_cast<size_t>(i)];
        p.startNewSubPath(left,foot);
        p.cubicTo(left+15,foot-5,left+15,71,left+23,61);
        p.cubicTo(left+26,51,centre-19,top,centre,top);
        p.cubicTo(centre+19,top,right-26,51,right-23,61);
        p.cubicTo(right-15,71,right-15,foot-5,right,foot);
      }
      return paths;
    }();
    for(size_t i=0;i<traces.size();++i){
      g.setColour(juce::Colours::black.withAlpha(.75f));
      g.strokePath(traces[i],juce::PathStrokeType(1.25f),juce::AffineTransform::translation(-.3f,-.5f));
      g.setColour((i==0?BondLookAndFeel::copper():BondLookAndFeel::sage()).withAlpha(i==0?.40f:.13f+float(i)*.024f));
      g.strokePath(traces[i],juce::PathStrokeType(i==0?.8f:.65f));
    }
    juce::Path breath;
    breath.startNewSubPath(607,84);breath.cubicTo(617,81,621,76,627,74);
    breath.startNewSubPath(637,70);breath.cubicTo(643,77,648,79,655,85);
    breath.startNewSubPath(624,86);breath.cubicTo(630,84,634,82,638,78);
    g.setColour(BondLookAndFeel::sage().withAlpha(.20f));g.strokePath(breath,juce::PathStrokeType(.7f));
    juce::Path fracturedRing;
    fracturedRing.addCentredArc(634,58,61,30,0,-1.88f,-1.1f,true);
    fracturedRing.addCentredArc(634,58,61,30,0,.85f,1.73f,true);
    g.setColour(BondLookAndFeel::copper().withAlpha(.26f));g.strokePath(fracturedRing,juce::PathStrokeType(.7f));
  }
  static void screw(juce::Graphics& g,float x,float y){
    juce::Path rubbed;
    rubbed.addCentredArc(x,y,6.4f,6.4f,0,-2.4f,-.4f,true);
    g.setColour(BondLookAndFeel::copper().withAlpha(.35f));g.strokePath(rubbed,juce::PathStrokeType(.8f));
    g.setColour(juce::Colour(0xffd4d0bd).withAlpha(.21f));g.drawLine(x-7,y-3.4f,x-4.5f,y-5.5f,.6f);
    g.setColour(juce::Colour(0xff0b110d));g.fillEllipse(x-4,y-4,8,8);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffadb4a3),x-3,y-3,juce::Colour(0xff2b362a),x+3,y+3,false));g.fillEllipse(x-3,y-3,6,6);
    g.setColour(juce::Colour(0xff131a12));g.drawLine(x-1.7f,y+.8f,x+1.7f,y-.8f,1.2f);
  }
  static void glass(juce::Graphics& g,juce::Rectangle<float> r){
    g.setColour(juce::Colour(0xff070e0f));g.fillRoundedRectangle(r.expanded(2),4);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff101e1a),r.getX(),r.getY(),juce::Colour(0xff030807),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,3);
    g.setColour(juce::Colour(0xff707e6d));g.drawRoundedRectangle(r,3,.7f);
    g.setColour(juce::Colours::black.withAlpha(.65f));g.drawLine(r.getX()+2,r.getY()+2,r.getRight()-2,r.getY()+2,3);
    g.setColour(juce::Colour(0xffd5e3df).withAlpha(.035f));g.fillRect(r.withHeight(r.getHeight()*.43f).reduced(2,0));
  }
  static void bay(juce::Graphics& g,juce::Rectangle<float> r,const juce::String& title){
    // A narrow cut edge encloses the anodised face. Broad highlights remain matte.
    g.setColour(juce::Colours::black.withAlpha(.5f));g.fillRoundedRectangle(r.translated(0,2),3);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff728277),r.getX(),r.getY(),juce::Colour(0xff060d10),r.getX(),r.getBottom(),false));g.fillRoundedRectangle(r,3);
    auto face=r.reduced(1.5f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff272f2c),face.getX(),face.getY(),juce::Colour(0xff141d1d),face.getRight(),face.getBottom(),false));g.fillRoundedRectangle(face,2);
    for(float y=face.getY()+2;y<face.getBottom()-2;y+=2){
      g.setColour(juce::Colours::white.withAlpha(.006f));g.drawLine(face.getX()+2,y,face.getRight()-2,y,.5f);
    }
    g.setColour(juce::Colour(0xff0c1c25).withAlpha(.9f));g.drawRoundedRectangle(r,3,.7f);
    g.setColour(juce::Colour(0xffc9dbd7).withAlpha(.33f));g.drawLine(r.getX()+3,r.getY()+1,r.getRight()-3,r.getY()+1,.7f);
    edgeWear(g,r,.6f);
    if(title.isNotEmpty()){
      g.setColour(BondLookAndFeel::copper().withAlpha(.72f));g.fillRect(r.getX()+8,r.getY()+18,2.f,9.f);
      label(g,title,{r.getX()+18,r.getY()+14,r.getWidth()-36,19},10.5f,BondLookAndFeel::ink(),true);
      line(g,r.getX()+18,r.getY()+41,r.getRight()-18,r.getY()+41);
    }
  }
  void grMeter(juce::Graphics& g,float y,const char* channel,float reading){
    const auto amber=BondLookAndFeel::amber();
    label(g,channel,{46,y+1,20,18},12,BondLookAndFeel::ink(),true);
    const float x=74,w=429;
    auto map=[](float v){return std::log1p(juce::jlimit(0.f,24.f,v)) / std::log(25.f);};
    g.setColour(juce::Colours::black.withAlpha(.7f));g.fillRoundedRectangle(x,y,w,13,2);
    const float lit=map(reading);
    for(int i=0;i<72;++i){const float at=i/72.f;g.setColour(amber.withAlpha(at<lit?.88f:.09f));g.fillRect(x+2+i*(w-4)/72.f,y+3,(w-4)/72.f-1.7f,7.f);}
    for(float tick:{0.f,1.f,2.f,3.f,6.f,9.f,12.f,18.f,24.f}){
      const float xx=x+map(tick)*w;
      g.setColour(BondLookAndFeel::muted().withAlpha(.65f));g.drawLine(xx,y+16,xx,y+19,.7f);
      label(g,juce::String(tick,0),{xx-10,y+20,20,12},8.5f,BondLookAndFeel::muted(),false,juce::Justification::centred);
    }
    label(g,juce::String(reading,1),{515,y-3,57,24},18,amber,true,juce::Justification::right);
  }
  static juce::String peakText(float value){
    if(value<.000032f)return juce::String::fromUTF8("\xe2\x88\x92\xe2\x88\x9e");
    return juce::String(juce::Decibels::gainToDecibels(value,-90.f),1);
  }
  void timerCallback() override {
    if(auto* p=processor.state.getRawParameterValue("bond_model"))precision=p->load()>.5f;
    for(auto* c:std::array<juce::Component*,7>{&topology,&detector,&knee,&range,&lowpass,&link,&autoRelease})c->setEnabled(precision);
    grLeft=juce::jmax(processor.reductionLeft.load(),grLeft*.86f);
    grRight=juce::jmax(processor.reductionRight.load(),grRight*.86f);
    peakIn=juce::jmax(processor.inputPeak.load(),peakIn*.87f);
    peakOut=juce::jmax(processor.outputPeak.load(),peakOut*.87f);
    repaint();
  }
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BondPanel)
};
} // namespace hungryghost
