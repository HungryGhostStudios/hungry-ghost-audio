#include "Entitlement.h"
#include "PublicKey.h"
namespace hungryghost {
namespace {
bool decode(const juce::String &encoded, juce::MemoryBlock &block) {
  if (encoded.length() > 16384)
    return false;
  for (auto c : encoded)
    if (!(juce::CharacterFunctions::isLetterOrDigit(c) || c == '-' || c == '_'))
      return false;
  juce::String standard =
      encoded.replaceCharacter('-', '+').replaceCharacter('_', '/');
  while (standard.length() % 4)
    standard += '=';
  juce::MemoryOutputStream stream(block, false);
  return juce::Base64::convertFromBase64(stream, standard);
}
juce::String hex(const juce::MemoryBlock &block) {
  return juce::String::toHexString(block.getData(),
                                   static_cast<int>(block.getSize()), 0);
}
} // namespace
juce::Result Entitlement::verify(const juce::String &envelope,
                                 const juce::String &device,
                                 const juce::String &product, juce::int64 now,
                                 Entitlement &output) {
  output = {};
  if (envelope.length() > 20000)
    return juce::Result::fail("Entitlement too large");
  auto document = juce::JSON::parse(envelope);
  if (!document.isObject())
    return juce::Result::fail("Invalid entitlement");
  juce::MemoryBlock payload, signature;
  if (!decode(document["payload"].toString(), payload) ||
      !decode(document["signature"].toString(), signature) ||
      signature.getSize() != licenceSignatureBytes)
    return juce::Result::fail("Invalid signature encoding");
  juce::BigInteger sig, modulus;
  sig.parseString(hex(signature), 16);
  modulus.parseString(
      juce::String(licencePublicKey).fromFirstOccurrenceOf(",", false, false),
      16);
  if (sig.isZero() || sig >= modulus)
    return juce::Result::fail("Invalid signature value");
  juce::RSAKey key(licencePublicKey);
  if (!key.applyToValue(sig))
    return juce::Result::fail("Signature verification failed");
  const juce::SHA256 hash(payload);
  const juce::String digestInfo =
      "3031300d060960864801650304020105000420" + hash.toHexString();
  juce::String expectedHex = "0001";
  const int padding = licenceSignatureBytes - 3 - digestInfo.length() / 2;
  for (int i = 0; i < padding; ++i)
    expectedHex += "ff";
  expectedHex += "00" + digestInfo;
  juce::BigInteger expected;
  expected.parseString(expectedHex, 16);
  if (sig != expected)
    return juce::Result::fail("Signature verification failed");
  auto data = juce::JSON::parse(
      juce::String::fromUTF8(static_cast<const char *>(payload.getData()),
                             static_cast<int>(payload.getSize())));
  if (!data.isObject() || static_cast<int>(data["v"]) != 1)
    return juce::Result::fail("Unsupported entitlement");
  if (data["device"].toString() != device)
    return juce::Result::fail("Licence belongs to another device");
  auto *owned = data["products"].getArray();
  if (!owned || !owned->contains(juce::var(product)))
    return juce::Result::fail("Licence does not include this product");
  auto issued = static_cast<juce::int64>(data["issued"]),
       expires = static_cast<juce::int64>(data["expires"]);
  if (issued <= 0 || expires <= issued || issued > now + 300 || expires <= now)
    return juce::Result::fail("Licence needs an online refresh");
  output.activationId = data["activationId"].toString();
  output.licenceId = data["licenceId"].toString();
  output.expires = expires;
  for (const auto &id : *owned)
    output.products.add(id.toString());
  if (output.activationId.isEmpty() || output.licenceId.isEmpty())
    return juce::Result::fail("Incomplete entitlement");
  return juce::Result::ok();
}
} // namespace hungryghost
