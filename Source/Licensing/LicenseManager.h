#pragma once
#include "Entitlement.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace hungryghost {
// All I/O, signature verification and networking happen outside the audio
// callback. Audio reads one atomic session flag. Expiration is assessed when
// opening a session.
class LicenseManager final : private juce::Thread {
public:
  explicit LicenseManager(juce::String product,juce::File cacheDirectory={});
  ~LicenseManager() override;
  bool canProcess() const noexcept {
    return allowed.load(std::memory_order_relaxed);
  }
  bool isBusy() const noexcept { return busy.load(); }
  juce::String status() const;
  void activate(const juce::String &key);
  const juce::String device;

private:
  const juce::String product;
  juce::File directory;
  std::atomic<bool> allowed{false}, busy{false};
  mutable juce::CriticalSection lock;
  juce::String message, pendingKey, pendingActivation;
  void run() override;
  void setMessage(const juce::String &);
  bool readCached();
  void checkTrial();
  static juce::String deviceId();
  static juce::CriticalSection fileLock;
  JUCE_DECLARE_WEAK_REFERENCEABLE(LicenseManager)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseManager)
};
class LicenseButton final : public juce::TextButton, private juce::Timer {
public:
  explicit LicenseButton(LicenseManager &);

private:
  LicenseManager &manager;
  void timerCallback() override;
};
} // namespace hungryghost
