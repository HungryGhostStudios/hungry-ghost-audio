#include "HauntEditor.h"
#include <cmath>
namespace hungryghost::haunt {
namespace {
juce::Colour violet(){return juce::Colour(0xffbaa1eb);}
juce::Colour mint(){return juce::Colour(0xff9dd0c0);}
juce::Font font(float size,bool bold=false){return juce::Font(juce::FontOptions("Segoe UI",size,bold?juce::Font::bold:juce::Font::plain));}
const char* noteNames[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
juce::String noteName(float value){const int n=juce::jlimit(0,127,int(std::round(value)));return juce::String(noteNames[n%12])+juce::String(n/12-1);}
void text(juce::Graphics& g,juce::String value,juce::Rectangle<float> rect,float size,juce::Colour colour,bool bold=false,int justify=juce::Justification::left){g.setFont(font(size,bold));g.setColour(colour);g.drawText(value,rect,justify);}
}
HauntEditor::HauntEditor(HauntProcessor& p):AudioProcessorEditor(p),processor(p),surface(*this),license(p.license) {
    setLookAndFeel(&theme);addAndMakeVisible(surface);
    license.setBounds(279,691,185,29);license.setName("Activate HAUNT or suite licence");surface.addAndMakeVisible(license);
    auto combo=[&](juce::ComboBox& box,const char* id,const juce::StringArray& items,juce::Rectangle<int> bounds) {
        box.addItemList(items,1);box.setBounds(bounds);box.setName(id);box.setTitle(id);box.setComponentID(id);box.setWantsKeyboardFocus(true);surface.addAndMakeVisible(box);
        comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,id,box));
    };
    combo(key,"key",{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},{36,113,106,32});
    combo(scale,"scale",{"Chromatic","Major","Minor","Major pentatonic","Custom"},{154,113,236,32});
    combo(range,"range",{"Wide / 65-1100 Hz","Low / 65-350 Hz","Mid / 100-700 Hz","High / 160-1100 Hz"},{406,113,225,32});
    preset.addItemList({"Natural / starting point","Hard tune / starting point","Modern / starting point"},1);preset.setTextWhenNothingSelected("Choose a starting point");preset.setBounds(590,37,232,30);preset.setName("Starting point");surface.addAndMakeVisible(preset);preset.onChange=[this]{processor.setPreset(preset.getSelectedId()-1);};
    auto button=[&](juce::TextButton& btn,const char* id,juce::Rectangle<int> bounds){btn.setBounds(bounds);btn.setClickingTogglesState(true);btn.setName(id);btn.setTitle(id);btn.setComponentID(id);surface.addAndMakeVisible(btn);buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,id,btn));};
    button(preserve,"preserve",{646,113,149,32});button(midi,"midi",{809,113,128,32});button(bypass,"bypass",{957,691,107,29});
    a.setBounds(843,37,44,30);b.setBounds(895,37,44,30);copy.setBounds(951,37,113,30);
    for(auto* btn:{&a,&b,&copy}){surface.addAndMakeVisible(btn);btn->setName(btn->getButtonText());}
    a.setClickingTogglesState(true);b.setClickingTogglesState(true);a.onClick=[this]{processor.selectBank(0);};b.onClick=[this]{processor.selectBank(1);};copy.onClick=[this]{processor.copyBank();};
    const char* ids[]={"retune","amount","humanize","vibrato","formant","transpose","reference","mix","output"};
    const char* names[]={"Retune speed","Correction strength","Humanize sustained notes","Preserve vibrato","Formant shift","Transpose","Reference tuning","Dry / wet","Output gain"};
    const char* units[]={" ms"," %"," %"," %"," st"," st"," Hz"," %"," dB"};
    const char* tips[]={"How quickly the voice moves to the target. Zero creates hard tuning.","How much detected pitch error is corrected.","Slows correction on sustained notes. Has no effect when retune is zero.","Preserves short pitch movement around the slowly changing vocal pitch.","Shifts the vocal spectral envelope independently of tuning.","Shifts the complete vocal by this many semitones.","Frequency used for concert A4.","Blends the tuned signal with the original, aligned to the same latency.","Level after the pitch engine."};
    for(int i=0;i<9;++i) {
        auto& s=sliders[i];s.setComponentID(ids[i]);s.setName(names[i]);s.setTitle(names[i]);s.setTooltip(tips[i]);s.setWantsKeyboardFocus(true);
        s.setSliderStyle(i<6?juce::Slider::RotaryHorizontalVerticalDrag:juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(i<6?juce::Slider::TextBoxBelow:juce::Slider::TextBoxRight,false,i<6?115:70,25);
        s.setColour(juce::Slider::thumbColourId,i==1?mint():violet());s.setTextValueSuffix(units[i]);s.setNumDecimalPlacesToDisplay(i==4||i==5||i==8?1:0);
        const auto* parameter=p.state.getParameter(ids[i]);s.setDoubleClickReturnValue(true,parameter->convertFrom0to1(parameter->getDefaultValue()));
        s.setBounds(i<6?juce::Rectangle<int>{39+i*174,508,153,139}:i==6?juce::Rectangle<int>{949,115,115,28}:i==7?juce::Rectangle<int>{522,692,170,27}:juce::Rectangle<int>{737,692,175,27});
        surface.addAndMakeVisible(s);sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s));
        const int decimals=i==4||i==5||i==8?1:0;
        s.textFromValueFunction=[decimals](double value){return decimals?juce::String(value,decimals):juce::String(int(std::round(value)));};
        s.updateText();
    }
    for(int i=0;i<12;++i) {
        auto& n=notes[i];n.setButtonText(noteNames[i]);n.setName("Allow "+juce::String(noteNames[i]));n.setTitle(n.getName());n.setComponentID("note"+juce::String(i));n.setBounds(36+i*86,439,79,37);n.setClickingTogglesState(true);surface.addAndMakeVisible(n);
        n.onClick=[this,i]{unsigned custom=0;for(int j=0;j<12;++j)if(processor.value("note"+juce::String(j))>.5f)custom|=1u<<j;
            unsigned mask=scaleMask(int(processor.value("key")),int(processor.value("scale")),custom)^(1u<<i);
            for(int j=0;j<12;++j)processor.setValue("note"+juce::String(j),(mask&(1u<<j))?1.f:0.f);processor.setValue("scale",4);poll();};
    }
    preserve.setTooltip("Compensate the formant envelope for pitch changes. Formant Shift remains available.");midi.setTooltip("Tune to held MIDI notes. Sustain pedal is supported. No held notes means no automatic correction.");
    getConstrainer()->setFixedAspectRatio(1100./740.);setResizable(true,true);setResizeLimits(880,592,1650,1110);setSize(1100,740);startTimerHz(40);poll();
}
HauntEditor::~HauntEditor(){stopTimer();setLookAndFeel(nullptr);}
void HauntEditor::paint(juce::Graphics& g){g.fillAll(GhostTheme::background());}
void HauntEditor::resized(){surface.setBounds(0,0,1100,740);surface.setTransform(juce::AffineTransform::scale(getWidth()/1100.f,getHeight()/740.f));}
void HauntEditor::poll() {
    current=processor.reading();
    if(current.samples!=lastSample) {if(current.samples<lastSample)history.clear();history.push_back(current);lastSample=current.samples;while(history.size()>900)history.pop_front();}
    if(current.voiced&&std::abs(current.note-displayCentre)>4)displayCentre+=.13f*(current.note-displayCentre);
    unsigned custom=0;for(int i=0;i<12;++i)if(processor.value("note"+juce::String(i))>.5f)custom|=1u<<i;
    const unsigned mask=scaleMask(int(processor.value("key")),int(processor.value("scale")),custom);
    const bool midiActive=processor.value("midi")>.5f;
    for(int i=0;i<12;++i){notes[i].setToggleState((mask&(1u<<i))!=0,juce::dontSendNotification);notes[i].setEnabled(!midiActive);}
    key.setEnabled(!midiActive);scale.setEnabled(!midiActive);a.setToggleState(processor.selectedBank()==0,juce::dontSendNotification);b.setToggleState(processor.selectedBank()==1,juce::dontSendNotification);surface.repaint();
}
void HauntEditor::paintSurface(juce::Graphics& g) {
    theme.paintChassis(g,{0,0,1100,740});theme.paintSpectralMark(g,{478,18,85,67},violet());
    text(g,"HUNGRY GHOST / VOCAL INSTRUMENT",{36,18,440,17},10,GhostTheme::muted());theme.paintWordmark(g,"HAUNT",{34,36,400,44},38);
    text(g,"REAL-TIME PITCH / EARLY ACCESS",{37,79,470,18},10,violet(),true);
    for(auto pair:{std::pair<const char*,float>{"KEY",36},{"SCALE",154},{"VOICE RANGE",406},{"REFERENCE A4",949}})text(g,pair.first,{pair.second,96,220,15},9,GhostTheme::muted(),true);
    const juce::Rectangle<float> graph{36,168,807,232};theme.paintDisplay(g,graph.expanded(6));
    const auto plot=graph.withTrimmedLeft(44).withTrimmedTop(29).withTrimmedRight(13).withTrimmedBottom(19);
    text(g,"PITCH TRACE",{49,176,180,18},10,GhostTheme::ink(),true);
    text(g,"INPUT",{598,176,70,18},9,mint(),true);text(g,"TARGET",{678,176,90,18},9,violet(),true);
    const float low=std::floor(displayCentre)-6, high=low+12;
    const auto y=[&](float n){return plot.getBottom()-(n-low)/12*plot.getHeight();};
    for(int n=int(low);n<=int(high);++n) {
        const bool black=n%12==1||n%12==3||n%12==6||n%12==8||n%12==10;
        if(black){g.setColour(juce::Colour(0xff171c25));g.fillRect(plot.getX(),y(float(n))-(plot.getHeight()/24),plot.getWidth(),plot.getHeight()/12);}
        g.setColour(violet().withAlpha(n%12==0?.20f:.055f));g.drawHorizontalLine(int(y(float(n))),plot.getX(),plot.getRight());
        text(g,noteName(float(n)),{42,y(float(n))-7,34,14},9,GhostTheme::muted(),false,juce::Justification::centredRight);
    }
    for(int i=0;i<7;++i){g.setColour(violet().withAlpha(.08f));g.drawVerticalLine(int(plot.getX()+plot.getWidth()*i/6),plot.getY(),plot.getBottom());}
    if(history.size()>1) {
        juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(plot.toNearestInt());
        juce::Path inputPath,targetPath;bool inputOpen=false,targetOpen=false;
        const double sr=processor.getSampleRate()>0?processor.getSampleRate():48000;
        for(const auto& r:history){const float x=plot.getRight()-float(double(current.samples-r.samples)/(sr*6))*plot.getWidth();
            if(x<plot.getX()){inputOpen=targetOpen=false;continue;}
            if(r.voiced){if(!inputOpen)inputPath.startNewSubPath(x,y(r.note));else inputPath.lineTo(x,y(r.note));inputOpen=true;}else inputOpen=false;
            if(r.hasTarget){if(!targetOpen)targetPath.startNewSubPath(x,y(r.target));else targetPath.lineTo(x,y(r.target));targetOpen=true;}else targetOpen=false;
        }
        g.setColour(mint().withAlpha(.12f));g.strokePath(inputPath,juce::PathStrokeType(6));g.setColour(mint());g.strokePath(inputPath,juce::PathStrokeType(1.6f));
        g.setColour(violet().withAlpha(.78f));const float dash[]={5,5};juce::Path dashed;juce::PathStrokeType(1).createDashedStroke(dashed,targetPath,dash,2);g.fillPath(dashed);
    }
    if(history.empty())text(g,"Sing a single vocal line to see its pitch",plot,16,GhostTheme::muted(),false,juce::Justification::centred);
    theme.paintPanel(g,{863,162,201,244},violet());
    text(g,"DETECTED",{880,178,165,18},10,GhostTheme::muted(),true);
    text(g,current.voiced?noteName(current.note):"--",{877,194,170,50},39,GhostTheme::ink(),true);
    const float cents=(current.note-std::round(current.note))*100;
    text(g,current.voiced?juce::String(cents>=0?"+":"")+juce::String(int(std::round(cents)))+" cents / "+juce::String(current.frequency,1)+" Hz":"Waiting for voiced audio",{880,248,168,19},10,mint());
    g.setColour(violet().withAlpha(.10f));g.fillRoundedRectangle(880,282,166,4,2);g.setColour(violet());g.fillRoundedRectangle(880,282,166*current.confidence,4,2);
    text(g,"CONFIDENCE  "+juce::String(int(std::round(current.confidence*100)))+"%",{880,291,170,17},9,GhostTheme::muted());
    text(g,"TARGET  "+(current.hasTarget?noteName(current.target):juce::String("--")),{880,324,170,22},16,violet(),true);
    text(g,"CORRECTION  "+juce::String(current.correction,2)+" st",{880,358,170,18},10,GhostTheme::ink());
    text(g,"ALLOWED NOTES / CLICK TO CUSTOMISE",{36,413,590,18},9,GhostTheme::muted(),true);
    text(g,processor.value("midi")>.5f?"MIDI / HELD NOTES":current.hasTarget?"TRACKING":current.voiced?"NO TARGET NOTES":"LISTENING",{849,413,214,18},9,violet(),true,juce::Justification::centredRight);
    const char* labels[]={"RETUNE","CORRECTION","HUMANIZE","VIBRATO","FORMANT","TRANSPOSE"};
    const char* hints[]={"Note transition speed","Pull towards target","Ease sustained notes","Keep expressive motion","Vocal envelope","Shift the whole voice"};
    for(int i=0;i<6;++i){const float x=36+i*174.f;theme.paintPanel(g,{x,492,158,177},i==1?mint():violet());text(g,labels[i],{x+8,498,142,20},10,GhostTheme::ink(),true,juce::Justification::centred);text(g,hints[i],{x+4,647,150,15},9,GhostTheme::muted(),false,juce::Justification::centred);}
    const double sr=processor.getSampleRate()>0?processor.getSampleRate():48000;
    text(g,"HAUNT 0.1.3 / EARLY ACCESS",{36,689,232,17},10,GhostTheme::muted(),true);
    text(g,juce::String(processor.getLatencySamples()*1000./sr,1)+" ms reported latency / "+juce::String(sr/1000,1)+" kHz",{36,710,232,16},9,GhostTheme::muted());
    text(g,"MIX",{487,696,36,20},9,GhostTheme::muted(),true);text(g,"OUT",{702,696,36,20},9,GhostTheme::muted(),true);
}
}
