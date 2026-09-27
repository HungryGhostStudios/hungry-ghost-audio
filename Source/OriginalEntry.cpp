#if HG_PRODUCT_INDEX == 0
#include "Originals/Reverb/PluginProcessor.h"
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AfterProcessor();
}
#else
#include "Originals/Feral/PluginProcessor.h"
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new FeralProcessor();
}
#endif
