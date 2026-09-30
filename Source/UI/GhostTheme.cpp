#include "GhostTheme.h"
#include <BinaryData.h>
#include <array>
#include <cmath>

namespace hungryghost {
namespace {
juce::Font type(float height, bool bold = false) {
  return juce::Font(juce::FontOptions("Segoe UI",height,bold?juce::Font::bold:juce::Font::plain));
}
juce::Font font(float height,bool bold=false){return type(height*1.2f,bold);}
juce::Colour sage(){return GhostTheme::accent();}
juce::Colour amber(){return juce::Colour(0xffdca16c);}
juce::Colour copper(){return juce::Colour(0xff986a4e);}
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
  static void edgeWear(juce::Graphics& g,juce::Rectangle<float> r,float amount){
    // Deterministic, directional wear is confined to the four-pixel metal lip.
    // There are no random samples per frame and no scratches across legends.
    const auto copperColour=copper();
    const int seed=juce::roundToInt(r.getX()+r.getY()*3+r.getWidth());
    for(int i=0;i<14;++i){
      const float t=((i*47+seed)%101)/101.f;
      const float length=3.f+float((i*11+seed)%13);
      const float xx=r.getX()+8+t*(r.getWidth()-34);
      const float yy=r.getY()+1.3f+(i%3)*.65f;
      g.setColour((i%3==0?copperColour:juce::Colour(0xffc0b7a0)).withAlpha(amount*(i%3==0?.38f:.16f)));
      g.drawLine(xx,yy,xx+length,yy-.25f,.55f+(i%2)*.3f);
    }
    g.setColour(copperColour.withAlpha(.52f*amount));
    g.drawLine(r.getX()+7,r.getY()+1,r.getX()+39,r.getY()+1,.85f);
    g.drawLine(r.getRight()-49,r.getBottom()-1.1f,r.getRight()-10,r.getBottom()-1.1f,.85f);
    g.setColour(juce::Colour(0xffa9ad98).withAlpha(.22f*amount));
    g.drawLine(r.getX()+1.2f,r.getY()+8,r.getX()+1.2f,r.getY()+25,.7f);
    g.drawLine(r.getRight()-1.3f,r.getBottom()-23,r.getRight()-1.3f,r.getBottom()-8,.7f);
  }
  static void screw(juce::Graphics& g,float x,float y){
    juce::Path rubbed;
    rubbed.addCentredArc(x,y,6.4f,6.4f,0,-2.4f,-.4f,true);
    g.setColour(copper().withAlpha(.35f));g.strokePath(rubbed,juce::PathStrokeType(.8f));
    g.setColour(juce::Colour(0xffd4d0bd).withAlpha(.21f));g.drawLine(x-7,y-3.4f,x-4.5f,y-5.5f,.6f);
    g.setColour(juce::Colour(0xff0b110d));g.fillEllipse(x-4,y-4,8,8);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffadb4a3),x-3,y-3,juce::Colour(0xff2b362a),x+3,y+3,false));g.fillEllipse(x-3,y-3,6,6);
    g.setColour(juce::Colour(0xff131a12));g.drawLine(x-1.7f,y+.8f,x+1.7f,y-.8f,1.2f);
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
juce::Colour GhostTheme::background(){return juce::Colour(0xff070a0b);}
juce::Colour GhostTheme::panel(){return juce::Colour(0xff1b2322);}
juce::Colour GhostTheme::ink(){return juce::Colour(0xffe9dfcd);}
juce::Colour GhostTheme::muted(){return juce::Colour(0xffaeb6ab);}
juce::Colour GhostTheme::accent(){return juce::Colour(0xffa1c8b6);}
juce::Colour GhostTheme::line(){return juce::Colour(0xff34433e);}
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
  setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff091012));
  setColour(juce::TextEditor::textColourId,ink());
  setColour(juce::TextEditor::highlightColourId,accent().withAlpha(.26f));
  setColour(juce::TextEditor::outlineColourId,accent());
}
bool GhostTheme::hasAssets() const {
  return dialAtlas.getWidth() == 3072 && dialAtlas.getHeight() == 2048 &&
         faceplate.isValid() && display.isValid() && keycap.isValid() &&
         selectedKeycap.isValid();
}

