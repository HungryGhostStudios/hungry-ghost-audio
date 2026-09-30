# Suite 0.3.0

This update combines BOND Precision with the Hungry Ghost interface treatment across the complete collection. REVERB advances to 0.3.1; the other 49 plugins are 0.3.0. Windows x64 VST3 and universal Mac VST3 / AU installers are published on [hungryghostaudio.com](https://hungryghostaudio.com). The Mac 0.3.0 installer passed validation, signing, notarization, selectable-installation checks and complete remote checksum verification before publication. The previous 0.2.0 Mac download remains available at its immutable URL.

- Shared blackened metal, etched detail, recessed controls and readable numeric fields, with each plugin retaining its own layout and purpose.
- Existing plugin identities, primary parameter IDs, saved sessions, purchases and trial policy are preserved. Processing outside BOND is unchanged by the interface rollout.

All 50 exact Windows release binaries passed pluginval 1.0.4 at strictness 5, seed 2130, with GUI tests disabled. Their Windows version resources and VST3 module versions independently match the catalogue. Six native check targets passed; native editor checks covered all 50 tools at three sizes, with reviewed captures from the real interfaces. The installer verified 100 embedded hashes and passed selected extraction and replacement while preserving previous bytes and unrelated files. All 51 Windows ZIPs passed integrity checks. The corresponding-source archive includes JUCE 9.0.2 and passed an offline configuration and DSP-test build.

The [universal Mac build](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36679329788) passed checks on Intel and Apple Silicon. The [signed release workflow](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36683669264) then revalidated all 100 signed VST3 / AU bundles on both architectures. Apple accepted the installer for notarization, its ticket is stapled, and Gatekeeper assessment passed. Apple's installer exposed all 100 choices and installed only the four selected CRUSH/REEL bundles with matching hashes. Thirty-one storefront/API tests and eight release-metadata tests pass. See [Mac release evidence](macOS.md) for publication status; refreshing the existing Polar file attachments remains pending.

[Download the signed Mac 0.3.0 installer](https://hungryghostaudio.com/downloads/macos/0.3.0/2138d85a37b0c36b49334ee2a6c55e37f87a7ca46625dc7e2a19cba263b0e24d/HungryGhostSuite-0.3.0-macOS-Universal.pkg). The final installer is 2,615,474,937 bytes with SHA-256 `2138d85a37b0c36b49334ee2a6c55e37f87a7ca46625dc7e2a19cba263b0e24d`, verified locally and by reading back the complete remote object. Apple notarization `cf100466-6800-4b8b-b966-53312e5eb121` returned Accepted. Live checks confirmed both current and previous Mac downloads, including exact range responses above 2 GiB.

Close the audio host before installing. Existing purchases cover the update; individual effects remain $5.99 USD and the complete suite remains $74.99 USD. These checks do not establish every host workflow; a fresh installed 0.3.0 REAPER session remains a user-side check.

## BOND Precision

- Dedicated scalable metal interface with calibrated dials, independent left/right reduction meters, bus peak readings and separate compression, detector and output sections.
- Hungry Ghost finish with blackened metal, restrained edge wear, etched spectral artwork and recessed controls. The character pass retains the Precision preview's processing and parameter behaviour.
- Precision engine adds independent channel detection, variable stereo linking, Peak/RMS detection, feed-forward/feedback modes, knee, maximum reduction, detector low-pass and programme-dependent release.
- Three complete starting presets: Mix glue, Open drum bus and Parallel weight.
- Earlier sessions retain Original processing in both A/B banks. Existing primary automation parameters and plugin identity are preserved.
- Windows x64 VST3 and universal macOS VST3 / Audio Units are included in the suite release pipeline. The standalone remains a developer preview target.

See [BondPrecision.md](BondPrecision.md) for control behaviour and compatibility.

# Suite 0.2.0

- Fix controls missing on first editor open; checks now run before any resize.
- Individual layouts for compression, EQ, distortion, delay, modulation, stereo and utility tools, with real measured meters and scopes.
- Original Blender-rendered REEL spools, including fixed-light rotation frames and editable source scene.
- Factory and two musical starting presets for the 48 newer tools.
- Host tempo synchronisation, note divisions and fallback tempo for delay and rhythmic modulation. Key filtering/audition, pre-drive and repeat low cuts, EQ widths and mono audition where relevant.
- Preserve previous free-time controls and load legacy sessions with neutral advanced defaults.
- HG branding for the installer, website and Polar; individual purchases $5.99 USD and the suite $74.99 USD, before checkout tax rules.

See [InterfaceUpdate.md](InterfaceUpdate.md) for the control and display audit. Validation records and package hashes accompany the release.

# Suite 0.1.0

Initial Windows x64 VST3 release with 50 effects. REVERB updates to 0.3.0 and FERAL to 0.2.0 while keeping their previous plugin identities and saved-state layouts.

- Shared metal interfaces with actual input/output analysis, editable readouts and A/B settings.
- Six nonlinear tools use oversampling with host-reported latency and matched bypass.
- Peak, fast peak, RMS, bus and parallel compressor variants use different detector behaviours.
- Signed device entitlements, a shared 30-day trial and a 90-day offline paid cache. Licence work runs outside the audio callback.
- Windows installer verifies every embedded and installed plugin file, supports individual selection and backs up replacements.
- Complete AGPLv3 corresponding source includes JUCE 9.0.2, editable Blender scenes, interface assets, installer, tests and storefront.

All 50 exact release binaries passed pluginval 1.0.4 at strictness 5, seed 2130, with GUI tests disabled. Native checks separately render each editor at three sizes. Four native check targets and thirteen store API tests passed. Processor checks cover six sample rates, mono/stereo, large and empty blocks, non-finite input recovery, bypass latency, external sidechain behaviour and a coherent high-frequency alias measurement.

The installer passed payload verification, individual extraction, replacement and restoration after a blocked copy. Windows plugins include the C++ runtime statically. Published binary hashes appear in `manifest.json`; package hashes appear in `SHA256SUMS.txt`.

This initial release has no Windows code-signing certificate and no macOS, Linux, AU, AAX or VST2 binaries. GUARD is a sample-peak limiter. New delay and modulation tools use millisecond/Hz controls rather than host tempo synchronisation. Compatibility outside the stated platform and the tests above is not claimed.

Trial downloads can be published before purchasing opens. A product only becomes purchasable after its Polar licence entitlement, checkout and delivery are configured and the merchant can accept payment.
