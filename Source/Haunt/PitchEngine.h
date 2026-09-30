#pragma once
#include <array>
#include <cstdint>
#include <memory>

namespace hungryghost::haunt {
struct Settings {
    float retuneMs=35, amount=1, humanize=.35f, vibrato=.5f;
    float formant=0, transpose=0, reference=440, mix=1, outputDb=0;
    int key=0, scale=0, range=0;
    unsigned customMask=4095;
    bool preserveFormants=true, midiTarget=false, bypass=false;
};
struct Reading {
    float frequency=0, note=0, target=0, correction=0, confidence=0, inputDb=-100;
    std::uint64_t samples=0;
    bool voiced=false, hasTarget=false;
};
unsigned scaleMask(int key, int scale, unsigned customMask=4095);
float nearestNote(float note, unsigned mask, float previous=-1);
class PitchEngine {
public:
    PitchEngine();
    ~PitchEngine();
    void prepare(double sampleRate, int channels);
    void reset();
    void setSettings(const Settings&);
    void midiNote(int channel, int note, bool down);
    void sustain(int channel, bool down);
    void allNotesOff(int channel=-1);
    void process(float* const* channels, int channelCount, int samples);
    int latencySamples() const;
    Reading reading() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
