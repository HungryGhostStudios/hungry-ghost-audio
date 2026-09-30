#pragma once
#include "PitchEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace hungryghost::haunt {
class HauntProcessor final:public juce::AudioProcessor {
public:
    HauntProcessor();
    void prepareToPlay(double,int) override;
    void releaseResources() override { engine.reset(); }
    void reset() override { engine.reset(); }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    const juce::String getName() const override { return "Hungry Ghost HAUNT Preview"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return .1; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    void setValue(const juce::String&,float);
    float value(const juce::String&) const;
    void setPreset(int);
    void selectBank(int);
    void copyBank();
    int selectedBank() const { return bank.load(); }
    Reading reading() const;
    juce::AudioProcessorValueTreeState state;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    Settings settings() const;
    void run(juce::AudioBuffer<float>&,juce::MidiBuffer&,bool);
    PitchEngine engine;
    std::array<juce::ValueTree,2> banks;
    std::atomic<int> bank{0};
    juce::CriticalSection bankLock;
    std::array<std::atomic<float>*,27> parameters{};
    std::atomic<float> pitch{0},frequency{0},target{0},correction{0},confidence{0},level{-100};
    std::atomic<std::uint64_t> samples{0};
    std::atomic<bool> voiced{false},hasTarget{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HauntProcessor)
};
}
