#include "HauntProcessor.h"
#include "HauntEditor.h"
namespace hungryghost::haunt {
juce::AudioProcessorValueTreeState::ParameterLayout HauntProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto number=[&](const char* id,const char* name,float lo,float hi,float def,float skew=1) {
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{lo,hi,.01f,skew},def));
    };
    number("retune","Retune",0,250,35,.5f); number("amount","Correction",0,100,100);
    number("humanize","Humanize",0,100,35); number("vibrato","Vibrato preservation",0,100,50);
    number("formant","Formant shift",-12,12,0);number("transpose","Transpose",-12,12,0);
    number("reference","Reference frequency",420,460,440);number("mix","Mix",0,100,100);number("output","Output",-24,12,0);
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"key",1},"Key",juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"scale",1},"Scale",juce::StringArray{"Chromatic","Major","Minor","Major pentatonic","Custom"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"range",1},"Vocal range",juce::StringArray{"Wide / 65-1100 Hz","Low / 65-350 Hz","Mid / 100-700 Hz","High / 160-1100 Hz"},0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"preserve",1},"Preserve formants",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"midi",1},"MIDI targeting",false));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
    for(int i=0;i<12;++i)p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"note"+juce::String(i),1},"Allow note "+juce::String(i),true));
    return p;
}
HauntProcessor::HauntProcessor():AudioProcessor(BusesProperties().withInput("Vocal",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"HAUNT",layout()) {
    const char* ids[]={"retune","amount","humanize","vibrato","formant","transpose","reference","mix","output","key","scale","range","preserve","midi","bypass"};
    for(int i=0;i<15;++i)parameters[i]=state.getRawParameterValue(ids[i]);
    for(int i=0;i<12;++i)parameters[15+i]=state.getRawParameterValue("note"+juce::String(i));
    banks[0]=state.copyState();banks[1]=banks[0].createCopy();
}
float HauntProcessor::value(const juce::String& id) const {return state.getRawParameterValue(id)->load();}
void HauntProcessor::setValue(const juce::String& id,float v) {if(auto* p=state.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();}}
Settings HauntProcessor::settings() const {
    auto v=[&](int i){return parameters[i]->load(std::memory_order_relaxed);};
    Settings s;s.retuneMs=v(0);s.amount=v(1)*.01f;s.humanize=v(2)*.01f;s.vibrato=v(3)*.01f;
    s.formant=v(4);s.transpose=v(5);s.reference=v(6);s.mix=v(7)*.01f;s.outputDb=v(8);
    s.key=int(v(9));s.scale=int(v(10));s.range=int(v(11));s.preserveFormants=v(12)>.5f;s.midiTarget=v(13)>.5f;s.bypass=v(14)>.5f;
    s.customMask=0;for(int i=0;i<12;++i)if(v(15+i)>.5f)s.customMask|=1u<<i;
    return s;
}
bool HauntProcessor::isBusesLayoutSupported(const BusesLayout& b) const {const auto c=b.getMainInputChannelSet();return (c==juce::AudioChannelSet::mono()||c==juce::AudioChannelSet::stereo())&&c==b.getMainOutputChannelSet();}
void HauntProcessor::prepareToPlay(double sr,int) {engine.setSettings(settings());engine.prepare(sr,getTotalNumInputChannels());setLatencySamples(engine.latencySamples());}
void HauntProcessor::run(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi,bool bypassed) {
    juce::ScopedNoDenormals noDenormals;
    auto s=settings();s.bypass|=bypassed;engine.setSettings(s);
    const int count=std::min(2,audio.getNumChannels());
    auto process=[&](int start,int end){if(end<=start||count==0)return;float* channels[2]={audio.getWritePointer(0,start),count>1?audio.getWritePointer(1,start):nullptr};engine.process(channels,count,end-start);};
    int offset=0;
    for(const auto event:midi) {
        const int next=juce::jlimit(offset,audio.getNumSamples(),event.samplePosition);process(offset,next);offset=next;
        // Only short note/controller messages affect this effect. Avoid copying
        // an unrelated SysEx payload (and allocating) on the audio thread.
        if(event.numBytes>3)continue;
        const auto message=event.getMessage();
        if(message.isNoteOn())engine.midiNote(message.getChannel(),message.getNoteNumber(),true);
        else if(message.isNoteOff())engine.midiNote(message.getChannel(),message.getNoteNumber(),false);
        else if(message.isControllerOfType(64))engine.sustain(message.getChannel(),message.getControllerValue()>=64);
        else if(message.isAllNotesOff()||message.isAllSoundOff())engine.allNotesOff(message.getChannel());
    }
    process(offset,audio.getNumSamples());midi.clear();
    const auto r=engine.reading();pitch=r.note;frequency=r.frequency;target=r.target;correction=r.correction;confidence=r.confidence;level=r.inputDb;voiced=r.voiced;hasTarget=r.hasTarget;samples=r.samples;
}
void HauntProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){run(b,m,false);}
void HauntProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){run(b,m,true);}
Reading HauntProcessor::reading() const {Reading r;r.note=pitch;r.frequency=frequency;r.target=target;r.correction=correction;r.confidence=confidence;r.inputDb=level;r.voiced=voiced;r.hasTarget=hasTarget;r.samples=samples;return r;}
void HauntProcessor::setPreset(int preset) {
    if(preset==0){setValue("retune",45);setValue("amount",85);setValue("humanize",60);setValue("vibrato",80);}
    else if(preset==1){setValue("retune",0);setValue("amount",100);setValue("humanize",0);setValue("vibrato",0);}
    else {setValue("retune",18);setValue("amount",100);setValue("humanize",20);setValue("vibrato",35);}
}
void HauntProcessor::selectBank(int b) {const juce::ScopedLock lock(bankLock);b=juce::jlimit(0,1,b);if(b==bank)return;banks[bank.load()]=state.copyState();bank=b;state.replaceState(banks[bank.load()].createCopy());}
void HauntProcessor::copyBank(){const juce::ScopedLock lock(bankLock);banks[1-bank.load()]=state.copyState();}
void HauntProcessor::getStateInformation(juce::MemoryBlock& data) {
    const juce::ScopedLock lock(bankLock);
    juce::ValueTree root("HAUNT_SESSION");root.setProperty("version",1,nullptr);root.setProperty("bank",bank.load(),nullptr);
    for(int i=0;i<2;++i)root.addChild((i==bank?state.copyState():banks[i].createCopy()),-1,nullptr);
    if(auto xml=root.createXml())copyXmlToBinary(*xml,data);
}
void HauntProcessor::setStateInformation(const void* data,int bytes) {
    const juce::ScopedLock lock(bankLock);
    if(auto xml=getXmlFromBinary(data,bytes)) {
        const auto tree=juce::ValueTree::fromXml(*xml);
        if(tree.hasType("HAUNT_SESSION")&&tree.getNumChildren()==2&&tree.getChild(0).hasType("HAUNT")&&tree.getChild(1).hasType("HAUNT")) {
            banks[0]=tree.getChild(0).createCopy();banks[1]=tree.getChild(1).createCopy();bank=juce::jlimit(0,1,int(tree["bank"]));state.replaceState(banks[bank.load()].createCopy());
        }
    }
}
juce::AudioProcessorEditor* HauntProcessor::createEditor(){return new HauntEditor(*this);}
}
