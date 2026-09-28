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
