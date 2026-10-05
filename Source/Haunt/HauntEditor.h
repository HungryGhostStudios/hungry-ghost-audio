#pragma once
#include "HauntProcessor.h"
#include "UI/GhostTheme.h"
#include <deque>
namespace hungryghost::haunt {
class HauntEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit HauntEditor(HauntProcessor&);
    ~HauntEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void poll();
private:
    void timerCallback() override {poll();}
    void paintSurface(juce::Graphics&);
    struct Surface:juce::Component {
        explicit Surface(HauntEditor& e):editor(e){}
        void paint(juce::Graphics& g)override{editor.paintSurface(g);}
        HauntEditor& editor;
    };
    HauntProcessor& processor;
    GhostTheme theme;
    Surface surface;
    juce::ComboBox key,scale,range,preset;
    std::array<juce::Slider,9> sliders;
    std::array<juce::TextButton,12> notes;
    juce::TextButton preserve{"FORMANT LOCK"},midi{"MIDI TARGET"},bypass{"BYPASS"},a{"A"},b{"B"},copy{"COPY"};
    LicenseButton license;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::deque<Reading> history;
    Reading current;
    float displayCentre=60;
    std::uint64_t lastSample=0;
    juce::TooltipWindow tooltip{this,650};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HauntEditor)
};
}
