#include "SuiteEditor.h"
#include "SuiteProcessor.h"
#include "Originals/Reverb/PluginProcessor.h"
#include "Originals/Feral/PluginProcessor.h"
#include <iostream>
using namespace hungryghost;
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
    for(auto* child:editor->getChildren())if(auto* slider=dynamic_cast<juce::Slider*>(child))if(slider->isVisible()){
      ++initialSliders;
      if(slider->getWidth()<25||slider->getHeight()<25||!editor->getLocalBounds().contains(slider->getBounds())){std::cerr<<p.product.name<<" first-open slider missing\n";return 10;}
    }
    const int expectedSliders=p.product.controlCount+2-(p.product.kind==Kind::Polarity?2:0);
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
    for (float scale : { .75f, 1.f, 1.5f }) {
      editor->setSize(juce::roundToInt(original.getWidth()*scale),juce::roundToInt(original.getHeight()*scale));
      auto image=editor->createComponentSnapshot(editor->getLocalBounds());
      if (!image.isValid()) return 5;
      if (argc > 1 && scale == 1.f) {
        auto directory=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        auto stream=directory.getChildFile(index == 0 ? "reverb.png" : "feral.png").createOutputStream();
        if (!stream || !stream->setPosition(0) || stream->truncate().failed() || !juce::PNGImageFormat().writeImageToStream(image,*stream)) return 6;
      }
    }
    std::cout << p->getName() << " native editor at three sizes passed\n";
  }
  return 0;
}
