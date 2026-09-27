#include "SuiteEditor.h"
#include "SuiteProcessor.h"
#include <iostream>
using namespace hungryghost;
int main() {
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
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    parameter->setValueNotifyingHost(.1f);
    p.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    if (std::abs(parameter->getValue() - .75f) > .0001f)
      return 1;
    p.selectBank(1);
    p.selectBank(0);
    if (std::abs(parameter->getValue() - .75f) > .0001f)
      return 2;
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    for (auto size : {juce::Point<int>{760, 534}, juce::Point<int>{940, 660},
                      juce::Point<int>{1410, 990}}) {
      editor->setSize(size.x, size.y);
      auto image = editor->createComponentSnapshot(editor->getLocalBounds());
      if (!image.isValid())
        return 3;
    }
    std::cout << p.product.name
              << " state/A-B, large block and native editor passed\n";
  }
  return 0;
}
