# Hungry Ghost Audio

A collection of 50 Windows x64 VST3 effects with shared metalwork and individual processing controls. The complete source, original interface assets, editable Blender scenes, installer and storefront are released under **AGPL-3.0-or-later**. JUCE 9.0.2 is the pinned framework dependency.

## Current state

Suite 0.1.0 includes REVERB 0.3.0, FERAL 0.2.0 and 48 additional effects. All 50 passed pluginval strictness level 5 (seed 2130, GUI tests disabled). Separate native integration checks render all 50 editors at three sizes. Processor checks cover six sample rates, mono/stereo, oversized buffers, non-finite input recovery, exact bypass latency, external-key ducking and measured oversampling alias suppression. These checks establish the tested behaviours; they do not establish compatibility with every host or a subjective sound-quality ranking.

The catalogue lists the actual controls and processing family of every product. The Cloudflare storefront is in `site/`; all 50 individual products and the complete suite are connected to live Polar checkout. See [launch status](Docs/LAUNCH-STATUS.md) for verification evidence and the remaining end-to-end order check. Read the [user guide](Docs/UserGuide.md) for installation, the 30-day trial, activation and each processing family.

## Build

Install Visual Studio 2022 with Desktop C++ and CMake 3.22 or newer.

```powershell
cmake -S . -B build
cmake --build build --config Release --target hg_suite --parallel 8
cmake --build build --config Release --target hg_dsp_tests hg_plugin_tests hg_processor_tests hg_license_tests
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches official JUCE tag 9.0.2. The corresponding-source release includes it in `vendor/JUCE`, which CMake uses automatically without fetching. An existing checkout can be supplied using `-DJUCE_SOURCE_DIR=/absolute/path/to/JUCE`. JUCE modules compile once for the shared runtime, while each plugin retains its own VST3 identity. Windows builds include the C++ runtime statically. Original Reverb and FERAL state layouts and plugin IDs remain intact.

The catalogue is generated from `scripts/catalogue.py`; its optional `--validated` argument marks the exact hash-verified release. Existing Reverb and FERAL sources are included in `Source/Originals`, so building does not require the previous repositories. `Design/Blender` contains the original editable metalwork scenes and rendering scripts.

## Release checks and packaging

`scripts/validate_release.py` runs pluginval on every catalogue VST3 and records exact binary hashes. `scripts/package_release.py` refuses to package missing, failing or modified binaries. It creates the complete suite, 50 individual ZIPs, a manifest and the installer's file-hash list. `scripts/build_installer.ps1` compiles the Windows installer using the system .NET Framework compiler. `scripts/source_release.py` packages the complete corresponding source with pinned JUCE. Published downloads include SHA-256 checksums.

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
