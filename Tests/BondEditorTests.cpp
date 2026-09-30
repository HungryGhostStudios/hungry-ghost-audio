#include "SuiteEditor.h"
#include <iostream>
#include <stdexcept>
using namespace hungryghost;
namespace {
void require(bool ok, const juce::String& message) {
  if (!ok) throw std::runtime_error(message.toStdString());
}
void set(SuiteProcessor& p, const juce::String& id, float value) {
  auto* param=p.state.getParameter(id);
  require(param!=nullptr,"Missing parameter: "+id);
  param->setValueNotifyingHost(param->convertTo0to1(value));
}
void collect(juce::Component& parent, std::vector<juce::Component*>& out) {
  for(auto* c:parent.getChildren()) if(c->isVisible()) {
    // Treat each control as one hit area, excluding its own editable label.
    if(dynamic_cast<juce::Slider*>(c)||dynamic_cast<juce::ComboBox*>(c)||dynamic_cast<juce::Button*>(c))out.push_back(c);
    else collect(*c,out);
  }
}
juce::Component* find(juce::Component& root,const juce::String& id) {
  std::vector<juce::Component*> controls;collect(root,controls);
  for(auto* c:controls)if(c->getComponentID()==id)return c;
  throw std::runtime_error("Missing visible control: "+id.toStdString());
}
void tick() {
  juce::Thread::sleep(45);
  juce::Timer::callPendingTimersSynchronously();
}
void saveImage(const juce::Image& image,const juce::File& directory,const char* filename) {
  require(image.isValid(),"Invalid native capture");
  auto stream=directory.getChildFile(filename).createOutputStream();
  require(stream&&stream->setPosition(0)&&stream->truncate().wasOk(),"Cannot write capture");
  require(juce::PNGImageFormat().writeImageToStream(image,*stream),"Cannot encode capture");
}
void save(juce::AudioProcessorEditor& editor,const juce::File& directory,const char* filename,float density=1) {
  saveImage(editor.createComponentSnapshot(editor.getLocalBounds(),true,density),directory,filename);
}
// Capture the actual native components in their supported paint states.
// This is a review sheet, not a second implementation of their drawing code.
void saveControlStates(juce::AudioProcessorEditor& editor,const juce::File& directory) {
  juce::Image sheet(juce::Image::RGB,1840,1120,true);
  {
  juce::Graphics g(sheet);g.addTransform(juce::AffineTransform::scale(2.f));
  g.fillAll(juce::Colour(0xff172024));
  auto label=[&](const juce::String& text,float x,float y,float width,float size=11.f){
    g.setColour(BondLookAndFeel::ink());g.setFont(BondLookAndFeel::font(size));
    g.drawText(text,juce::Rectangle<float>(x,y,width,22),juce::Justification::left);
  };
  auto component=[&](juce::Component& c,float x,float y){
    const auto native=c.createComponentSnapshot(c.getLocalBounds(),true,2);
    g.drawImage(native,juce::Rectangle<float>(x,y,float(c.getWidth()),float(c.getHeight())));
  };
  label("BOND / NATIVE CONTROL STATES",30,21,830,20);
  label("SWITCH",30,67,250,13);label("SELECTOR",326,67,250,13);label("STEREO FADER",628,67,262,13);
  auto& button=*dynamic_cast<juce::Button*>(find(editor,"bond_auto_release"));
  const bool on=button.getToggleState();
  const char* states[]={"Off","On","Hover","Pressed","Disabled"};
  for(int i=0;i<5;++i){
    button.setEnabled(i!=4);button.setToggleState(i>0,juce::dontSendNotification);
    button.setState(i==2?juce::Button::buttonOver:i==3?juce::Button::buttonDown:juce::Button::buttonNormal);
    const float y=105+i*82.f;label(states[i],30,y,250);component(button,30,y+26);
  }
  button.setEnabled(true);button.setToggleState(on,juce::dontSendNotification);button.setState(juce::Button::buttonNormal);
  auto& selector=*dynamic_cast<juce::ComboBox*>(find(editor,"bond_topology"));
  label("Current selection",326,105,250);component(selector,326,131);
  selector.setEnabled(false);label("Disabled",326,187,250);component(selector,326,213);selector.setEnabled(true);
  label("Open menu is captured separately",326,296,270,10);
  auto& fader=*dynamic_cast<juce::Slider*>(find(editor,"bond_link"));const double previous=fader.getValue();
  for(int i=0;i<3;++i){
    fader.setValue(i*50.,juce::sendNotificationSync);label(juce::String(i*50)+"%",628,105+i*101.f,262);
    component(fader,628,131+i*101.f);
  }
  fader.setValue(previous,juce::sendNotificationSync);fader.setEnabled(false);
  label("Disabled",628,408,262);component(fader,628,434);fader.setEnabled(true);
  }
  saveImage(sheet,directory,"BOND-Control-States.png");
}
}
int main(int argc,char** argv) {
  juce::ScopedJuceInitialiser_GUI init;
  try {
    int index=0;for(int i=2;i<50;++i)if(products[i].kind==Kind::BusCompressor)index=i;
    require(index>1,"BOND missing");
    SuiteProcessor p(index);p.prepareToPlay(48000,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    // JUCE exposes native accessibility handlers only after a window handle
    // exists. Keep this test peer hidden; captures do not need a visible window.
    editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
    const int width=editor->getWidth(),height=editor->getHeight();
    std::vector<juce::Component*> controls;collect(*editor,controls);
    int sliders=0;
    for(auto* c:controls) {
      const auto id=c->getComponentID();auto* parameter=p.state.getParameter(id);
      if(auto* slider=dynamic_cast<juce::Slider*>(c)) {
        ++sliders;require(parameter!=nullptr,"Unbound slider: "+id);
        auto* accessible=slider->getAccessibilityHandler();
        require(slider->getWantsKeyboardFocus()&&accessible&&accessible->getRole()==juce::AccessibilityRole::slider&&accessible->getTitle().isNotEmpty(),"Slider missing keyboard access or accessible title: "+id);
        float value=parameter->convertFrom0to1(.63f);
        slider->setValue(value,juce::sendNotificationSync);
        require(std::abs(parameter->convertFrom0to1(parameter->getValue())-value)<std::max(.02f,std::abs(value)*.001f),"UI change lost: "+id);
        value=parameter->convertFrom0to1(.37f);set(p,id,value);tick();
        require(std::abs(slider->getValue()-value)<std::max(.02f,std::abs(value)*.001f),"Host automation lost: "+id);
        require(slider->getTextBoxPosition()==juce::Slider::TextBoxBelow&&slider->isTextBoxEditable(),"Value field not editable: "+id);
        const float beforeKey=parameter->getValue();
        require(slider->keyPressed(juce::KeyPress(juce::KeyPress::rightKey)),"Slider ignored arrow key: "+id);
        require(parameter->getValue()>beforeKey,"Keyboard slider change lost: "+id);
      }
    }
    require(sliders==12,"Expected twelve visible BOND sliders");
    for(const char* id:{"bond_model","bond_topology","bond_detector"}) {
      auto* box=dynamic_cast<juce::ComboBox*>(find(*editor,id));require(box!=nullptr,"Missing choice");
      auto* accessible=box->getAccessibilityHandler();
      require(accessible&&accessible->getRole()==juce::AccessibilityRole::comboBox&&accessible->getTitle().isNotEmpty(),"Choice missing accessible title");
      box->setSelectedId(2,juce::sendNotificationSync);tick();
      require(p.state.getRawParameterValue(id)->load()==1,"Choice did not reach processor");
      set(p,id,0);tick();require(box->getSelectedId()==1,"Choice automation did not reach UI");
    }
    for(const char* id:{"bond_auto_release","key_listen"}) {
      auto* button=dynamic_cast<juce::Button*>(find(*editor,id));require(button!=nullptr,"Missing toggle");
      button->setToggleState(true,juce::sendNotificationSync);tick();
      require(p.state.getRawParameterValue(id)->load()==1,"Toggle did not reach processor");
      set(p,id,0);tick();require(!button->getToggleState(),"Toggle automation did not reach UI");
    }
    set(p,"bond_model",0);tick();
    for(const char* id:{"bond_knee","bond_range","bond_key_lowpass","bond_link","bond_topology","bond_detector","bond_auto_release"})require(!find(*editor,id)->isEnabled(),"Original mode enabled unused control");
    for(const char* id:{"control0","control4","key_listen"})require(find(*editor,id)->isEnabled(),"Original mode disabled original control");
    set(p,"bond_model",1);tick();
    for(const char* id:{"bond_knee","bond_range","bond_key_lowpass","bond_link","bond_topology","bond_detector","bond_auto_release"})require(find(*editor,id)->isEnabled(),"Precision mode did not enable control");
    auto* presets=dynamic_cast<juce::ComboBox*>(find(*editor,"preset"));
    require(presets!=nullptr,"Missing preset menu");
    constexpr float presetTopology[]={1,0,0},presetDetector[]={1,0,1},presetLink[]={100,75,100},
        presetKnee[]={6,4,9},presetRange[]={6,9,18},presetLP[]={16000,20000,12000},presetAuto[]={1,0,1};
    for(int i=0;i<3;++i){
      presets->setSelectedId(i+1,juce::sendNotificationSync);tick();
      const std::pair<const char*,float> expected[]={{"bond_model",1},{"bond_topology",presetTopology[i]},
        {"bond_detector",presetDetector[i]},{"bond_link",presetLink[i]},{"bond_knee",presetKnee[i]},
        {"bond_range",presetRange[i]},{"bond_key_lowpass",presetLP[i]},{"bond_auto_release",presetAuto[i]},{"key_listen",0}};
      for(const auto& item:expected)require(std::abs(p.state.getRawParameterValue(item.first)->load()-item.second)<.01f,"Preset advanced setting missing");
      set(p,"bond_link",53);tick();require(presets->getSelectedId()==0,"Edited preset still appears selected");
      presets->setSelectedId(i+1,juce::sendNotificationSync);tick();
      require(p.state.getRawParameterValue("bond_link")->load()==presetLink[i],"Same preset did not reapply after edit");
    }
    p.selectBank(1);tick();require(presets->getSelectedId()==0,"A/B switch retained stale preset selection");
    presets->setSelectedId(3,juce::sendNotificationSync);tick();
    require(p.state.getRawParameterValue("bond_range")->load()==18,"Preset did not reapply after bank switch");
    p.selectBank(0);p.factoryReset();
    set(p,"control0",-24);set(p,"control1",4);set(p,"control2",20);set(p,"control3",180);
    set(p,"bond_detector",1);set(p,"bond_link",70);set(p,"bond_knee",6);set(p,"bond_range",12);set(p,"bond_auto_release",1);
    juce::AudioBuffer<float> buffer(2,512);juce::MidiBuffer midi;
    const auto processSignal=[&]{for(int block=0;block<128;++block) {
      for(int i=0;i<512;++i) {
        const auto t=(block*512+i)/48000.;
        buffer.setSample(0,i,float(.42*std::sin(t*2*juce::MathConstants<double>::pi*220)+.12*std::sin(t*2*juce::MathConstants<double>::pi*1730)));
        buffer.setSample(1,i,float(.22*std::sin(t*2*juce::MathConstants<double>::pi*330)));
      }
      p.processBlock(buffer,midi);
    }};
    processSignal();
    tick();
    if(argc>1)require(p.licence.canProcess()&&p.reductionLeft.load()>1&&p.reductionRight.load()>1,"Capture needs active processing and actual GR");
    juce::File directory=argc>1?juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]):juce::File();
    if(argc>1)require(directory.createDirectory().wasOk(),"Cannot create capture directory");
    for(float scale:{.85f,1.f,1.5f}) {
      editor->setSize(juce::roundToInt(width*scale),juce::roundToInt(height*scale));tick();
      controls.clear();collect(*editor,controls);
      std::vector<juce::Rectangle<int>> areas;
      for(auto* c:controls) {
        auto bounds=editor->getLocalArea(c,c->getLocalBounds());
        require(editor->getLocalBounds().contains(bounds)&&bounds.getWidth()>=20&&bounds.getHeight()>=18,"Clipped control: "+c->getComponentID());
        for(const auto& other:areas)require(!bounds.intersects(other),"Overlapping control: "+c->getComponentID());
        areas.push_back(bounds);
      }
      if(argc>1)save(*editor,directory,scale<1?"BOND-85.png":scale==1?"BOND-100.png":"BOND-150.png");
    }
    editor->setSize(width,height);tick();
    if(argc>1) {
      save(*editor,directory,"BOND-Retina.png",2);
      set(p,"bond_model",0);processSignal();
      for(int i=0;i<12;++i)tick();
      require(std::abs(p.reductionLeft.load()-p.reductionRight.load())<.001f,"Original capture must reflect linked processing");
      save(*editor,directory,"BOND-Original.png");
      set(p,"bond_model",1);tick();saveControlStates(*editor,directory);
      // Popup menus are separate native components; capture the actual menu.
      presets->setSelectedId(1,juce::sendNotificationSync);tick();
      presets->showPopup();
      if(auto* popup=juce::Component::getCurrentlyModalComponent()){
        auto* accessible=popup->getAccessibilityHandler();
        require(accessible&&accessible->getRole()==juce::AccessibilityRole::popupMenu,"Expected native popup menu");
        popup->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
        saveImage(popup->createComponentSnapshot(popup->getLocalBounds(),true,2),directory,"BOND-Preset-Menu.png");
      }else require(false,"Native popup failed to open for capture");
      presets->hidePopup();
    }
    std::cout<<"BOND native controls, host automation, mode availability and three-size layout passed\n";
    return 0;
  } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