void GhostTheme::paintChassis(juce::Graphics& g,juce::Rectangle<float> bounds) const {
  g.setColour(background());g.fillRect(bounds);
  // Work in a local material space so all physical detail follows host scaling.
  juce::Graphics::ScopedSaveState save(g);
  g.addTransform(juce::AffineTransform::scale(scale).translated(bounds.getX(),bounds.getY()));
  const float w=bounds.getWidth()/scale,h=bounds.getHeight()/scale;
  const auto plate=juce::Rectangle<float>(8,8,w-16,h-16);
  juce::ColourGradient body(juce::Colour(0xff43473f),8,8,juce::Colour(0xff0c1113),w-8,h-8,false);
  body.addColour(.15,juce::Colour(0xff282e2b));body.addColour(.52,juce::Colour(0xff1b2322));
  g.setGradientFill(body);g.fillRoundedRectangle(plate,7);
  g.setColour(juce::Colour(0xff7d887c));g.drawRoundedRectangle(plate.reduced(.5f),7,1);
  g.setColour(juce::Colour(0xff070d0f));g.drawRoundedRectangle(plate.reduced(3),5,2);
  g.setColour(juce::Colour(0xffd1cebb).withAlpha(.14f));g.drawRoundedRectangle(plate.reduced(5),4,.6f);
  for(float y=14;y<h-14;y+=2){
    g.setColour(juce::Colours::white.withAlpha(int(y)%7==0?.016f:.006f));
    g.drawLine(14,y,w-14,y,.5f);
  }
  g.setColour(juce::Colours::white.withAlpha(.14f));g.drawLine(19,18,w-19,18,.7f);
  edgeWear(g,plate,1.f);
  for(float x:{20.f,w-20})for(float y:{20.f,h-20})screw(g,x,y);
}

void GhostTheme::paintPanel(juce::Graphics& g,juce::Rectangle<float> r,juce::Colour tint) const {
  if(r.isEmpty())return;
  // Supplied geometry is already in the caller's drawing space. Never apply scale here.
  const float s=juce::jlimit(.55f,2.f,r.getHeight()/220.f);
  g.setColour(juce::Colours::black.withAlpha(.5f));g.fillRoundedRectangle(r.translated(0,2*s),3*s);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff728277),r.getX(),r.getY(),juce::Colour(0xff060d10),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,3*s);
  const auto face=r.reduced(1.5f*s);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff272f2c).interpolatedWith(tint,.055f),face.getX(),face.getY(),juce::Colour(0xff141d1d).interpolatedWith(tint,.024f),face.getRight(),face.getBottom(),false));g.fillRoundedRectangle(face,2*s);
  g.setColour(juce::Colours::white.withAlpha(.006f));
  for(float y=face.getY()+2*s;y<face.getBottom()-2*s;y+=2*s)g.drawLine(face.getX()+2*s,y,face.getRight()-2*s,y,.5f*s);
  g.setColour(juce::Colour(0xff0c1c25).withAlpha(.9f));g.drawRoundedRectangle(r,3*s,.7f*s);
  g.setColour(juce::Colour(0xffc9dbd7).withAlpha(.33f));g.drawLine(r.getX()+3*s,r.getY()+s,r.getRight()-3*s,r.getY()+s,.7f*s);
  juce::Graphics::ScopedSaveState save(g);
  g.addTransform(juce::AffineTransform::scale(s).translated(r.getX(),r.getY()));
  edgeWear(g,{0,0,r.getWidth()/s,r.getHeight()/s},.6f);
}

void GhostTheme::paintDisplay(juce::Graphics& g,juce::Rectangle<float> r) const {
  const float s=scale;
  g.setColour(juce::Colour(0xff050a0c));g.fillRoundedRectangle(r.expanded(2*s),4*s);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff111d19),r.getX(),r.getY(),juce::Colour(0xff030807),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,3*s);
  g.setColour(juce::Colour(0xff707e6d));g.drawRoundedRectangle(r,3*s,.7f*s);
  g.setColour(juce::Colours::black.withAlpha(.65f));g.drawLine(r.getX()+2*s,r.getY()+2*s,r.getRight()-2*s,r.getY()+2*s,3*s);
  g.setColour(juce::Colour(0xffd5e3df).withAlpha(.035f));g.fillRect(r.withHeight(r.getHeight()*.43f).reduced(2*s,0));
}

