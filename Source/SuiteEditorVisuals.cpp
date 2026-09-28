#include "SuiteEditor.h"
#include <cmath>

namespace hungryghost {
namespace {
using L=EditorLayout;
float db(float x){return juce::Decibels::gainToDecibels(x,-90.f);}
bool delayLayout(L l){return l==L::Echo||l==L::Tape||l==L::PingPong||l==L::Dub||l==L::Comb;}
bool motionLayout(L l){return l==L::Modulation||l==L::Phase||l==L::Pan||l==L::Ring;}
bool toneLayout(L l){return l==L::Console||l==L::Filter;}
}
float SuiteEditor::value(int i) const {
  if(i==0)if(auto* sync=processor.state.getRawParameterValue("tempo_sync"))if(sync->load()>.5f&&processor.effectivePrimary.load()>0)return processor.effectivePrimary.load();
  return static_cast<float>(knobs[i].getValue());
}
void SuiteEditor::applyPreset(int index){
  if(index<0||index>=3)return;
  for(auto* p:processor.getParameters())if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(p)){
    const auto id=parameter->paramID;
    if(!id.startsWith("control")&&id!="mix"&&id!="output"&&id!="bypass"){parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->getDefaultValue());parameter->endChangeGesture();}
  }
  auto set=[this](const juce::String& id,float v){auto* p=processor.state.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();};
  for(int i=0;i<processor.product.controlCount;++i)set("control"+juce::String(i),presets[index].values[i]);
  set("mix",presets[index].mix);set("output",0);
}
void SuiteEditor::place(int i,juce::Rectangle<float> r,bool fader){
  const float s=theme.scale;
  labels[i].setBounds(r.withHeight(22).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  knobs[i].setVisible(true);labels[i].setVisible(true);
  knobs[i].setSliderStyle(fader?juce::Slider::LinearVertical:juce::Slider::RotaryHorizontalVerticalDrag);
  knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow,false,juce::roundToInt(std::min(120.f,r.getWidth())*s),juce::roundToInt(29*s));
  knobs[i].getProperties().set("primary",r.getHeight()>210);
  knobs[i].setBounds(r.withTrimmedTop(24).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
}
void SuiteEditor::row(const std::vector<int>& ids,juce::Rectangle<float> r,bool fader){
  if(ids.empty())return;
  const float cell=r.getWidth()/ids.size();
  for(size_t j=0;j<ids.size();++j)place(ids[j],{r.getX()+j*cell+3,r.getY(),cell-6,r.getHeight()},fader);
}
void SuiteEditor::resized(){
  const float w=static_cast<float>(design.width),h=static_cast<float>(design.height);
  theme.scale=static_cast<float>(getWidth())/w;
  const float s=theme.scale;
  presetMenu.setBounds(juce::Rectangle<float>(w-280,22,244,30).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  bankA.setBounds(juce::Rectangle<float>(w-280,64,36,28).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  bankB.setBounds(juce::Rectangle<float>(w-240,64,36,28).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  copy.setBounds(juce::Rectangle<float>(w-196,64,65,28).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  bypassButton.setBounds(juce::Rectangle<float>(w-122,64,86,28).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  resetButton.setBounds(juce::Rectangle<float>(w-118,h-72,82,28).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  licenceButton.setBounds(juce::Rectangle<float>(36,h-33,230,23).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  if(advancedPanel){advancedButton.setBounds(juce::Rectangle<float>(w-395,65,100,26).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());advancedPanel->setBounds(juce::roundToInt((w-456)*s),juce::roundToInt(114*s),juce::roundToInt(420*s),advancedPanel->preferredHeight());}
  for(int i=0;i<8;++i){knobs[i].setVisible(false);labels[i].setVisible(false);}
  for(int i:{6,7}){
    const float x=w-470+(i-6)*168;
    labels[i].setBounds(juce::Rectangle<float>(x,h-82,150,18).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
    labels[i].setJustificationType(juce::Justification::left);
    knobs[i].setVisible(true);labels[i].setVisible(true);
    knobs[i].getProperties().set("compact",true);
    knobs[i].setSliderStyle(juce::Slider::LinearHorizontal);
    knobs[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,juce::roundToInt(76*s),juce::roundToInt(25*s));
    knobs[i].setBounds(juce::Rectangle<float>(x,h-62,150,26).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
  }
  display={};secondaryDisplay={};
  auto screen=[&](float x,float y,float ww,float hh){display=juce::Rectangle<float>(x,y,ww,hh).transformedBy(juce::AffineTransform::scale(s));};
  switch(design.layout){
  case L::Precision:
    screen(36,116,550,270);secondaryDisplay=juce::Rectangle<float>(610,116,w-646,270).transformedBy(juce::AffineTransform::scale(s));
    row({0,1,2,3,4,5},{36,410,w-72,h-510});break;
  case L::Rack:
    screen(395,125,270,220);place(0,{42,135,176,226});place(5,{838,135,176,226});
    row({1,2},{226,131,160,210});row({3,4},{678,131,150,210});break;
  case L::Optical:
    screen(294,122,312,205);place(0,{45,164,226,275});place(5,{629,164,226,275});
    row({1,2,3,4},{280,348,340,150});break;
  case L::Bus:
    screen(36,116,475,205);secondaryDisplay=juce::Rectangle<float>(535,116,w-571,205).transformedBy(juce::AffineTransform::scale(s));
    row({0,1,2,3,4,5},{36,346,w-72,153});break;
  case L::Parallel:
    screen(36,116,928,205);place(0,{36,346,220,196});
    row({1,2,3,4,5},{280,351,684,172});break;
  case L::Gate:
    screen(36,116,558,250);place(0,{634,117,254,257});place(4,{656,374,210,132});
    row({1,2,3,5},{36,391,558,119});break;
  case L::Transient:
    screen(36,116,w-72,166);place(0,{44,303,230,225});place(1,{w-274,303,230,225});
    row({2,3},{294,309,w-588,107});row({4,5},{294,430,w-588,107});break;
  case L::Peak:
    screen(36,116,252,412);place(0,{332,121,354,272});row({1,2},{324,403,w-360,125});break;
  case L::Rider:
    screen(36,116,438,292);place(0,{522,122,250,292},true);row({1,2,3,4},{36,428,w-72,113});break;
  case L::Console:
    screen(36,116,w-72,208);
    if(processor.product.kind==Kind::ParametricEQ){
      for(int j=0;j<3;++j){float x=42+j*320;place(j*2,{x,352,154,208});place(j*2+1,{x+160,352,142,208},true);}
    }else if(processor.product.kind==Kind::Cuts){
      place(0,{48,361,264,208});place(2,{390,374,260,172});place(1,{728,361,264,208});
    }else{place(0,{44,362,218,206});row({1,2},{286,363,468,203},true);place(3,{786,366,214,202});}
    break;
  case L::Filter:
    screen(36,116,w-72,208);
    if(processor.product.kind==Kind::TiltEQ){place(1,{340,335,220,165});place(0,{50,348,200,153});}
    else{place(0,{48,349,270,155});place(1,{352,347,246,159});if(processor.product.controlCount>2)place(2,{659,353,210,150});}
    break;
  case L::Drive:
    place(0,{30,152,300,312});screen(362,116,w-398,208);
    row(processor.product.controlCount==2?std::vector<int>{1}:std::vector<int>{1,2},{356,353,w-392,h-454});break;
  case L::Digital:
    screen(36,116,w-72,218);place(0,{48,359,290,166});
    row(processor.product.controlCount==2?std::vector<int>{1}:std::vector<int>{1,2},{382,360,w-420,163});break;
  case L::Echo: case L::Comb: case L::PingPong:
    screen(36,116,w-72,200);place(0,{36,343,244,h-442});
    row({1,2,3},{320,346,w-356,h-446});break;
  case L::Tape:
    screen(36,116,w-72,240);row({0,1,2,3},{46,385,w-92,164});break;
  case L::Dub:
    screen(36,116,570,230);place(1,{665,125,230,316},true);
    row({0,2,3},{42,371,567,196});break;
  case L::Modulation: case L::Phase:
    screen(36,116,548,280);place(0,{630,140,225,225});row({1,2,3},{45,422,w-90,119});break;
  case L::Pan: case L::Ring:
    screen(36,116,w-72,220);place(0,{45,362,285,151});
    row(processor.product.controlCount==2?std::vector<int>{1}:std::vector<int>{1,2},{392,361,w-438,153});break;
  case L::Stereo:
    screen(36,116,394,363);place(0,{474,118,314,220});place(1,{500,347,260,137});break;
  case L::Trim:
    screen(320,116,304,330);place(0,{42,130,128,302},true);place(1,{179,224,118,201});break;
  case L::Polarity:
    screen(36,116,w-72,115);
    for(int i=0;i<2;++i)polarityButtons[i].setBounds(juce::Rectangle<float>(44+i*310,268,284,91).transformedBy(juce::AffineTransform::scale(s)).toNearestInt());
    break;
  case L::Clean:
    screen(294,116,w-330,259);place(0,{37,133,234,241});break;
  }
}
void SuiteEditor::text(juce::Graphics& g,const juce::String& t,juce::Rectangle<float> r,float size,juce::Colour c,bool bold,juce::Justification align){
  g.setFont(juce::Font(juce::FontOptions("Segoe UI",size*theme.scale,bold?juce::Font::bold:juce::Font::plain)));
  g.setColour(juce::Colours::black.withAlpha(.65f));g.drawText(t,r.translated(0,theme.scale),align);
  g.setColour(c);g.drawText(t,r,align);
}
void SuiteEditor::panel(juce::Graphics& g,juce::Rectangle<float> r,const juce::String& name,bool screen){
  const float s=theme.scale;
  g.setColour(juce::Colours::black.withAlpha(.5f));g.fillRoundedRectangle(r.translated(0,3*s),5*s);
  if(screen)theme.paintDisplay(g,r);
  else{g.setGradientFill(juce::ColourGradient(design.metal.brighter(.12f),r.getX(),r.getY(),design.metal.darker(.35f),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,4*s);}
  g.setColour(design.accent.withAlpha(.25f));g.drawRoundedRectangle(r.reduced(s),4*s,s);
  if(!name.isEmpty())text(g,name,r.reduced(16*s).withHeight(20*s),10,design.accent,true);
}
void SuiteEditor::waveform(juce::Graphics& g,juce::Rectangle<float> r,bool compare){
  const float s=theme.scale;auto plot=r.reduced(20*s,32*s);
  g.setColour(GhostTheme::line());g.drawLine(plot.getX(),plot.getCentreY(),plot.getRight(),plot.getCentreY(),s);
  auto draw=[&](const auto& samples,juce::Colour c){
    juce::Path path;for(int i=0;i<512;++i){int index=i*8;float x=plot.getX()+plot.getWidth()*i/511.f;float y=plot.getCentreY()-juce::jlimit(-1.f,1.f,samples[index])*plot.getHeight()*.45f;
      if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
    g.setColour(c);g.strokePath(path,juce::PathStrokeType(1.4f*s));
  };
  if(compare)draw(waveformPre,GhostTheme::muted().withAlpha(.5f));draw(waveformPost,design.accent);
}
void SuiteEditor::meter(juce::Graphics& g,juce::Rectangle<float> r,float reading,const juce::String& name,bool reductionMeter){
  const float s=theme.scale;
  text(g,name,r.withHeight(22*s),11,GhostTheme::muted(),true,juce::Justification::centred);
  auto track=r.withTrimmedTop(35*s).withTrimmedBottom(38*s).withWidth(42*s).withX(r.getCentreX()-21*s);
  g.setColour(juce::Colours::black.withAlpha(.7f));g.fillRoundedRectangle(track,3*s);
  const float amount=reductionMeter?juce::jlimit(0.f,1.f,std::max(0.f,reading)/36.f):juce::jlimit(0.f,1.f,(reading+60)/60.f);
  for(int j=0;j<30;++j){auto segment=juce::Rectangle<float>(track.getX()+3*s,track.getBottom()-(j+1)*track.getHeight()/30,track.getWidth()-6*s,track.getHeight()/30-2*s);
    const auto lit=reductionMeter?design.accent:j>26?juce::Colour(0xffe8a07b):design.accent;
    g.setColour(lit.withAlpha(j/30.f<amount?.85f:.09f));g.fillRect(segment);}
  text(g,juce::String(reading,1)+" dB",r.withY(r.getBottom()-28*s).withHeight(26*s),14,GhostTheme::ink(),true,juce::Justification::centred);
}
void SuiteEditor::needle(juce::Graphics& g,juce::Rectangle<float> r,float reading,const juce::String& name,bool cream){
  const float s=theme.scale;auto face=r.reduced(10*s);
  g.setGradientFill(juce::ColourGradient(cream?juce::Colour(0xffe0d4ae):juce::Colour(0xff313b37),face.getX(),face.getY(),cream?juce::Colour(0xffb3a987):juce::Colour(0xff101915),face.getX(),face.getBottom(),false));
  g.fillRoundedRectangle(face,4*s);g.setColour(juce::Colours::black.withAlpha(.7f));g.drawRoundedRectangle(face,4*s,2*s);
  const auto ink=cream?juce::Colour(0xff32352c):GhostTheme::ink();
  const float cx=face.getCentreX(),cy=face.getBottom()-25*s,rad=std::min(face.getWidth()*.43f,face.getHeight()*.75f);
  for(int i=0;i<=12;++i){float a=-1.f+i/6.f;auto a0=juce::Point<float>(cx+std::sin(a)*rad,cy-std::cos(a)*rad),a1=juce::Point<float>(cx+std::sin(a)*(rad-10*s),cy-std::cos(a)*(rad-10*s));
    g.setColour(ink.withAlpha(.7f));g.drawLine({a0,a1},s);if(i%3==0)text(g,juce::String(i*3),{a0.x-14*s,a0.y-23*s,28*s,20*s},10,ink,false,juce::Justification::centred);}
  float angle=-1.f+2.f*juce::jlimit(0.f,1.f,std::max(0.f,reading)/36.f);
  g.setColour(cream?juce::Colour(0xff974a36):design.accent);g.drawLine(cx,cy,cx+std::sin(angle)*(rad-14*s),cy-std::cos(angle)*(rad-14*s),2.4f*s);
  g.setColour(ink);g.fillEllipse(cx-4*s,cy-4*s,8*s,8*s);
  text(g,name,face.withY(face.getY()+face.getHeight()*.53f).withHeight(22*s),12,ink,true,juce::Justification::centred);
  text(g,juce::String(reading,1)+" dB",face.withY(face.getBottom()-23*s).withHeight(20*s),12,ink,true,juce::Justification::centred);
}
void SuiteEditor::response(juce::Graphics& g){
  const float s=theme.scale;auto r=display.reduced(27*s,36*s);
  using C=juce::dsp::IIR::Coefficients<float>;
  std::vector<C::Ptr> filters,side;
  const double sr=processor.getSampleRate()>0?processor.getSampleRate():48000;
  auto frequency=[&](int i){return juce::jlimit(1.f,static_cast<float>(sr*.45),value(i));};
  auto gain=[](float d){return juce::Decibels::decibelsToGain(d);};
  auto q=[this](int i){auto* param=processor.state.getRawParameterValue("band_q"+juce::String(i));return param?param->load():.707f;};
  switch(processor.product.kind){
  case Kind::ParametricEQ:for(int j=0;j<3;++j)filters.push_back(C::makePeakFilter(sr,frequency(j*2),q(j),gain(value(j*2+1))));break;
  case Kind::TiltEQ:filters={C::makeLowShelf(sr,frequency(0),q(0),gain(-value(1))),C::makeHighShelf(sr,frequency(0),q(0),gain(value(1)))};break;
  case Kind::LowShelf:filters={C::makeLowShelf(sr,frequency(0),value(2),gain(value(1)))};break;
  case Kind::HighShelf:filters={C::makeHighShelf(sr,frequency(0),value(2),gain(value(1)))};break;
  case Kind::Notch:filters={C::makeNotch(sr,frequency(0),value(1))};break;
  case Kind::BandPass:filters={C::makeBandPass(sr,frequency(0),value(1))};break;
  case Kind::Cuts:filters={C::makeHighPass(sr,frequency(0),value(2)),C::makeLowPass(sr,std::min(static_cast<float>(sr*.45),std::max(value(1),value(0)*1.05f)),value(2))};break;
  case Kind::MidSideEQ:filters={C::makePeakFilter(sr,frequency(0),value(3),gain(value(1)))};side={C::makePeakFilter(sr,frequency(0),value(3),gain(value(2)))};break;
  default:break;
  }
  const double top=std::min(20000.,sr*.45);
  for(float y:{-24.f,-12.f,0.f,12.f,24.f}){
    float py=r.getCentreY()-y*r.getHeight()/48.f;g.setColour(GhostTheme::line().withAlpha(.65f));g.drawLine(r.getX(),py,r.getRight(),py,s);
    text(g,juce::String(y,0),{display.getX()+3*s,py-7*s,22*s,14*s},9,GhostTheme::muted());
  }
  for(float hz:{100.f,1000.f,10000.f})if(hz<top){float x=r.getX()+r.getWidth()*std::log(hz/20.f)/std::log(top/20.);g.setColour(GhostTheme::line().withAlpha(.65f));g.drawLine(x,r.getY(),x,r.getBottom(),s);}
  auto draw=[&](const std::vector<C::Ptr>& chain,juce::Colour colour){
    juce::Path path;for(int x=0;x<=400;++x){double hz=20*std::pow(top/20.,x/400.);double mag=1;for(auto& f:chain)mag*=f->getMagnitudeForFrequency(hz,sr);
      float y=r.getCentreY()-juce::jlimit(-24.f,24.f,db(static_cast<float>(mag)))*r.getHeight()/48.f;
      if(x==0)path.startNewSubPath(r.getX(),y);else path.lineTo(r.getX()+r.getWidth()*x/400.f,y);}
    g.setColour(colour.withAlpha(.12f));g.strokePath(path,juce::PathStrokeType(7*s));g.setColour(colour);g.strokePath(path,juce::PathStrokeType(2*s));
  };
  draw(filters,design.accent);if(!side.empty())draw(side,juce::Colour(0xffe8c29b));
  std::vector<int> handles=processor.product.kind==Kind::ParametricEQ?std::vector<int>{0,2,4}:processor.product.kind==Kind::Cuts?std::vector<int>{0,1}:std::vector<int>{0};
  for(auto i:handles){float x=r.getX()+r.getWidth()*std::log(frequency(i)/20.f)/std::log(top/20.);float d=0;
    if(processor.product.kind==Kind::ParametricEQ)d=value(i+1);
    else if(processor.product.kind==Kind::TiltEQ||processor.product.kind==Kind::LowShelf||processor.product.kind==Kind::HighShelf||processor.product.kind==Kind::MidSideEQ)d=value(1);
    float y=r.getCentreY()-d*r.getHeight()/48.f;
    g.setColour(design.metal);g.fillEllipse(x-5*s,y-5*s,10*s,10*s);g.setColour(design.accent);g.drawEllipse(x-5*s,y-5*s,10*s,10*s,1.5f*s);
  }
  text(g,"20 Hz",{r.getX(),display.getBottom()-24*s,90*s,16*s},10,GhostTheme::muted());
  text(g,"DRAG FREQUENCY / SHIFT: GAIN",{r.getCentreX()-135*s,display.getBottom()-24*s,270*s,16*s},9,GhostTheme::muted(),false,juce::Justification::centred);
  text(g,juce::String(top/1000,top==20000?0:1)+" kHz",{r.getRight()-70*s,display.getBottom()-24*s,70*s,16*s},10,GhostTheme::muted(),false,juce::Justification::right);
}
void SuiteEditor::timing(juce::Graphics& g,bool alternating){
  const float s=theme.scale;auto r=display.reduced(26*s,43*s);
  const float span=processor.product.kind==Kind::SlapDelay?500:processor.product.kind==Kind::Comb?100:4000;
  const float time=value(0),feedback=std::abs(value(1))*.01f;
  for(int j=0;j<=4;++j){float x=r.getX()+r.getWidth()*j/4;g.setColour(GhostTheme::line());g.drawLine(x,r.getY(),x,r.getBottom(),s);text(g,juce::String(span*j/4,0)+" ms",{x-30*s,display.getBottom()-24*s,60*s,16*s},9,GhostTheme::muted(),false,juce::Justification::centred);}
  for(int j=1;j<=32;++j){if(j*time>span)break;float strength=std::pow(feedback,static_cast<float>(j-1));if(strength<.005f)break;
    float x=r.getX()+r.getWidth()*j*time/span;float mid=alternating?(j%2?r.getY()+r.getHeight()*.25f:r.getY()+r.getHeight()*.75f):r.getCentreY();
    float length=r.getHeight()*(alternating?.2f:.4f)*strength;g.setColour(design.accent.withAlpha(.3f+.65f*strength));g.drawLine(x,mid-length,x,mid+length,3*s);
    g.fillEllipse(x-3*s,mid-length-3*s,6*s,6*s);
  }
  if(alternating){text(g,"L",{display.getX()+8*s,r.getY(),18*s,18*s},11,design.accent,true);text(g,"R",{display.getX()+8*s,r.getCentreY()+12*s,18*s,18*s},11,design.accent,true);}
  text(g,juce::String(time,time<100?1:0)+" ms / "+juce::String(value(1),0)+" % feedback",{r.getX(),display.getY()+28*s,r.getWidth(),18*s},12,GhostTheme::ink(),true,juce::Justification::right);
}
void SuiteEditor::motion(juce::Graphics& g){
  const float s=theme.scale;auto r=display.reduced(25*s,38*s);juce::Path path;
  for(int i=0;i<=400;++i){float angle=i/400.f*juce::MathConstants<float>::twoPi;float y=std::sin(angle),depth=value(1)*.01f;
    if(processor.product.kind==Kind::Tremolo||processor.product.kind==Kind::AutoPan){float shape=value(2)*.01f;y=y*(1-shape)+(y>=0?1.f:-1.f)*shape;}
    float px=r.getX()+r.getWidth()*i/400.f,py=r.getCentreY()-y*depth*r.getHeight()*.35f;if(i==0)path.startNewSubPath(px,py);else path.lineTo(px,py);
  }
  g.setColour(GhostTheme::line());g.drawLine(r.getX(),r.getCentreY(),r.getRight(),r.getCentreY(),s);
  g.setColour(design.accent.withAlpha(.13f));g.strokePath(path,juce::PathStrokeType(8*s));g.setColour(design.accent);g.strokePath(path,juce::PathStrokeType(2*s));
  const juce::String title=processor.product.kind==Kind::RingMod?"OSCILLATOR CYCLE":"ONE MODULATION CYCLE";
  text(g,title,{r.getX(),display.getBottom()-24*s,r.getWidth(),16*s},10,GhostTheme::muted(),false,juce::Justification::centred);
  text(g,juce::String(value(0),value(0)>=100?0:2)+" Hz",{r.getX(),display.getY()+28*s,r.getWidth(),22*s},15,GhostTheme::ink(),true,juce::Justification::right);
}
void SuiteEditor::stereo(juce::Graphics& g){
  const float s=theme.scale;auto r=display.reduced(35*s,46*s);const float rad=std::min(r.getWidth(),r.getHeight())*.45f;const auto c=r.getCentre();
  g.setColour(GhostTheme::line());for(int i=1;i<=3;++i)g.drawEllipse(c.x-rad*i/3,c.y-rad*i/3,rad*2*i/3,rad*2*i/3,s);
  g.drawLine(c.x-rad,c.y-rad,c.x+rad,c.y+rad,s);g.drawLine(c.x+rad,c.y-rad,c.x-rad,c.y+rad,s);
  double cross=0,left=0,right=0;for(int i=0;i<4096;i+=4){float l=stereoL[i],rr=stereoR[i];cross+=l*rr;left+=l*l;right+=rr*rr;
    float x=juce::jlimit(-1.f,1.f,(rr-l)*1.5f),y=juce::jlimit(-1.f,1.f,(rr+l)*1.5f);g.setColour(design.accent.withAlpha(.35f));g.fillEllipse(c.x+x*rad,c.y-y*rad,2*s,2*s);}
  const juce::String correlation=left*right>1e-12?juce::String(juce::jlimit(-1.,1.,cross/std::sqrt(left*right)),2):juce::String("--");
  text(g,"L",{c.x-rad-15*s,c.y-rad-20*s,24*s,20*s},11,GhostTheme::muted());text(g,"R",{c.x+rad-10*s,c.y-rad-20*s,24*s,20*s},11,GhostTheme::muted());
  text(g,"OUTPUT CORRELATION  "+correlation,{r.getX(),display.getBottom()-27*s,r.getWidth(),20*s},11,design.accent,true,juce::Justification::centred);
}
void SuiteEditor::paint(juce::Graphics& g){
  const float s=theme.scale,w=static_cast<float>(design.width),h=static_cast<float>(design.height);
  auto bounds=getLocalBounds().toFloat();theme.paintChassis(g,bounds);
  g.setGradientFill(juce::ColourGradient(design.metal.withAlpha(.84f),0,0,design.metal.darker(.55f).withAlpha(.74f),0,getHeight(),false));g.fillRect(bounds.reduced(18*s));
  // Fine metal striations stay subdued behind the screenprinted controls.
  for(float y=19*s;y<getHeight()-18*s;y+=3*s){g.setColour(juce::Colours::white.withAlpha(.018f));g.drawLine(19*s,y,getWidth()-19*s,y,.5f*s);}
  for(float x:{25.f,w-25})for(float y:{25.f,h-25}){g.setColour(juce::Colours::black.withAlpha(.7f));g.fillEllipse((x-4)*s,(y-4)*s,8*s,8*s);g.setColour(GhostTheme::muted().withAlpha(.35f));g.drawLine((x-2)*s,(y-1)*s,(x+2)*s,(y+1)*s,s);}
  auto r=[&](float x,float y,float ww,float hh){return juce::Rectangle<float>(x,y,ww,hh).transformedBy(juce::AffineTransform::scale(s));};
  text(g,"HUNGRY GHOST AUDIO / "+juce::String(processor.product.family).toUpperCase(),r(36,21,w-340,17),10,GhostTheme::muted(),true);
  text(g,processor.product.name,r(34,40,w-330,51),40,GhostTheme::ink(),true);
  text(g,design.instrument,r(36,92,w-72,16),10,design.accent,true);
  g.setColour(design.accent.withAlpha(.28f));g.drawLine(36*s,109*s,(w-36)*s,109*s,s);
  switch(design.layout){
  case L::Precision:{
    panel(g,display,"TRANSFER / THRESHOLD",true);panel(g,secondaryDisplay,"LIVE GAIN REDUCTION",true);
    auto plot=display.reduced(30*s,39*s);g.setColour(GhostTheme::line());for(int i=1;i<4;++i){g.drawLine(plot.getX(),plot.getY()+plot.getHeight()*i/4,plot.getRight(),plot.getY()+plot.getHeight()*i/4,s);g.drawLine(plot.getX()+plot.getWidth()*i/4,plot.getY(),plot.getX()+plot.getWidth()*i/4,plot.getBottom(),s);}
    juce::Path path;for(int j=0;j<=200;++j){float x=-60+j*.3f,over=x-value(0),y=x;
      if(processor.product.kind==Kind::Expander)y=x-std::min(value(4),std::max(0.f,-over)*(value(1)-1));
      else{const float knee=value(4);float reduction=over>knee*.5f?over*(1-1/value(1)):over>-knee*.5f&&knee>0?std::pow(over+knee*.5f,2.f)*(1-1/value(1))/(2*knee):0;y=x-reduction;}
      float px=plot.getX()+plot.getWidth()*j/200.f,py=plot.getBottom()-plot.getHeight()*juce::jlimit(0.f,1.f,(y+60)/60);if(j==0)path.startNewSubPath(px,py);else path.lineTo(px,py);}
    g.setColour(design.accent);g.strokePath(path,juce::PathStrokeType(2*s));
    float thresholdY=plot.getBottom()-(value(0)+60)/60*plot.getHeight();g.setColour(design.accent.withAlpha(.35f));g.drawLine(plot.getX(),thresholdY,plot.getRight(),thresholdY,s);
    text(g,"DRAG THRESHOLD / SHIFT: RATIO",r(66,display.getBottom()/s-25,450,18),9,GhostTheme::muted());
    auto history=secondaryDisplay.reduced(20*s,45*s);juce::Path hp;for(int i=0;i<200;++i){float x=history.getX()+history.getWidth()*i/199.f,y=history.getY()+juce::jlimit(0.f,36.f,reductionHistory[(historyWrite+i)%200])*history.getHeight()/36;if(i==0)hp.startNewSubPath(x,y);else hp.lineTo(x,y);}
    g.setColour(design.accent);g.strokePath(hp,juce::PathStrokeType(1.6f*s));text(g,juce::String(gr,1)+" dB",secondaryDisplay.reduced(20*s).withY(secondaryDisplay.getBottom()-34*s).withHeight(22*s),17,GhostTheme::ink(),true,juce::Justification::right);break;
  }
  case L::Rack:panel(g,display,"GAIN REDUCTION");needle(g,display.withTrimmedTop(25*s),gr,"PEAK DETECTOR");break;
  case L::Optical:panel(g,display,"");needle(g,display,gr,"RMS REDUCTION",true);break;
  case L::Bus:panel(g,display,"BUS LEVELS",true);meter(g,display.reduced(35*s,35*s).withWidth(170*s),db(peakIn),"INPUT");meter(g,display.reduced(35*s,35*s).withX(display.getRight()-205*s).withWidth(170*s),db(peakOut),"OUTPUT");panel(g,secondaryDisplay,"");needle(g,secondaryDisplay,gr,"LINKED REDUCTION");break;
  case L::Parallel:{panel(g,display,processor.product.kind==Kind::Ducker?"EXTERNAL KEY / PROGRAM OUTPUT":"DRY / COMPRESSED SIGNAL",true);waveform(g,display);text(g,"GR "+juce::String(gr,1)+" dB",display.reduced(16*s).withY(display.getBottom()-26*s).withHeight(18*s),12,design.accent,true,juce::Justification::right);break;}
  case L::Gate:
    panel(g,display,processor.product.kind==Kind::DeEsser?"DETECTOR FREQUENCY / INPUT - OUTPUT":"INPUT / GATED OUTPUT",true);
    if(processor.product.kind==Kind::DeEsser){drawSpectrum(g,spectrumPre,GhostTheme::muted().withAlpha(.55f));drawSpectrum(g,spectrumPost,design.accent);const float top=std::min(20000.,(processor.getSampleRate()>0?processor.getSampleRate():48000)*.45);float x=display.getX()+22*s+(display.getWidth()-44*s)*std::log(std::min(top,value(1))/20.f)/std::log(top/20.f);g.setColour(design.accent.withAlpha(.2f));g.fillRect(x-5*s,display.getY()+36*s,10*s,display.getHeight()-65*s);}
    else waveform(g,display);
    text(g,"REDUCTION "+juce::String(gr,1)+" dB",display.reduced(20*s).withY(display.getBottom()-26*s).withHeight(18*s),12,design.accent,true,juce::Justification::right);break;
  case L::Transient:panel(g,display,"TRANSIENT / OUTPUT WAVEFORM",true);waveform(g,display);break;
  case L::Peak:panel(g,display,"PEAK LEVELS",true);meter(g,display.reduced(17*s,36*s).withWidth(102*s),db(peakIn),"IN");meter(g,display.reduced(17*s,36*s).withX(display.getRight()-119*s).withWidth(102*s),db(peakOut),"OUT");break;
  case L::Rider:panel(g,display,"INPUT / LEVELLED OUTPUT",true);waveform(g,display);break;
  case L::Console:
    for(int i=0;i<3;++i){auto strip=r(36+i*323,345,306,227);panel(g,strip,processor.product.kind==Kind::ParametricEQ?(i==0?"LOW":i==1?"MID":"HIGH"):processor.product.kind==Kind::Cuts?(i==0?"LOW CUT":i==1?"RESONANCE":"HIGH CUT"):(i==0?"FREQUENCY":i==1?"MID / SIDE":"RESONANCE"));}
    panel(g,display,processor.product.kind==Kind::MidSideEQ?"FILTER RESPONSE / MID + SIDE":"FILTER RESPONSE / SETTINGS",true);response(g);break;
  case L::Filter:panel(g,display,"FILTER RESPONSE / SETTINGS",true);response(g);break;
  case L::Drive:case L::Digital:{
    panel(g,display,processor.product.kind==Kind::RateReducer?"SAMPLE-HOLD / SETTINGS":"STATIC SHAPING / BEFORE TONE + MIX",true);
    auto plot=display.reduced(25*s,35*s);g.setColour(GhostTheme::line());g.drawLine(plot.getX(),plot.getCentreY(),plot.getRight(),plot.getCentreY(),s);g.drawLine(plot.getCentreX(),plot.getY(),plot.getCentreX(),plot.getBottom(),s);
    const float drive=juce::Decibels::decibelsToGain(value(0));juce::Path path;
    for(int j=0;j<=400;++j){float x=-1+j*.005f,y=x;
      switch(processor.product.kind){
      case Kind::SoftSaturation:y=std::tanh(x*drive)/std::max(1.f,std::sqrt(drive));break;
      case Kind::AsymmetricSaturation:y=(std::tanh(x*drive+value(1))-std::tanh(value(1)))/std::max(1.f,std::sqrt(drive));break;
      case Kind::TubeSaturation:y=std::tanh(std::tanh(x*drive)*(1+2*value(1)*.01f))/std::max(1.f,std::sqrt(drive));break;
      case Kind::Wavefolder:y=2/juce::MathConstants<float>::pi*(std::asin(std::sin((x*drive+value(1))*juce::MathConstants<float>::halfPi))-std::asin(std::sin(value(1)*juce::MathConstants<float>::halfPi)));break;
      case Kind::Rectifier:y=x*(1-value(0)*.01f)+(std::abs(x+value(1))-value(1))*value(0)*.01f;break;
      case Kind::BitCrusher:{float steps=std::pow(2.f,std::round(value(0))-1);y=std::round(x*steps)/steps;break;}
      case Kind::RateReducer:{float rate=std::min(value(0),48000.f);float t=j/400.f*.002f;float held=std::floor(t*rate)/rate;y=std::sin(held*juce::MathConstants<float>::twoPi*1000);break;}
      default:break;}
      float px=plot.getX()+plot.getWidth()*j/400.f,py=plot.getCentreY()-juce::jlimit(-1.f,1.f,y)*plot.getHeight()*.45f;if(j==0)path.startNewSubPath(px,py);else path.lineTo(px,py);}
    g.setColour(design.accent.withAlpha(.12f));g.strokePath(path,juce::PathStrokeType(8*s));g.setColour(design.accent);g.strokePath(path,juce::PathStrokeType(1.8f*s));
    if(design.layout==L::Drive){text(g,"DRIVE STAGE",r(76,122,200,22),12,design.accent,true,juce::Justification::centred);for(int j=0;j<7;++j){g.setColour(design.accent.withAlpha(value(0)>j*5?.8f:.1f));g.fillRect(r(73+j*28,h-126,18,5));}}
    break;
  }
  case L::Echo:case L::PingPong:case L::Comb:case L::Dub:
    panel(g,display,design.layout==L::PingPong?"LEFT / RIGHT REPEAT TIMING":design.layout==L::Comb?"COMB PERIOD / SETTINGS":"REPEAT TIMING / SETTINGS",true);timing(g,design.layout==L::PingPong);break;
  case L::Tape:{
    panel(g,display,"FEEDBACK TAPE PATH");auto face=display.reduced(38*s,30*s);float diameter=198*s;
    for(int j=0;j<2;++j){float cx=j==0?face.getX()+diameter*.5f:face.getRight()-diameter*.5f;
      theme.paintReel(g,{cx-diameter*.5f,face.getCentreY()-diameter*.5f,diameter,diameter},reelFrame+j*5);}
    text(g,juce::String(value(0),0)+" ms",r(355,162,290,76),43,design.accent,true,juce::Justification::centred);text(g,"REPEAT INTERVAL",r(355,241,290,22),10,GhostTheme::muted(),true,juce::Justification::centred);break;
  }
  case L::Modulation:case L::Phase:case L::Pan:case L::Ring:
    panel(g,display,design.layout==L::Phase?"SIX ALL-PASS STAGES / MODULATION":design.layout==L::Pan?"MODULATION DEPTH + SHAPE":design.layout==L::Ring?"CARRIER OSCILLATOR":"MODULATION DEPTH",true);motion(g);break;
  case L::Stereo:panel(g,display,"LIVE OUTPUT IMAGE",true);stereo(g);break;
  case L::Trim:panel(g,display,"INPUT / OUTPUT LEVELS",true);meter(g,display.reduced(25*s,37*s).withWidth(104*s),db(peakIn),"IN");meter(g,display.reduced(25*s,37*s).withX(display.getRight()-129*s).withWidth(104*s),db(peakOut),"OUT");break;
  case L::Polarity:
    panel(g,display,"OUTPUT WAVEFORM / MONO SUM",true);waveform(g,display,false);
    for(int i=0;i<2;++i){panel(g,r(36+i*310,246,298,132),i==0?"LEFT CHANNEL":"RIGHT CHANNEL");}break;
  case L::Clean:panel(g,display,"INPUT / FILTERED OUTPUT",true);waveform(g,display);break;
  }
  g.setColour(juce::Colours::black.withAlpha(.24f));g.fillRect(r(19,h-98,w-38,80));g.setColour(GhostTheme::line());g.drawLine(36*s,(h-97)*s,(w-36)*s,(h-97)*s,s);
  text(g,"IN  "+juce::String(db(peakIn),1)+" dBFS",r(36,h-85,144,21),11,GhostTheme::muted());
  text(g,"OUT "+juce::String(db(peakOut),1)+" dBFS",r(36,h-62,144,21),11,GhostTheme::muted());
  text(g,juce::String((processor.getSampleRate()>0?processor.getSampleRate():48000)/1000.,1)+" kHz / "+juce::String(processor.getLatencySamples())+" samples latency",r(w-362,h-30,324,18),10,GhostTheme::muted(),false,juce::Justification::right);
}
void SuiteEditor::editGraph(const juce::MouseEvent& e){
  if(dragParameter<0)return;
  auto r=display.reduced(27*theme.scale,36*theme.scale);
  float x=juce::jlimit(0.f,1.f,(e.position.x-r.getX())/r.getWidth()),y=juce::jlimit(0.f,1.f,(r.getBottom()-e.position.y)/r.getHeight());
  auto* p=processor.state.getParameter("control"+juce::String(dragParameter));float v=p->convertFrom0to1(x);
  if(toneLayout(design.layout)){
    if(juce::String(processor.product.controls[dragParameter].unit)=="Hz")v=20*std::pow(std::min(20000.,(processor.getSampleRate()>0?processor.getSampleRate():48000)*.45)/20.,x);
    else v=(y-.5f)*48;
  }else if(design.layout==L::Precision)v=dragParameter==0?-60+y*60:1+(1-y)*19;
  else if(delayLayout(design.layout)){
    if(dragParameter==0)v=x*(processor.product.kind==Kind::SlapDelay?500:processor.product.kind==Kind::Comb?100:4000);
    else v=(y-.5f)*190;
  }
  p->setValueNotifyingHost(p->convertTo0to1(v));
}
void SuiteEditor::mouseDown(const juce::MouseEvent& e){
  if(!display.contains(e.position))return;
  if(toneLayout(design.layout)){
    const float sr=static_cast<float>(processor.getSampleRate()>0?processor.getSampleRate():48000);auto r=display.reduced(27*theme.scale,36*theme.scale);
    std::vector<int> ids=processor.product.kind==Kind::ParametricEQ?std::vector<int>{0,2,4}:processor.product.kind==Kind::Cuts?std::vector<int>{0,1}:std::vector<int>{0};
    dragParameter=ids[0];float nearest=1e9f;for(int i:ids){float px=r.getX()+r.getWidth()*std::log(juce::jlimit(20.f,sr*.45f,value(i))/20)/std::log(std::min(20000.f,sr*.45f)/20);float distance=std::abs(px-e.position.x);if(distance<nearest){nearest=distance;dragParameter=i;}}
    if(e.mods.isShiftDown()){
      if(processor.product.kind==Kind::ParametricEQ)++dragParameter;
      else if(processor.product.kind==Kind::TiltEQ||processor.product.kind==Kind::LowShelf||processor.product.kind==Kind::HighShelf||processor.product.kind==Kind::MidSideEQ)dragParameter=1;
    }
  }else if(design.layout==L::Precision)dragParameter=e.mods.isShiftDown()?1:0;
  else if(delayLayout(design.layout))dragParameter=e.mods.isShiftDown()?1:0;
  else if(design.layout==L::Drive||design.layout==L::Digital||motionLayout(design.layout))dragParameter=0;
  else return;
  if(dragParameter==0)if(auto* sync=processor.state.getRawParameterValue("tempo_sync"))if(sync->load()>.5f){dragParameter=-1;return;}
  processor.state.getParameter("control"+juce::String(dragParameter))->beginChangeGesture();editGraph(e);
}
void SuiteEditor::mouseDrag(const juce::MouseEvent& e){editGraph(e);}
void SuiteEditor::mouseUp(const juce::MouseEvent&){if(dragParameter>=0){processor.state.getParameter("control"+juce::String(dragParameter))->endChangeGesture();dragParameter=-1;}}
void SuiteEditor::timerCallback(){
  if(auto* sync=processor.state.getRawParameterValue("tempo_sync")){bool free=sync->load()<.5f;knobs[0].setEnabled(free);knobs[0].updateText();}
  peakIn=std::max(processor.inputPeak.load(),peakIn*.87f);peakOut=std::max(processor.outputPeak.load(),peakOut*.87f);gr=processor.reduction.load();
  reductionHistory[historyWrite++]=gr;historyWrite%=200;
  bankA.setToggleState(processor.selectedBank()==0,juce::dontSendNotification);bankB.setToggleState(processor.selectedBank()==1,juce::dontSendNotification);
  if(processor.product.kind==Kind::Polarity)for(int i=0;i<2;++i){bool inverted=value(i)>.5f;polarityButtons[i].setToggleState(inverted,juce::dontSendNotification);polarityButtons[i].setButtonText(inverted?"INVERTED  /  -":"NORMAL  /  +");}
  if(processor.popAnalysis(waveformPre,waveformPost,&stereoL,&stereoR)){
    fftPre.fill(0);fftPost.fill(0);for(int i=0;i<4096;++i){fftPre[i]=waveformPre[i]*window[i];fftPost[i]=waveformPost[i]*window[i];}
    fft.performFrequencyOnlyForwardTransform(fftPre.data());fft.performFrequencyOnlyForwardTransform(fftPost.data());
    for(int i=0;i<2048;++i){float a=db(fftPre[i]*4/4096.f),b=db(fftPost[i]*4/4096.f);spectrumPre[i]+=(a>spectrumPre[i]?.7f:.16f)*(a-spectrumPre[i]);spectrumPost[i]+=(b>spectrumPost[i]?.7f:.16f)*(b-spectrumPost[i]);}
    lastFrame=juce::Time::getMillisecondCounterHiRes();
  }else if(juce::Time::getMillisecondCounterHiRes()-lastFrame>300){for(auto& x:spectrumPre)x+=.1f*(-90-x);for(auto& x:spectrumPost)x+=.1f*(-90-x);for(auto& x:waveformPre)x*=.8f;for(auto& x:waveformPost)x*=.8f;for(auto& x:stereoL)x*=.8f;for(auto& x:stereoR)x*=.8f;}
  if(design.layout==L::Tape && peakIn>.0001f)reelFrame=(reelFrame+1)%24;
  repaint();
}
} // namespace hungryghost
