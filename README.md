# Hungry Ghost Audio

A collection of 50 effects with shared metalwork and individual processing controls: Windows x64 VST3, and universal macOS VST3 / Audio Units for Intel and Apple Silicon. The complete source, original interface assets, editable Blender scenes, installers and storefront are released under **AGPL-3.0-or-later**. JUCE 9.0.2 is the pinned framework dependency.

## Current state

Suite 0.3.0 brings the Hungry Ghost interface treatment to all 50 effects and adds BOND's new Precision engine. REVERB advances to 0.3.1; the other 49 plugins are 0.3.0. Plugin identities and saved-state compatibility are preserved; processing outside BOND is unchanged by the interface update. Windows x64 VST3 and the signed, notarized universal Mac 0.3.0 installer are available from [hungryghostaudio.com](https://hungryghostaudio.com). See [release notes](Docs/ReleaseNotes.md) and [the BOND guide](Docs/BondPrecision.md).

All 50 exact Windows 0.3.0 release binaries passed pluginval strictness level 5 (seed 2130, GUI tests disabled), including independent Windows PE and VST3 version checks. Six native test targets passed. Native checks cover editor sizing and attachments, six sample rates, mono/stereo, oversized buffers, non-finite input recovery, bypass latency, external-key ducking and oversampling alias suppression. These checks establish tested behaviours, not universal host compatibility or a subjective sound-quality ranking.

The catalogue lists the actual controls and processing family of every product. The Cloudflare storefront is in `site/`; all 50 individual products and the complete suite are connected to live Polar checkout. See [launch status](Docs/LAUNCH-STATUS.md) for verification evidence and the remaining end-to-end order check. Read the [user guide](Docs/UserGuide.md) for installation, the 30-day trial, activation and each processing family.

The [universal 0.3.0 Mac build](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36679329788) and [signed release](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36683669264) passed validation of all 100 universal VST3 / AU bundles on Intel and Apple Silicon. Apple accepted the installer for notarization; its ticket is stapled and Gatekeeper assessment passed. Installer checks exposed all 100 choices and verified exactly the four selected CRUSH/REEL bundles. The complete uploaded installer was read back and its checksum verified before publication; live download checks also passed. Thirty-one storefront/API tests and eight release-metadata tests pass. See [the Mac release evidence](Docs/macOS.md); updating the existing Polar download attachments remains separate and pending.

[Download the signed Mac 0.3.0 installer](https://hungryghostaudio.com/downloads/macos/0.3.0/2138d85a37b0c36b49334ee2a6c55e37f87a7ca46625dc7e2a19cba263b0e24d/HungryGhostSuite-0.3.0-macOS-Universal.pkg). The final stapled installer is 2,615,474,937 bytes. Its SHA-256, verified locally and against the complete remote object, is `2138d85a37b0c36b49334ee2a6c55e37f87a7ca46625dc7e2a19cba263b0e24d`; the notarization record and full validation scope are in [the Mac release evidence](Docs/macOS.md).

## Build

HAUNT, the separate vocal pitch-correction development preview, is opt-in and is not included in the current 50-plugin release. See [its controls, build instructions and live-first roadmap](Docs/Haunt.md). Recorded-note editing and production-quality low-latency monitoring remain future work.

Install Visual Studio 2022 with Desktop C++ and CMake 3.22 or newer.

```powershell
cmake -S . -B build
cmake --build build --config Release --target hg_suite --parallel 8
cmake --build build --config Release --target hg_dsp_tests hg_bond_tests hg_plugin_tests hg_processor_tests hg_license_tests hg_bond_editor_tests
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches official JUCE tag 9.0.2. The corresponding-source release includes it in `vendor/JUCE`, which CMake uses automatically without fetching. An existing checkout can be supplied using `-DJUCE_SOURCE_DIR=/absolute/path/to/JUCE`. JUCE modules compile once for the shared runtime, while each plugin retains its own VST3 identity. Windows builds include the C++ runtime statically. Original Reverb and FERAL state layouts and plugin IDs remain intact.

On Mac, install Xcode command-line tools and CMake, then configure with `-DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DHG_BUILD_STANDALONE=OFF`. The shared suite target builds both VST3 and Audio Units. Public packages require Developer ID signatures and Apple notarization; the workflows in `.github/workflows` perform those checks before packaging.

The catalogue is generated from `scripts/catalogue.py`; its optional `--validated` argument marks the exact hash-verified release. Existing Reverb and FERAL sources are included in `Source/Originals`, so building does not require the previous repositories. `Design/Blender` contains the original editable metalwork scenes and rendering scripts.

## Release checks and packaging

`scripts/validate_release.py` runs pluginval on every catalogue VST3 and records exact binary hashes. `scripts/package_release.py` refuses to package missing, failing or modified binaries. It creates the complete suite, 50 individual ZIPs, a manifest and the installer's file-hash list. `scripts/build_installer.ps1` compiles the Windows installer using the system .NET Framework compiler. `scripts/source_release.py` packages the complete corresponding source with pinned JUCE. Published downloads include SHA-256 checksums.

Release and product versions come from `CMakeLists.txt`; `scripts/release_metadata.py --check` rejects stale native or storefront catalogues. The Windows validator also checks the built VST3 version and Mac validation checks each bundle's version. See [the release procedure](Docs/Releasing.md) for packaging, signing and publication order.

Native licence interoperability checks use the published signed fixtures in `Tests/Fixtures/entitlements.json`. They contain artificial device and activation IDs, and the verifier uses their fixed reference clock. No real customer key or production private key is distributed. Trial-policy checks create an isolated temporary cache on each run. For custom fixtures, `scripts/license_fixtures.mjs` needs a private key matching the verification public key; use an isolated development checkout for a different key. Do not substitute a new production key during an ordinary build. Processor integration checks require an active local trial or licence; the independent DSP tests do not.

## Storefront

```powershell
cd site
npm ci
npm test
npm run dev
```

The Cloudflare Worker serves static pages and a guarded checkout/activation API. Polar organization ID, benefits and checkout destinations are deployment configuration; they are not hard-coded customer secrets. Only the RSA public verification key is shipped in the plugin and source. The private signing key must remain outside this repository and be configured as a Cloudflare secret.

## Licensing and source rights

Source rights are provided by AGPLv3. Customers may inspect, modify, build and redistribute the software under that licence. Paid purchases cover convenient release packages and the store licence service. A signed device entitlement is an authenticity and entitlement check, not a promise of uncrackable software. No audio, session or preset contents are sent for activation.

Read `LICENSE` and JUCE's `LICENSE.md` for the actual licence texts. Third-party dependency notices are supplied with releases. This repository contains no third-party FabFilter, Waves or Valhalla code, assets or trademarks in the plugin presentation.
