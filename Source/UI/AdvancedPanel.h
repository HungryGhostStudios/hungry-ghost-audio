#pragma once
#include "../SuiteProcessor.h"
#include "GhostTheme.h"
namespace hungryghost {
class AdvancedPanel final:public juce::Component {
public:
  explicit AdvancedPanel(SuiteProcessor& p):processor(p){
    setLookAndFeel(&localTheme);
    title.setText("SHAPE / "+juce::String(p.product.name),juce::dontSendNotification);
    addAndMakeVisible(title);close.setButtonText("Close");addAndMakeVisible(close);
    close.onClick=[this]{setVisible(false);};
    for(const char* id:{"tempo_sync","key_listen","mono_listen"})if(auto* param=p.state.getParameter(id)){
      auto button=std::make_unique<juce::TextButton>(param->getName(60));button->setComponentID(id);button->setClickingTogglesState(true);addAndMakeVisible(*button);
      buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,id,*button));toggles.push_back(std::move(button));
    }
    if(p.state.getParameter("beat_division")){
      division=std::make_unique<juce::ComboBox>();division->setComponentID("beat_division");
      auto* parameter=dynamic_cast<juce::AudioParameterChoice*>(p.state.getParameter("beat_division"));division->addItemList(parameter->choices,1);addAndMakeVisible(*division);
      divisionAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"beat_division",*division);
    }
    for(const char* id:{"fallback_bpm","key_highpass","band_q0","band_q1","band_q2","input_lowcut","repeat_lowcut"})if(auto* param=p.state.getParameter(id)){
      Entry entry;entry.slider=std::make_unique<juce::Slider>();entry.label=std::make_unique<juce::Label>();
      entry.slider->setComponentID(id);entry.slider->setName(param->getName(60));entry.slider->setSliderStyle(juce::Slider::LinearHorizontal);entry.slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,96,26);entry.slider->getProperties().set("suite",true);entry.slider->getProperties().set("compact",true);
      entry.slider->setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
      const bool hz=juce::String(id).contains("cut")||juce::String(id).contains("pass"),bpm=juce::String(id)=="fallback_bpm";
      entry.label->setText(param->getName(60),juce::dontSendNotification);addAndMakeVisible(*entry.label);addAndMakeVisible(*entry.slider);
      entry.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,*entry.slider);
      // SliderAttachment installs the parameter's generic formatter; customise after binding.
      entry.slider->textFromValueFunction=[hz,bpm](double v){return hz&&v<.5?juce::String("Off"):juce::String(v,hz||bpm?0:2)+(hz?" Hz":bpm?" BPM":" Q");};
      entry.slider->valueFromTextFunction=[](const juce::String& t){return t.equalsIgnoreCase("off")?0.:t.getDoubleValue();};
      entry.slider->updateText();entries.push_back(std::move(entry));
    }
    const auto kind=p.product.kind;
    explanation=hasTempoSync(kind)?"Host tempo takes priority. Fallback tempo is used when the host supplies none. Free time / rate returns when sync is off.":
      hasDetector(kind)?"Audition the signal that drives the detector. The key comes from the external bus when connected, otherwise the main input. Filtering changes detection, not the programme tone.":
      hasColourFilter(kind)?"Remove lows before the nonlinear stage to keep bass from driving the distortion. The main Tone control filters afterwards.":
      kind==Kind::ParametricEQ||kind==Kind::TiltEQ?"Control the width and resonance of the existing bands. The response display follows these settings.":
      "Audition the mono sum to check cancellation. Turn it off to return to stereo output.";
    setOpaque(true);setVisible(false);
  }
  ~AdvancedPanel()override{setLookAndFeel(nullptr);}
  int preferredHeight()const{return 114+static_cast<int>(toggles.size())*37+(division?42:0)+static_cast<int>(entries.size())*57;}
  void resized()override{
    title.setBounds(18,12,getWidth()-110,26);close.setBounds(getWidth()-84,12,67,27);int y=50;
    for(auto& toggle:toggles){toggle->setBounds(18,y,getWidth()-36,30);y+=37;}
    if(division){division->setBounds(18,y,getWidth()-36,32);y+=42;}
    for(auto& entry:entries){entry.label->setBounds(18,y,getWidth()-36,20);entry.slider->setBounds(22,y+22,getWidth()-44,27);y+=57;}
  }
  void paint(juce::Graphics& g)override{
    g.fillAll(GhostTheme::background());localTheme.paintPanel(g,getLocalBounds().toFloat(),juce::Colour(0xff29312c));g.setColour(GhostTheme::muted());g.setFont(juce::Font(juce::FontOptions("Segoe UI",12.f,juce::Font::plain)));
    g.drawFittedText(explanation,getLocalBounds().withY(getHeight()-58).withHeight(48).reduced(18,0),juce::Justification::topLeft,3);
  }
private:
  GhostTheme localTheme;
  SuiteProcessor& processor;
  juce::Label title;
  juce::TextButton close;
  juce::String explanation;
  struct Entry{std::unique_ptr<juce::Slider>slider;std::unique_ptr<juce::Label>label;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>attachment;};
  std::vector<Entry>entries;
  std::vector<std::unique_ptr<juce::TextButton>>toggles;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>buttonAttachments;
  std::unique_ptr<juce::ComboBox>division;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>divisionAttachment;
};
} // namespace hungryghost
