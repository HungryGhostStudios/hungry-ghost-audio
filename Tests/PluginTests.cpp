#include "SuiteEditor.h"
#include "SuiteProcessor.h"
#include "Originals/Reverb/PluginProcessor.h"
#include "Originals/Feral/PluginProcessor.h"
#include "Originals/Feral/PluginEditor.h"
#include <iostream>
using namespace hungryghost;
namespace {
void primeNativeCapture(juce::AudioProcessor& processor) {
  // Capture-only multitone input drives the actual meters, spectrum and DSP.
  // Two bounded timer updates consume the analysis frames without fake traces
  // or changes to the selected preset. Normal ctest runs never wait here.
  juce::AudioBuffer<float> signal(juce::jmax(processor.getTotalNumInputChannels(),
                                           processor.getTotalNumOutputChannels()),4096);
  juce::MidiBuffer midi;
  int sample=0;
  for(int tick=0;tick<2;++tick) {
    for(int block=0;block<2;++block) {
      for(int i=0;i<signal.getNumSamples();++i,++sample) {
        const double t=sample/48000.;
        const double phase=juce::MathConstants<double>::twoPi*t;
        const float left=float(.14*std::sin(phase*83)+.12*std::sin(phase*337)
                               +.08*std::sin(phase*2039)+.045*std::sin(phase*8101));
        const float right=float(.11*std::sin(phase*83)+.105*std::sin(phase*421)
                                +.065*std::sin(phase*2039)+.035*std::sin(phase*6947));
        for(int channel=0;channel<signal.getNumChannels();++channel)
          signal.setSample(channel,i,channel%2==0?left:right);
      }
      processor.processBlock(signal,midi);
    }
    juce::Thread::sleep(45);
    juce::Timer::callPendingTimersSynchronously();
  }
}
}
int main(int argc, char** argv) {
  juce::ScopedJuceInitialiser_GUI init;
  for (int index = 2; index < 50; ++index) {
    SuiteProcessor p(index);
    p.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> buffer(2, 8193);
    juce::MidiBuffer midi;
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 8193; ++i)
        buffer.setSample(c, i, .1f * std::sin(i * .09f));
    p.processBlock(buffer, midi);
    auto *parameter = p.state.getParameter("control0");
    parameter->setValueNotifyingHost(.75f);
    const float savedValue = parameter->getValue();
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    parameter->setValueNotifyingHost(.1f);
    p.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    if (std::abs(parameter->getValue() - savedValue) > .0001f)
      return 1;
    p.selectBank(1);
    p.selectBank(0);
    if (std::abs(parameter->getValue() - savedValue) > .0001f)
      return 2;
    p.factoryReset();
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    const auto baseSize=editor->getBounds();
    // A same-size setSize does not trigger resized(). Check the first host-open view.
    int initialSliders=0;
    bool invalidSlider=false;
    std::function<void(juce::Component&)> inspectSliders=[&](juce::Component& parent){
      for(auto* child:parent.getChildren())if(child->isVisible()){
        if(auto* slider=dynamic_cast<juce::Slider*>(child)){
          ++initialSliders;
          if(slider->getWidth()<25||slider->getHeight()<25||!editor->getLocalBounds().contains(editor->getLocalArea(slider,slider->getLocalBounds())))invalidSlider=true;
        }else inspectSliders(*child);
      }
    };
    inspectSliders(*editor);
    if(invalidSlider){std::cerr<<p.product.name<<" first-open slider missing\n";return 10;}
    const int expectedSliders=p.product.kind==Kind::BusCompressor?12:p.product.controlCount+2-(p.product.kind==Kind::Polarity?2:0);
    if(initialSliders!=expectedSliders){std::cerr<<p.product.name<<" first-open slider count\n";return 11;}
    for(auto* child:editor->getChildren())if(auto* panel=dynamic_cast<AdvancedPanel*>(child)){
      panel->setVisible(true);
      for(auto* c:panel->getChildren()){
        if(!panel->getLocalBounds().contains(c->getBounds())){std::cerr<<p.product.name<<" advanced control out of bounds\n";return 12;}
        if(auto* slider=dynamic_cast<juce::Slider*>(c)){
          auto* param=p.state.getParameter(slider->getComponentID());
          const float target=param->convertFrom0to1(.65f);
          slider->setValue(target,juce::sendNotificationSync);
          if(std::abs(param->convertFrom0to1(param->getValue())-target)>std::max(.01f,target*.001f))return 13;
          const auto id=slider->getComponentID();
          const juce::String suffix=id=="fallback_bpm"?" BPM":id.startsWith("band_q")?" Q":" Hz";
          if(!slider->getTextFromValue(slider->getValue()).endsWith(suffix)){std::cerr<<p.product.name<<" advanced unit missing\n";return 14;}
        }
      }
      if(argc>1&&juce::String(p.product.id)=="reel"){auto dir=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);auto out=dir.getChildFile("reel-shape.png").createOutputStream();if(out&&out->setPosition(0)&&out->truncate().wasOk())juce::PNGImageFormat().writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds()),*out);}
      panel->setVisible(false);
      p.factoryReset();
    }
    const auto presets=presetsFor(p.product);
    for(auto* child:editor->getChildren())if(auto* menu=dynamic_cast<juce::ComboBox*>(child)){
      if(menu->getComponentID()!="preset")continue;
      for(int preset=0;preset<3;++preset){
        menu->setSelectedId(preset+1,juce::sendNotificationSync);
        for(int c=0;c<p.product.controlCount;++c){auto* param=p.state.getParameter("control"+juce::String(c));if(std::abs(param->convertFrom0to1(param->getValue())-presets[preset].values[c])>std::max(.001f,std::abs(presets[preset].values[c])*.00001f))return 7;}
      }
      menu->setSelectedId(1,juce::sendNotificationSync);
    }
    for (float scale : {.85f,1.f,1.5f}) {
      editor->setSize(juce::roundToInt(baseSize.getWidth()*scale),juce::roundToInt(baseSize.getHeight()*scale));
      std::vector<juce::Component*> controls;
      for(auto* child:editor->getChildren())if(child->isVisible()){
        if(!editor->getLocalBounds().contains(child->getBounds())||child->getHeight()<12){std::cerr<<p.product.name<<" control out of bounds\n";return 8;}
        if(dynamic_cast<juce::Slider*>(child)||dynamic_cast<juce::Button*>(child)||dynamic_cast<juce::ComboBox*>(child))controls.push_back(child);
      }
      for(size_t a=0;a<controls.size();++a)for(size_t b=a+1;b<controls.size();++b)
        if(controls[a]->getBounds().intersects(controls[b]->getBounds())){std::cerr<<p.product.name<<" overlapping controls "<<controls[a]->getComponentID()<<" / "<<controls[b]->getComponentID()<<"\n";return 9;}
      if(argc>1&&scale==1.f)primeNativeCapture(p);
      auto image = editor->createComponentSnapshot(editor->getLocalBounds());
      if (!image.isValid())
        return 3;
      if (argc > 1 && scale == 1.f) {
        juce::File directory = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        directory.createDirectory();
        auto stream = directory.getChildFile(juce::String(p.product.id) + ".png").createOutputStream();
        if (!stream || !stream->setPosition(0) || stream->truncate().failed() || !juce::PNGImageFormat().writeImageToStream(image, *stream))
          return 4;
      }
    }
    std::cout << p.product.name
              << " state/A-B, large block and native editor passed\n";
  }
  for (int index = 0; index < 2; ++index) {
    std::unique_ptr<juce::AudioProcessor> p;
    if (index == 0) p = std::make_unique<AfterProcessor>();
    else p = std::make_unique<FeralProcessor>();
    p->prepareToPlay(48000,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditorIfNeeded());
    const auto original = editor->getBounds();
    if(argc>1)primeNativeCapture(*p);
    for (float scale : { .75f, 1.f, 1.5f }) {
      editor->setSize(juce::roundToInt(original.getWidth()*scale),juce::roundToInt(original.getHeight()*scale));
      auto image=editor->createComponentSnapshot(editor->getLocalBounds());
      if (!image.isValid()) return 5;
      if (argc > 1 && (scale == 1.f || scale == .75f)) {
        auto directory=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        const auto filename=juce::String(index == 0 ? "reverb" : "feral")
                            +(scale == .75f ? "-75.png" : ".png");
        auto stream=directory.getChildFile(filename).createOutputStream();
        if (!stream || !stream->setPosition(0) || stream->truncate().failed() || !juce::PNGImageFormat().writeImageToStream(image,*stream)) return 6;
      }
    }
    if (argc > 1) {
      // Exercise the actual native view callbacks, without a host window or a
      // message-loop delay. Default ctest runs do not open these extra views.
      editor->setSize(original.getWidth(),original.getHeight());
      const juce::String caption=index == 0 ? "Shape +" : "Bus";
      bool activated=false;
      for(auto* child:editor->getChildren())if(auto* button=dynamic_cast<juce::TextButton*>(child)) {
        if(button->getButtonText()==caption&&button->isEnabled()&&button->onClick) {
          button->onClick();activated=true;break;
        }
      }
      if(!activated){std::cerr<<p->getName()<<" alternate view callback missing\n";return 15;}
      if(index == 1) {
        auto* feral=dynamic_cast<FeralEditor*>(editor.get());
        if(!feral)return 15;
        feral->refresh();
      }
      primeNativeCapture(*p);
      auto image=editor->createComponentSnapshot(editor->getLocalBounds());
      if(!image.isValid())return 5;
      auto directory=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
      auto stream=directory.getChildFile(index == 0 ? "reverb-shape.png" : "feral-bus.png").createOutputStream();
      if(!stream||!stream->setPosition(0)||stream->truncate().failed()||!juce::PNGImageFormat().writeImageToStream(image,*stream))return 6;
    }
    std::cout << p->getName() << " native editor at three sizes passed\n";
  }
  return 0;
}