void GhostTheme::paintWordmark(juce::Graphics& g,const juce::String& text,juce::Rectangle<float> bounds,float fontHeight) const {
  if(text.isEmpty()||bounds.isEmpty())return;
  juce::GlyphArrangement glyphs;glyphs.addLineOfText(type(fontHeight,true),text,0,0);
  juce::Path die;glyphs.createPath(die);const auto source=die.getBounds();
  if(source.isEmpty())return;
  const float fit=juce::jmin(1.f,juce::jmin(bounds.getWidth()/source.getWidth(),bounds.getHeight()/source.getHeight()));
  die.applyTransform(juce::AffineTransform::translation(-source.getX(),-source.getY()).scaled(fit).translated(bounds.getX(),bounds.getCentreY()-source.getHeight()*fit*.5f));
  const float s=fontHeight/42.f;
  g.setColour(juce::Colour(0xffc2c5b1).withAlpha(.34f));g.strokePath(die,juce::PathStrokeType(1.7f*s),juce::AffineTransform::translation(.2f*s,1.1f*s));
  g.setColour(juce::Colour(0xff020707));g.strokePath(die,juce::PathStrokeType(2.1f*s),juce::AffineTransform::translation(-.2f*s,-.65f*s));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd9ccb3),bounds.getX(),bounds.getY(),juce::Colour(0xffb3a48b),bounds.getRight(),bounds.getBottom(),false));g.fillPath(die);
}

void GhostTheme::paintSpectralMark(juce::Graphics& g,juce::Rectangle<float> bounds,juce::Colour accentColour) const {
  if(bounds.isEmpty())return;
  juce::Graphics::ScopedSaveState save(g);
  const float fit=juce::jmin(bounds.getWidth()/122.f,bounds.getHeight()/64.f);
  g.addTransform(juce::AffineTransform::translation(-573,-24).scaled(fit).translated(bounds.getCentreX()-61*fit,bounds.getCentreY()-32*fit));
  static const std::array<juce::Path,7> traces=[] {
    std::array<juce::Path,7> paths;
    for(int i=0;i<7;++i){
      const float inset=i*2.6f,left=582+inset,right=685-inset,top=26+i*2.8f,foot=85-i*.7f,centre=633.5f;
      auto& p=paths[static_cast<size_t>(i)];p.startNewSubPath(left,foot);
      p.cubicTo(left+15,foot-5,left+15,71,left+23,61);p.cubicTo(left+26,51,centre-19,top,centre,top);
      p.cubicTo(centre+19,top,right-26,51,right-23,61);p.cubicTo(right-15,71,right-15,foot-5,right,foot);
    }return paths;
  }();
  for(size_t i=0;i<traces.size();++i){
    g.setColour(juce::Colours::black.withAlpha(.75f));g.strokePath(traces[i],juce::PathStrokeType(1.25f),juce::AffineTransform::translation(-.3f,-.5f));
    g.setColour((i==0?copper():accentColour).withAlpha(i==0?.48f:.17f+float(i)*.028f));g.strokePath(traces[i],juce::PathStrokeType(i==0?.8f:.7f));
  }
  juce::Path breath;breath.startNewSubPath(607,84);breath.cubicTo(617,81,621,76,627,74);
  breath.startNewSubPath(637,70);breath.cubicTo(643,77,648,79,655,85);
  breath.startNewSubPath(624,86);breath.cubicTo(630,84,634,82,638,78);
  g.setColour(accentColour.withAlpha(.24f));g.strokePath(breath,juce::PathStrokeType(.7f));
  juce::Path ring;ring.addCentredArc(634,58,61,30,0,-1.88f,-1.1f,true);ring.addCentredArc(634,58,61,30,0,.85f,1.73f,true);
  g.setColour(copper().withAlpha(.32f));g.strokePath(ring,juce::PathStrokeType(.7f));
}

