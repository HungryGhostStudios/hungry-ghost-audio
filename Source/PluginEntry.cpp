#include "SuiteProcessor.h"
#ifndef HG_PRODUCT_INDEX
#error HG_PRODUCT_INDEX must identify a catalogue product
#endif
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new hungryghost::SuiteProcessor(HG_PRODUCT_INDEX);
}
