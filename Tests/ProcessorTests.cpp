#include "SuiteProcessor.h"
#include <iostream>
#include <stdexcept>
using namespace hungryghost;
namespace {
void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
void set(SuiteProcessor& p, const juce::String& id, float value) {
  auto* parameter = p.state.getParameter(id);
  parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
int find(Kind kind) {
  for (int i = 2; i < 50; ++i) if (products[i].kind == kind) return i;
  throw std::runtime_error("Missing product");
}
double amplitude(const std::vector<float>& samples, double hz) {
  double re = 0, im = 0;
  for (size_t i = 4800; i < samples.size(); ++i) {
    double phase = juce::MathConstants<double>::twoPi * hz * i / 48000.;
    re += samples[i] * std::cos(phase); im += samples[i] * std::sin(phase);
  }
  return 2 * std::hypot(re, im) / (samples.size() - 4800);
}
}
int main() {
  juce::ScopedJuceInitialiser_GUI init;
  try {
    juce::MidiBuffer midi;
    // Exercise prepare/reprepare with mono/stereo and oversampling changes.
    for (int index = 2; index < 50; ++index) {
      SuiteProcessor p(index);
      require(p.licence.canProcess(), "Processor tests require an active trial or licence");
      for (double sr : {22050., 44100., 48000., 96000., 192000., 384000.}) {
        for (int channels : {1, 2}) {
          auto layout = p.getBusesLayout();
          layout.inputBuses.set(0, channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
          layout.outputBuses.set(0, layout.inputBuses[0]);
          require(p.setBusesLayout(layout), "Supported main bus rejected");
          p.prepareToPlay(sr, 137);
          for (int n : {0, 1, 137, 8193}) {
            juce::AudioBuffer<float> buffer(channels, n);
            for (int ch = 0; ch < channels; ++ch)
              for (int i = 0; i < n; ++i) buffer.setSample(ch, i, .25f * std::sin(i * .13f));
            p.processBlock(buffer, midi);
            for (int ch = 0; ch < channels; ++ch)
              for (int i = 0; i < n; ++i) require(std::isfinite(buffer.getSample(ch,i)), "Invalid processor output");
          }
          // Starting bypass must produce an exact latency-matched impulse.
          set(p, "bypass", 1); p.reset();
          juce::AudioBuffer<float> impulse(channels, 512); impulse.clear();
          for (int ch = 0; ch < channels; ++ch) impulse.setSample(ch, 0, .7f);
          p.processBlock(impulse, midi);
          for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < 512; ++i)
              require(std::abs(impulse.getSample(ch,i) - (i == p.getLatencySamples() ? .7f : 0.f)) < 1.e-6f,
                      "Bypass impulse disagrees with host latency");
          set(p, "bypass", 0); p.reset();
          juce::AudioBuffer<float> malformed(channels, 8193); malformed.clear();
          malformed.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
          malformed.setSample(0, 1, std::numeric_limits<float>::infinity());
          p.processBlock(malformed, midi);
          for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < malformed.getNumSamples(); ++i)
              require(std::isfinite(malformed.getSample(ch,i)), "Non-finite input contaminated processor state");
          p.releaseResources();
        }
      }
    }
    // External-key compression must respond to the key rather than the main signal.
    SuiteProcessor duck(find(Kind::Ducker));
    require(duck.setChannelLayoutOfBus(true, 1, juce::AudioChannelSet::stereo()), "External key bus rejected");
    duck.prepareToPlay(48000, 48000);
    set(duck, "control0", -30); set(duck, "control1", 10); set(duck, "control2", .1f); set(duck, "control4", 24);
    duck.reset();
    juce::AudioBuffer<float> keyed(4,48000);
    for (int i = 0; i < 48000; ++i) {
      keyed.setSample(0,i,.1f); keyed.setSample(1,i,.1f);
      keyed.setSample(2,i,.8f); keyed.setSample(3,i,.8f);
    }
    duck.processBlock(keyed,midi);
    require(std::abs(keyed.getSample(0,47999)) < .02f, "Ducker external key did not reduce main input");
    // Compare a coherent high-frequency tone's third-harmonic alias with the raw engine.
    SuiteProcessor ember(find(Kind::SoftSaturation)); ember.prepareToPlay(48000, 512);
    set(ember,"control0",24); set(ember,"control1",20000); set(ember,"mix",1); ember.reset();
    SuiteEngine raw(Kind::SoftSaturation); raw.prepare(48000); raw.setControls({24,20000,0,0,0,0},1,0); raw.reset();
    std::vector<float> reference(52800), output(52800);
    for (int i = 0; i < 52800; ++i) reference[i] = .5f * std::sin(juce::MathConstants<double>::twoPi * 10000 * i / 48000.);
    output = reference; raw.process(reference.data(),nullptr,nullptr,nullptr,static_cast<int>(reference.size()));
    for (int offset = 0; offset < 52800; offset += 512) {
      int n = std::min(512,52800-offset); juce::AudioBuffer<float> block(2,n);
      for (int i = 0; i < n; ++i) for (int ch = 0; ch < 2; ++ch) block.setSample(ch,i,output[offset+i]);
      ember.processBlock(block,midi);
      for (int i = 0; i < n; ++i) output[offset+i] = block.getSample(0,i);
    }
    const double before = amplitude(reference,18000), after = amplitude(output,18000);
    std::cout << "EMBER 18 kHz alias: " << 20*std::log10(before) << " -> " << 20*std::log10(after) << " dBFS\n";
    require(after < before * .4, "Oversampling failed to suppress the third-harmonic alias");
    std::cout << "PASS: 48 processors, six rates, mono/stereo, oversized blocks, latency bypass, external key and alias suppression\n";
    return 0;
  } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