void GhostTheme::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider) {
  const float s=scale;const auto area=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(2*s);
  const float size=juce::jmin(area.getWidth(),area.getHeight()),cx=area.getCentreX(),cy=area.getCentreY();
  if(size<8*s)return;
  const float radius=size*.355f,collar=juce::jlimit(2.5f*s,8.f*s,size*.06f);
  const float angle=start+(end-start)*position;const bool enabled=slider.isEnabled();
  const auto tint=slider.findColour(juce::Slider::thumbColourId);
  const auto point=[&](float a,float r){return juce::Point<float>(cx+std::sin(a)*r,cy-std::cos(a)*r);};
  g.beginTransparencyLayer(enabled?1.f:.36f);
  for(int i=0;i<=10;++i){
    const float a=start+(end-start)*i*.1f;g.setColour(muted().withAlpha(i%5==0?.7f:.32f));
    g.drawLine({point(a,size*.433f),point(a,size*(i%5==0?.467f:.455f))},.75f*s);
  }
  juce::Path track,active;track.addCentredArc(cx,cy,radius+4*s,radius+4*s,0,start,end,true);
  active.addCentredArc(cx,cy,radius+4*s,radius+4*s,0,start,angle,true);
  g.setColour(juce::Colours::black.withAlpha(.9f));g.strokePath(track,juce::PathStrokeType(2.6f*s));
  g.setColour(tint.withAlpha(.65f));g.strokePath(active,juce::PathStrokeType(1.25f*s));
  const auto rim=juce::Rectangle<float>(radius*2,radius*2).withCentre({cx,cy});
  g.setColour(juce::Colours::black.withAlpha(.75f));g.fillEllipse(rim.expanded(2*s).translated(0,3*s));
  juce::ColourGradient edge(juce::Colour(0xffb9b9aa),cx-radius,cy-radius,juce::Colour(0xff0b1012),cx+radius,cy+radius,false);
  edge.addColour(.2,juce::Colour(0xff606b63));edge.addColour(.48,juce::Colour(0xff171e1f));edge.addColour(.76,juce::Colour(0xff7a857a));
  g.setGradientFill(edge);g.fillEllipse(rim);
  // The collar and its light remain fixed; only the engraved index moves.
  const int divisions=size<60*s?40:64;
  for(int i=0;i<divisions;++i){const float a=juce::MathConstants<float>::twoPi*i/float(divisions);
    g.setColour(juce::Colours::white.withAlpha((.5f+.5f*std::cos(a+.65f))*.2f));
    g.drawLine({point(a,radius-.8f*s),point(a,radius-collar*.8f)},.65f*s);
  }
  const auto face=rim.reduced(collar);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff424b44),face.getX(),face.getY(),juce::Colour(0xff0d1718),face.getRight(),face.getBottom(),false));g.fillEllipse(face);
  g.setColour(juce::Colour(0xffd2e0da).withAlpha(.38f));g.drawEllipse(face,.7f*s);
  g.setColour(juce::Colours::black.withAlpha(.25f));g.drawEllipse(face.reduced(2*s),.8f*s);
  const float indexEnd=radius-collar-3*s,indexStart=juce::jmin(indexEnd-2*s,radius*.42f);
  g.setColour(juce::Colours::black.withAlpha(.8f));g.drawLine({point(angle,indexStart).translated(.7f*s,.7f*s),point(angle,indexEnd).translated(.7f*s,.7f*s)},3*s);
  g.setColour(tint);g.drawLine({point(angle,indexStart),point(angle,indexEnd)},2.2f*s);
  if(enabled&&(slider.isMouseOverOrDragging()||slider.hasKeyboardFocus(true))){
    g.setColour((slider.hasKeyboardFocus(true)?amber():tint).withAlpha(.55f));g.drawEllipse(rim.expanded(2*s),.9f*s);
  }
  g.endTransparencyLayer();
}

