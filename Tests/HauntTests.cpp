#include "Haunt/HauntEditor.h"
#include <iostream>
#include <stdexcept>
using namespace hungryghost::haunt;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void save(juce::AudioProcessorEditor& editor,const juce::File& directory,const char* name){auto stream=directory.getChildFile(name).createOutputStream();require(stream&&stream->setPosition(0)&&stream->truncate().wasOk(),"Capture stream failed");require(juce::PNGImageFormat().writeImageToStream(editor.createComponentSnapshot(editor.getLocalBounds(),true,1.5f),*stream),"Capture failed");}
}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;try {
    HauntProcessor p;p.prepareToPlay(48000,512);require(p.getLatencySamples()>0&&p.acceptsMidi(),"Host latency/MIDI missing");
    p.setValue("retune",0);p.setValue("vibrato",0);p.setValue("humanize",0);p.copyBank();p.selectBank(1);p.setValue("retune",180);p.selectBank(0);require(p.value("retune")==0,"A/B restoration failed");
    juce::MemoryBlock saved;p.getStateInformation(saved);HauntProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));require(restored.value("retune")==0,"Active bank restore failed");restored.selectBank(1);require(restored.value("retune")==180,"Inactive bank restore failed");
    HauntEditor editor(p);editor.addToDesktop(juce::ComponentPeer::windowIsTemporary);
    const auto directory=argc>1?juce::File(juce::String(argv[1])):juce::File();if(argc>1){directory.createDirectory();save(editor,directory,"HAUNT-Idle.png");}
    juce::AudioBuffer<float> block(2,512);juce::MidiBuffer midi;double phase=0;
    for(int n=0;n<430;++n){for(int i=0;i<512;++i){const double t=(n*512+i)/48000.;const double note=64+2*std::floor(t*.8)+.27+.14*std::sin(2*juce::MathConstants<double>::pi*5.2*t);const double hz=440*std::pow(2.,(note-69)/12);phase+=2*juce::MathConstants<double>::pi*hz/48000.;const float sample=float(.24*std::sin(phase)+.12*std::sin(2*phase)+.08*std::sin(3*phase));block.setSample(0,i,sample);block.setSample(1,i,sample);}
        p.processBlock(block,midi);editor.poll();}
    require(p.reading().voiced,"Processor did not track synthetic vocal");
    if(argc>1){save(editor,directory,"HAUNT-Live.png");editor.setSize(880,592);save(editor,directory,"HAUNT-Small.png");editor.setSize(1650,1110);save(editor,directory,"HAUNT-Large.png");}
    int sliders=0,boxes=0,buttons=0;
    std::function<void(juce::Component&)> check=[&](juce::Component& parent){for(auto* c:parent.getChildren()){
        if(auto* slider=dynamic_cast<juce::Slider*>(c)){++sliders;const auto id=slider->getComponentID();auto* parameter=p.state.getParameter(id);require(parameter!=nullptr,"Slider not bound to parameter");const float value=parameter->convertFrom0to1(.37f);slider->setValue(value,juce::sendNotificationSync);require(std::abs(p.value(id)-value)<.02f,"Slider did not update audio parameter");require(slider->getWantsKeyboardFocus(),"Slider not keyboard accessible");}
        else if(auto* box=dynamic_cast<juce::ComboBox*>(c)) {
            ++boxes;const auto id=box->getComponentID();
            if(id.isNotEmpty()){box->setSelectedId(2,juce::sendNotificationSync);require(p.value(id)==1,"Dropdown did not update its audio parameter");}
        }
        else if(auto* button=dynamic_cast<juce::TextButton*>(c)) {
            ++buttons;const auto id=button->getComponentID();
            if(id=="note0") {p.setValue("scale",0);button->onClick();require(p.value("scale")==4&&p.value("note0")==0&&p.value("note1")==1,"Custom keyboard did not edit the scale");}
            else if(id=="midi"||id=="preserve"||id=="bypass") {button->setToggleState(!button->getToggleState(),juce::sendNotificationSync);require((p.value(id)>.5f)==button->getToggleState(),"Toggle did not update its audio parameter");}
        }
        else check(*c);
    }};check(editor);require(sliders==9&&boxes==4&&buttons==18,"Controls missing on first open");
    p.setValue("bypass",0);p.setValue("midi",1);p.setValue("transpose",0);p.setValue("range",0);p.setValue("reference",440);
    p.prepareToPlay(48000,512);phase=0;
    for(int n=0;n<100;++n) {
        for(int i=0;i<512;++i){phase+=2*juce::MathConstants<double>::pi*440/48000.;const float value=float(.2*std::sin(phase));block.setSample(0,i,value);block.setSample(1,i,value);}
        if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,72,juce::uint8(100)),127);
        p.processBlock(block,midi);
    }
    require(p.reading().target==72,"Processor MIDI did not reach the live engine");
    midi.addEvent(juce::MidiMessage::noteOff(1,72),127);p.processBlock(block,midi);require(!p.reading().hasTarget,"Processor retained a released MIDI note");
    p.setValue("output",-12);p.setValue("bypass",1);p.prepareToPlay(48000,512);
    juce::AudioBuffer<float> impulse(2,p.getLatencySamples()+512);impulse.clear();impulse.setSample(0,0,.5f);impulse.setSample(1,0,.5f);p.processBlockBypassed(impulse,midi);
    require(std::abs(impulse.getSample(0,p.getLatencySamples())-.5f)<1e-6f,"Host bypass changed output gain or latency");
    std::cout<<"PASS: state and A/B recall, real-time processor, native controls and three editor sizes; latency "<<p.getLatencySamples()<<" samples\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
