#include "HauntProcessor.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new hungryghost::haunt::HauntProcessor;}