int GhostTheme::getSliderThumbRadius(juce::Slider& slider){
  const bool compact=bool(slider.getProperties()["compact"])||bool(slider.getProperties()["detail"]);
  return juce::roundToInt((slider.isHorizontal()&&compact?11.f:16.f)*scale);
}
void GhostTheme::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& slider){
  const float s=scale;const bool vertical=style==juce::Slider::LinearVertical||style==juce::Slider::LinearBarVertical;
  const bool compact=bool(slider.getProperties()["compact"])||bool(slider.getProperties()["detail"])||(!vertical&&h<32*s);
  const bool enabled=slider.isEnabled(),over=enabled&&slider.isMouseOverOrDragging();
  const auto tint=slider.findColour(juce::Slider::thumbColourId);
  const float cx=vertical?x+w*.5f:pos,cy=vertical?pos:y+h*.5f;
  auto slot=vertical?juce::Rectangle<float>(cx-4*s,float(y)-2*s,8*s,float(h)+4*s):juce::Rectangle<float>(float(x)-2*s,cy-3*s,float(w)+4*s,6*s);
  mount(g,slot.expanded(s),s);g.setColour(juce::Colour(0xff030708));g.fillRoundedRectangle(slot,2*s);
  g.setColour(juce::Colour(0xff798d8b).withAlpha(.3f));
  if(vertical)g.drawLine(slot.getRight(),slot.getY()+2*s,slot.getRight(),slot.getBottom()-2*s,.7f*s);
  else g.drawLine(slot.getX()+2*s,slot.getBottom(),slot.getRight()-2*s,slot.getBottom(),.7f*s);
  const int divisions=vertical?10:4;
  for(int i=0;i<=divisions;++i){const bool major=i==0||i==divisions||i==divisions/2;
    g.setColour(muted().withAlpha(enabled?.6f:.25f));
    if(vertical){const float at=y+h*i/float(divisions);g.drawLine(cx-22*s,at,cx-(major?11.f:14.f)*s,at,.8f*s);g.drawLine(cx+(major?11.f:14.f)*s,at,cx+22*s,at,.8f*s);}
    else if(!compact){const float at=x+w*i/float(divisions);g.drawLine(at,cy-13*s,at,cy-(major?7.f:9.f)*s,.8f*s);}
  }
  auto cap=juce::Rectangle<float>((vertical?38.f:compact?14.f:22.f)*s,(vertical?22.f:compact?18.f:28.f)*s).withCentre({cx,cy});
  g.beginTransparencyLayer(enabled?1.f:.42f);
  g.setColour(juce::Colours::black.withAlpha(.65f));g.fillRoundedRectangle(cap.expanded(1.5f*s,s).translated(0,2*s),2.4f*s);
  juce::ColourGradient edge(juce::Colour(0xffc5c9b9),cap.getX(),cap.getY(),juce::Colour(0xff101b1c),cap.getRight(),cap.getBottom(),false);
  edge.addColour(.3,juce::Colour(0xff6b786f));edge.addColour(.64,juce::Colour(0xff26372e));g.setGradientFill(edge);g.fillRoundedRectangle(cap,2.4f*s);
  auto face=cap.reduced(1.8f*s).translated(0,slider.isMouseButtonDown()?.6f*s:0.f);
  juce::ColourGradient metal(over?juce::Colour(0xff7d8779):juce::Colour(0xff5b6a60),face.getX(),face.getY(),juce::Colour(0xff192625),face.getX(),face.getBottom(),false);
  metal.addColour(.42,juce::Colour(0xff46574b));metal.addColour(.52,juce::Colour(0xff2b3b33));g.setGradientFill(metal);g.fillRoundedRectangle(face,1.3f*s);
  const float spacing=(compact?4.5f:vertical?5.f:7.f)*s;
  for(float offset:{-spacing,spacing}){g.setColour(juce::Colour(0xff10201e));g.drawLine(face.getX()+2*s,cy+offset,face.getRight()-2*s,cy+offset,1.25f*s);
    g.setColour(juce::Colour(0xffbac3ae).withAlpha(.3f));g.drawLine(face.getX()+2*s,cy+offset+.8f*s,face.getRight()-2*s,cy+offset+.8f*s,.6f*s);}
  g.setColour(juce::Colour(0xff0a1213));
  if(vertical){g.drawLine(face.getX()+3*s,cy,face.getRight()-3*s,cy,3*s);g.setColour(tint);g.drawLine(face.getX()+3*s,cy-.5f*s,face.getRight()-3*s,cy-.5f*s,1.3f*s);}
  else{g.fillRoundedRectangle(cx-1.7f*s,cy-4.3f*s,3.4f*s,8.6f*s,.6f*s);g.setColour(tint);g.fillRect(cx-.6f*s,cy-3.2f*s,1.2f*s,6.4f*s);}
  if(enabled&&slider.hasKeyboardFocus(true)){g.setColour(amber().withAlpha(.9f));g.drawRoundedRectangle(cap.expanded(1.8f*s),3*s,.9f*s);}
  g.endTransparencyLayer();
}
  juce::Label* GhostTheme::createSliderTextBox(juce::Slider& slider) {
    auto* label=juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->getProperties().set("ghostReadout",true);
    label->setTitle(slider.getName()+" value");
    label->setColour(juce::Label::textColourId,ink());
    label->setColour(juce::Label::backgroundColourId,juce::Colour(0xff091419));
    label->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label->setColour(juce::Label::textWhenEditingColourId,ink());
    label->setColour(juce::Label::backgroundWhenEditingColourId,juce::Colour(0xff091419));
    label->setBorderSize(juce::BorderSize<int>(0));
    return label;
  }
  void GhostTheme::fillTextEditorBackground(juce::Graphics& g,int w,int h,juce::TextEditor& editor) {
    readout(g,{0,0,float(w),float(h)},scale,editor.isEnabled(),true);
  }
  void GhostTheme::drawTextEditorOutline(juce::Graphics& g,int w,int h,juce::TextEditor& editor) {
    if(editor.hasKeyboardFocus(true)){
      g.setColour(amber().withAlpha(.9f));
      g.drawRoundedRectangle(juce::Rectangle<float>(0,0,float(w),float(h)).reduced(1.5f*scale),1.5f*scale,.8f*scale);
    }
  }
  void GhostTheme::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool over,bool down) {
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
  void GhostTheme::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool over,bool down) {
    const float s=scale;auto r=b.getLocalBounds().toFloat().reduced(6*s,0);
    if(isLatchingControl(b))r.removeFromLeft((b.getWidth()<50*s?5.f:13.f)*s);
    if(b.isEnabled()&&(down||b.getToggleState()))r=r.translated(0,1.1f*s);
    g.setFont(font(10.9f*s,true));
    const auto text=b.getButtonText().toUpperCase();
    g.setColour(juce::Colours::black.withAlpha(.7f));g.drawFittedText(text,r.translated(0,-.6f*s).toNearestInt(),juce::Justification::centred,1,.8f);
    g.setColour((over||b.getToggleState()?ink():muted()).withMultipliedAlpha(b.isEnabled()?1.f:.35f));
    g.drawFittedText(text,r.toNearestInt(),juce::Justification::centred,1,.8f);
  }
  void GhostTheme::drawComboBox(juce::Graphics& g,int w,int h,bool down,int,int,int,int,juce::ComboBox& box) {
    const float s=scale;const bool enabled=box.isEnabled(),open=down||box.isPopupActive();
    auto r=juce::Rectangle<float>(.6f*s,.6f*s,w-1.2f*s,h-1.2f*s);
    mount(g,r,s);
    const auto window=r.reduced(2.5f*s);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff030709),window.getX(),window.getY(),juce::Colour(0xff0d1718),window.getX(),window.getBottom(),false));
    g.fillRoundedRectangle(window,1.8f*s);
    g.setColour(juce::Colour(0xffd6e6db).withAlpha(.06f));
    g.fillRect(window.withHeight(window.getHeight()*.46f));
    const float keyWidth=juce::jmin(25*s,window.getWidth()*.3f);
    auto key=window.withWidth(keyWidth).withX(window.getRight()-keyWidth);
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
  void GhostTheme::positionComboBoxText(juce::ComboBox& box,juce::Label& label) {
    const float inset=juce::jmin(10*scale,box.getWidth()*.08f);
    const float arrow=juce::jmin(25*scale,(box.getWidth()-6*scale)*.3f);
    label.setBounds(juce::roundToInt(inset),0,juce::jmax(1,box.getWidth()-juce::roundToInt(inset+arrow+10*scale)),box.getHeight());
    label.getProperties().set("ghostSelectorLabel",true);
    label.setColour(juce::Label::textColourId,ink());
    label.setColour(juce::Label::backgroundColourId,juce::Colours::transparentBlack);
    label.setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
    label.setFont(getComboBoxFont(box));
  }
  void GhostTheme::drawComboBoxTextWhenNothingSelected(juce::Graphics& g,juce::ComboBox& box,juce::Label& label) {
    // JUCE calls this in the selector's coordinate space, not the label's.
    g.setFont(getLabelFont(label));
    g.setColour(muted().withAlpha(box.isEnabled()?.72f:.3f));
    g.drawText(box.getTextWhenNothingSelected(),label.getBounds().toFloat().reduced(4*scale,0),label.getJustificationType());
  }
  juce::Font GhostTheme::getPopupMenuFont() {return font(12.f*scale);}
  void GhostTheme::getIdealPopupMenuItemSize(const juce::String& text,bool separator,int standardHeight,int& w,int& h) {
    h=separator?juce::roundToInt(9*scale):juce::jmax(standardHeight,juce::roundToInt(31*scale));
    w=separator?juce::roundToInt(80*scale):juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(),text)+juce::roundToInt(62*scale);
  }
  void GhostTheme::drawPopupMenuBackground(juce::Graphics& g,int w,int h) {
    const float s=scale;auto r=juce::Rectangle<float>(0,0,float(w),float(h));
    g.fillAll(juce::Colour(0xff080e0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222b29),0,0,juce::Colour(0xff10191a),0,float(h),false));g.fillRect(r.reduced(s));
    g.setColour(juce::Colour(0xff9baea6).withAlpha(.7f));g.drawRect(r.reduced(.5f*s),s);
    g.setColour(juce::Colour(0xff060f15));g.drawRect(r.reduced(2*s),s);
  }
  void GhostTheme::drawPopupMenuItem(juce::Graphics& g,const juce::Rectangle<int>& area,bool separator,
      bool active,bool highlighted,bool ticked,bool submenu,const juce::String& text,
      const juce::String& shortcut,const juce::Drawable* icon,const juce::Colour*) {
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
  void GhostTheme::drawPopupMenuSectionHeader(juce::Graphics& g,const juce::Rectangle<int>& area,const juce::String& title) {
    g.setFont(font(10.f*scale,true));g.setColour(sage());
    g.drawText(title.toUpperCase(),area.toFloat().reduced(14*scale,0),juce::Justification::centredLeft);
  }
  void GhostTheme::drawPopupMenuUpDownArrow(juce::Graphics& g,int w,int h,bool up) {
    g.setColour(juce::Colour(0xff162421));g.fillRect(0,0,w,h);
    const float x=w*.5f,y=h*.5f,d=up?-1.f:1.f;juce::Path p;
    p.startNewSubPath(x-4*scale,y-d*2*scale);p.lineTo(x,y+d*2*scale);p.lineTo(x+4*scale,y-d*2*scale);
    g.setColour(ink());g.strokePath(p,juce::PathStrokeType(scale));
  }

juce::Font GhostTheme::getComboBoxFont(juce::ComboBox& box){return type(juce::jmin(16.f*scale,box.getHeight()*.52f));}
juce::Font GhostTheme::getLabelFont(juce::Label& label){
  if(auto* box=dynamic_cast<juce::ComboBox*>(label.getParentComponent()))return getComboBoxFont(*box);
  if(auto* slider=dynamic_cast<juce::Slider*>(label.getParentComponent())){
    const auto& p=slider->getProperties();
    const bool suite=bool(p["suite"]),detail=bool(p["detail"]),compact=bool(p["compact"]);
    float desired=suite?(compact?14.f:bool(p["primary"])?28.f:18.f):(bool(p["large"])?42.f:detail?14.f:28.f);
    desired=juce::jmin(desired*scale,juce::jmax(10.f*scale,label.getHeight()-7.f*scale));
    auto f=type(desired,!detail);
    const float width=juce::GlyphArrangement::getStringWidth(f,label.getText());
    if(width>0&&width>label.getWidth()-10*scale)f=f.withHeight(juce::jmax(10.f*scale,desired*(label.getWidth()-10*scale)/width));
    return f;
  }
  return type(15*scale);
}
void GhostTheme::drawLabel(juce::Graphics& g,juce::Label& label){
  const auto r=label.getLocalBounds().toFloat();
  // Detail sliders can create their value label before inheriting this theme.
  const bool value=bool(label.getProperties()["ghostReadout"])||dynamic_cast<juce::Slider*>(label.getParentComponent())!=nullptr;
  const bool selector=bool(label.getProperties()["ghostSelectorLabel"]);
  if(value){
    const bool parentFocus=label.getParentComponent()!=nullptr&&label.getParentComponent()->hasKeyboardFocus(true);
    readout(g,r,scale,label.isEnabled(),label.isMouseOver(true)||label.hasKeyboardFocus(true)||parentFocus);
  }else g.fillAll(label.findColour(juce::Label::backgroundColourId));
  if(!label.isBeingEdited()){
    const float alpha=label.isEnabled()?1.f:.4f;g.setFont(getLabelFont(label));
    const auto textBounds=value||selector?r.reduced(4*scale,0).toNearestInt():getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
    if(!value){g.setColour(juce::Colours::black.withAlpha(.65f*alpha));g.drawFittedText(label.getText(),textBounds.translated(0,juce::roundToInt(.6f*scale)),label.getJustificationType(),1,label.getMinimumHorizontalScale());}
    g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
    g.drawFittedText(label.getText(),textBounds,label.getJustificationType(),1,label.getMinimumHorizontalScale());
  }
  if(!value&&!selector){g.setColour(label.findColour(juce::Label::outlineColourId).withMultipliedAlpha(label.isEnabled()?1.f:.4f));g.drawRect(label.getLocalBounds());}
}
} // namespace hungryghost
