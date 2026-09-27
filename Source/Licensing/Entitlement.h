#pragma once
#include <juce_cryptography/juce_cryptography.h>
namespace hungryghost {
struct Entitlement {
  juce::String activationId, licenceId;
  juce::StringArray products;
  juce::int64 expires = 0;
  static juce::Result verify(const juce::String &envelope,
                             const juce::String &device,
                             const juce::String &product, juce::int64 now,
                             Entitlement &output);
};
} // namespace hungryghost
