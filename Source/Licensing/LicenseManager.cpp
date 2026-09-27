#include "LicenseManager.h"
#include "../UI/GhostTheme.h"
namespace hungryghost {
juce::CriticalSection LicenseManager::fileLock;
namespace {
constexpr juce::int64 day = 86400;
juce::int64 nowSeconds() {
  return juce::Time::getCurrentTime().toMilliseconds() / 1000;
}
juce::File storage() {
  return juce::File::getSpecialLocation(
             juce::File::userApplicationDataDirectory)
      .getChildFile("Hungry Ghost Audio")
      .getChildFile("Licences");
}
juce::var object() { return juce::var(new juce::DynamicObject()); }
class LicensePanel final : public juce::Component, private juce::Timer {
public:
  explicit LicensePanel(LicenseManager &m) : manager(&m) {
    setLookAndFeel(&theme);
    setSize(460, 310);
    title.setText("HUNGRY GHOST / ACTIVATION", juce::dontSendNotification);
    title.setFont(
        juce::Font(juce::FontOptions("Segoe UI", 18.f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, GhostTheme::ink());
    addAndMakeVisible(title);
    description.setText("Enter the licence key from your Polar receipt. Your "
                        "purchase includes two devices.",
                        juce::dontSendNotification);
    description.setColour(juce::Label::textColourId, GhostTheme::muted());
    addAndMakeVisible(description);
    key.setTextToShowWhenEmpty("HG_ licence key", GhostTheme::muted());
    key.setPasswordCharacter('*');
    key.setColour(juce::TextEditor::backgroundColourId,
                  GhostTheme::background());
    key.setColour(juce::TextEditor::textColourId, GhostTheme::ink());
    key.setColour(juce::TextEditor::outlineColourId, GhostTheme::line());
    addAndMakeVisible(key);
    activate.setButtonText("Activate / refresh");
    activate.onClick = [this] {
      if (auto *licence = manager.get())
        licence->activate(key.getText().trim());
    };
    addAndMakeVisible(activate);
    portal.setButtonText("Manage devices");
    portal.onClick = [] {
      juce::URL("https://polar.sh/hungry-ghost-audio/portal")
          .launchInDefaultBrowser();
    };
    addAndMakeVisible(portal);
    statusLabel.setColour(juce::Label::textColourId, GhostTheme::accent());
    addAndMakeVisible(statusLabel);
    privacy.setText(
        "Activation sends your key and a hashed device ID. No audio or project "
        "data leaves the plugin. Signed licences work offline for 90 days.",
        juce::dontSendNotification);
    privacy.setColour(juce::Label::textColourId, GhostTheme::muted());
    addAndMakeVisible(privacy);
    startTimerHz(4);
    timerCallback();
  }
  ~LicensePanel() override {
    stopTimer();
    setLookAndFeel(nullptr);
  }
  void resized() override {
    title.setBounds(20, 14, 420, 28);
    description.setBounds(20, 50, 420, 48);
    key.setBounds(22, 111, 416, 35);
    activate.setBounds(22, 160, 210, 33);
    portal.setBounds(244, 160, 194, 33);
    statusLabel.setBounds(20, 201, 420, 36);
    privacy.setBounds(20, 245, 420, 55);
  }
  void paint(juce::Graphics &g) override { g.fillAll(GhostTheme::panel()); }

private:
  GhostTheme theme;
  juce::WeakReference<LicenseManager> manager;
  juce::Label title, description, statusLabel, privacy;
  juce::TextEditor key;
  juce::TextButton activate, portal;
  void timerCallback() override {
    auto *licence = manager.get();
    statusLabel.setText(licence ? licence->status() : "Plugin closed",
                        juce::dontSendNotification);
    activate.setEnabled(licence && !licence->isBusy());
  }
};
} // namespace
juce::String LicenseManager::deviceId() {
  auto id = juce::SystemStats::getUniqueDeviceID();
  if (id.isEmpty()) {
    juce::ScopedLock guard(fileLock);
    auto folder = storage();
    folder.createDirectory();
    auto file = folder.getChildFile("device-id");
    id = file.loadFileAsString().trim();
    if (id.isEmpty()) {
      id = juce::Uuid().toString();
      file.replaceWithText(id);
    }
  }
  return juce::SHA256(juce::String("hungryghost.audio/v1/") + id).toHexString();
}
LicenseManager::LicenseManager(juce::String id,juce::File cacheDirectory)
    : Thread("Hungry Ghost licence"), device(deviceId()),
      product(std::move(id)), directory(cacheDirectory==juce::File()?storage():cacheDirectory) {
  directory.createDirectory();
  if (!readCached())
    checkTrial();
  // Refresh an older paid cache without stalling the editor or a running
  // session.
  if (pendingKey.isNotEmpty()) {
    busy.store(true);
    startThread();
  }
}
LicenseManager::~LicenseManager() {
  signalThreadShouldExit();
  stopThread(12000);
}
juce::String LicenseManager::status() const {
  juce::ScopedLock guard(lock);
  return message;
}
void LicenseManager::setMessage(const juce::String &text) {
  juce::ScopedLock guard(lock);
  message = text;
}
bool LicenseManager::readCached() {
  juce::ScopedLock guard(fileLock);
  juce::InterProcessLock cacheLock("HungryGhostAudioLicenceCache");
  const juce::InterProcessLock::ScopedLockType cacheGuard(cacheLock);
  for (const auto &file :
       directory.getChildFile("entitlements")
           .findChildFiles(juce::File::findFiles, false, "*.json")) {
    auto cache = juce::JSON::parse(file.loadFileAsString());
    if (cache["device"].toString() != device)
      continue;
    Entitlement entitlement;
    auto result = Entitlement::verify(cache["entitlement"].toString(), device,
                                      product, nowSeconds(), entitlement);
    if (result.wasOk()) {
      allowed.store(true);
      setMessage("Licensed / offline ready");
      if (entitlement.expires - nowSeconds() < 14 * day) {
        pendingKey = cache["key"].toString();
        pendingActivation = entitlement.activationId;
      }
      return true;
    }
    // These unsigned routing hints only select a key to ask Polar to verify;
    // they never authorize audio processing or replace signature verification.
    auto *products = cache["products"].getArray();
    if (products && products->contains(juce::var(product)) &&
        cache["key"].toString().isNotEmpty()) {
      pendingKey = cache["key"].toString();
      pendingActivation = cache["activation"].toString();
    }
  }
  return false;
}
void LicenseManager::checkTrial() {
  juce::ScopedLock guard(fileLock);
  juce::InterProcessLock cacheLock("HungryGhostAudioLicenceCache");
  const juce::InterProcessLock::ScopedLockType cacheGuard(cacheLock);
  auto file = directory.getChildFile("trial.json");
  auto data = juce::JSON::parse(file.loadFileAsString());
  const auto now = nowSeconds();
  if (!file.existsAsFile()) {
    data = object();
    data.getDynamicObject()->setProperty("first", now);
    data.getDynamicObject()->setProperty("last", now);
    data.getDynamicObject()->setProperty("device", device);
    file.replaceWithText(juce::JSON::toString(data));
  }
  const auto first = static_cast<juce::int64>(data["first"]),
             last = static_cast<juce::int64>(data["last"]);
  const bool valid = data.isObject() && data["device"].toString() == device &&
                     first > 0 && first <= now + 300 && now >= last - 300 &&
                     now - first < 30 * day;
  allowed.store(valid);
  if (valid) {
    auto remaining =
        std::max<juce::int64>(1, (30 * day - (now - first) + day - 1) / day);
    setMessage("Trial / " + juce::String(remaining) + " days left");
    data.getDynamicObject()->setProperty("last", std::max(last, now));
    file.replaceWithText(juce::JSON::toString(data));
  } else
    setMessage("Activate / trial ended");
}
void LicenseManager::activate(const juce::String &key) {
  if (key.length() < 8 || key.length() > 200) {
    setMessage("Enter the key from your purchase receipt.");
    return;
  }
  if (busy.exchange(true))
    return;
  if (isThreadRunning()) {
    busy.store(false);
    return;
  }
  juce::String activation;
  {
    juce::ScopedLock files(fileLock);
    juce::InterProcessLock cacheLock("HungryGhostAudioLicenceCache");
    const juce::InterProcessLock::ScopedLockType cacheGuard(cacheLock);
    for (const auto &file :
         directory.getChildFile("entitlements")
             .findChildFiles(juce::File::findFiles, false, "*.json")) {
      auto cache = juce::JSON::parse(file.loadFileAsString());
      if (cache["key"].toString() == key &&
          cache["device"].toString() == device) {
        activation = cache["activation"].toString();
        break;
      }
    }
  }
  {
    juce::ScopedLock guard(lock);
    pendingKey = key;
    pendingActivation = activation;
    message = "Verifying licence...";
  }
  startThread();
}
void LicenseManager::run() {
  juce::String key, activation;
  {
    juce::ScopedLock guard(lock);
    key = pendingKey;
    activation = pendingActivation;
  }
  auto body = object();
  auto *properties = body.getDynamicObject();
  properties->setProperty("key", key);
  properties->setProperty("device", device);
  if (activation.isNotEmpty())
    properties->setProperty("activationId", activation);
  const auto url =
      juce::URL("https://hungryghostaudio.com/api/license/" +
                juce::String(activation.isEmpty() ? "activate" : "validate"))
          .withPOSTData(juce::JSON::toString(body));
  int statusCode = 0;
  auto options =
      juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
          .withConnectionTimeoutMs(5000)
          .withExtraHeaders("Content-Type: application/json\r\n")
          .withStatusCode(&statusCode)
          .withNumRedirectsToFollow(0);
  auto stream = url.createInputStream(options);
  if (!stream || threadShouldExit()) {
    setMessage(allowed.load()
                   ? "Offline / cached licence or trial active"
                   : "Could not reach activation. Please try again.");
    busy.store(false);
    return;
  }
  juce::MemoryOutputStream response;
  char buffer[2048];
  while (!stream->isExhausted() && !threadShouldExit() &&
         response.getDataSize() <= 20000) {
    int count = stream->read(buffer, sizeof(buffer));
    if (count <= 0)
      break;
    response.write(buffer, static_cast<std::size_t>(count));
  }
  if (threadShouldExit()) {
    busy.store(false);
    return;
  }
  auto text = response.toString();
  if (statusCode != 200) {
    auto messageData = juce::JSON::parse(text);
    auto error = messageData["error"].toString();
    setMessage(error.isNotEmpty() ? error : "Licence could not be verified.");
    busy.store(false);
    return;
  }
  Entitlement entitlement;
  auto result =
      Entitlement::verify(text, device, product, nowSeconds(), entitlement);
  if (result.failed()) {
    setMessage(result.getErrorMessage());
    busy.store(false);
    return;
  }
  {
    juce::ScopedLock guard(fileLock);
    auto cache = object();
    juce::InterProcessLock cacheLock("HungryGhostAudioLicenceCache");
    const juce::InterProcessLock::ScopedLockType cacheGuard(cacheLock);
    cache.getDynamicObject()->setProperty("key", key);
    cache.getDynamicObject()->setProperty("activation",
                                          entitlement.activationId);
    cache.getDynamicObject()->setProperty("entitlement", text);
    cache.getDynamicObject()->setProperty("device", device);
    juce::Array<juce::var> ids;
    for (const auto &id : entitlement.products)
      ids.add(id);
    cache.getDynamicObject()->setProperty("products", ids);
    auto folder = directory.getChildFile("entitlements");
    folder.createDirectory();
    const auto name = juce::SHA256(key).toHexString();
    auto temporary = folder.getChildFile(name + ".tmp");
    if (!temporary.replaceWithText(juce::JSON::toString(cache)) ||
        !temporary.moveFileTo(folder.getChildFile(name + ".json"))) {
      setMessage("Licence verified, but the local cache could not be saved.");
      busy.store(false);
      return;
    }
  }
  allowed.store(true);
  setMessage("Licensed / offline ready");
  busy.store(false);
}
LicenseButton::LicenseButton(LicenseManager &m) : manager(m) {
  setTooltip("Activate your key or manage device activations");
  onClick = [this] {
    juce::CallOutBox::launchAsynchronously(
        std::make_unique<LicensePanel>(manager), getScreenBounds(), nullptr);
  };
  startTimerHz(2);
  timerCallback();
}
void LicenseButton::timerCallback() { setButtonText(manager.status()); }
} // namespace hungryghost
