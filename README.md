# Hungry Ghost Audio

A developing collection of 50 Windows x64 VST3 effects with shared metalwork and individual processing controls. The complete source, original interface assets and storefront are released under **AGPL-3.0-or-later**. JUCE 9.0.2 is the pinned framework dependency.

## Current state

Reverb 0.2.0 and FERAL 0.1.1 have existing validated releases. The additional 48 processors have passed the initial DSP suite; native integration, host validation, licensing and storefront release work are in progress. Do not describe this development branch as a complete production release.

The catalogue lists the actual controls and processing family of every product. The first storefront is in `site/`; checkout remains closed until a validated release and Polar delivery are configured.

## Build

Install Visual Studio 2022 with Desktop C++ and CMake 3.22 or newer.

```powershell
cmake -S . -B build
cmake --build build --config Release --target hg_suite --parallel 8
cmake --build build --config Release --target hg_dsp_tests hg_plugin_tests
ctest --test-dir build -C Release --output-on-failure
```

CMake fetches official JUCE tag 9.0.2. An existing checkout can be supplied using `-DJUCE_SOURCE_DIR=/absolute/path/to/JUCE`. The JUCE modules compile once for the shared runtime, while each plugin retains its own VST3 identity. The original Reverb and FERAL state layouts and plugin IDs remain intact.

The catalogue is generated from `scripts/catalogue.py`. Existing Reverb and FERAL sources are included in `Source/Originals`, so building a release does not require the previous repositories.

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
