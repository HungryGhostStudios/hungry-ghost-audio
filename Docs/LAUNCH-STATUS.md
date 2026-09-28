# Hungry Ghost Audio launch

Updated 28 September 2026. This file records verified release evidence and remaining launch requirements.

## Published and verified

- The [0.2.0 release](https://github.com/HungryGhostStudios/hungry-ghost-audio/releases/tag/v0.2.0) includes 50 Windows x64 VST3 effects, a selectable installer, individual downloads and complete AGPLv3 corresponding source with JUCE 9.0.2.
- The [signed Mac release](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36408294229) validated all 100 universal VST3 / AU bundles on Intel and Apple Silicon. Apple notarization returned Accepted; the installer was stapled and passed selectable-installation checks. The exact 2,604,372,347-byte installer was uploaded to private R2 storage and its complete remote SHA-256 matched the release manifest. The live website exposes its verified download, with matching HEAD and range reads at the beginning, end and above 2 GiB. See [Mac release details](macOS.md). A customer Mac session in Logic Pro or REAPER remains a host-level follow-up.
- All 50 exact release binaries passed pluginval 1.0.4 at strictness 5, seed 2130, with GUI tests disabled. Native editor checks verify controls on first open, render all 50 interfaces at three sizes, and exercise advanced slider attachments.
- Four native check targets passed. Processor checks cover six sample rates from 22.05 to 384 kHz, mono/stereo, empty and large blocks, saved state, non-finite input recovery, bypass latency and external sidechain behaviour. DSP checks include compressor ratios, filter rejection, delay timing and an oversampling alias measurement.
- The 0.2.0 installer passed all 100 embedded file hashes and selected-bundle extraction. Replacement backup and blocked-copy rollback were verified in the previous installer release; that installation logic is unchanged. The user will install 0.2.0 from the website. The first-open missing-controls bug in 0.1.0 is fixed in this update. An installed 0.2.0 REAPER session has not been verified.
- All 55 published release assets have GitHub SHA256 digests matching local files. Complete-source ZIP integrity and an offline build using its bundled JUCE source were checked.
- [hungryghostaudio.com](https://hungryghostaudio.com) serves the Cloudflare storefront, 50 native plugin screenshots and verified download links. The current desktop storefront has no horizontal overflow or broken images. The 390-pixel mobile layout was checked in the earlier release; responsive CSS is unchanged in this update. The www hostname redirects to the canonical domain, preserving paths and queries.
- Fourteen storefront/API tests passed, including all 51 configured checkouts and licence coverage, checkout destination checks, purchase gating, standard RSA SHA-256 interoperability, device binding, entitlement expiry and provider failure handling.
- The current 23 storefront/API tests passed, including immutable Mac delivery, large range offsets and private multipart upload guards. All 51 checkout destinations and existing licence coverage remain configured when Mac downloads are added.
- Polar has 50 individual products and a complete-suite product, with 51 checkout links and separate perpetual licence benefits. Each benefit allows two activations and customer-controlled device deactivation. The production signing service rejects invalid keys through Polar.

The [full Windows CI build for 0.2.0](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36391678570) passed all 50 plugin builds, the four native check targets and storefront/API checks. Local checks additionally cover exact packaged binary hashes and first-open editor rendering.

The storefront includes six dry/processed audio previews rendered through the exact released VST3 binaries. Their original synth/drum sources and settings are reproducible with `scripts/render_audio_demos.py`. Active-region RMS matching keeps the comparison from favouring the louder version. The renderer checks for finite output, real processing differences, matching durations and no clipping after MP3 decoding. These previews are examples, not a comprehensive sonic evaluation.

## Pricing

All 50 individual effects cost $5.99 USD; the complete suite costs $74.99 USD. These are one-time prices. Polar applies the configured tax rules at checkout. Live REVERB and suite checkouts were checked at the new prices.

## Purchasing and remaining verification

- Polar's account-review page now shows **Account approved** and **Identity verified**. The user reported completing payout setup. The REVERB and complete-suite checkout links display live payment forms at $5.99 and $74.99, without the previous payments-unavailable message.
- The website's 50 individual purchase gates and complete-suite gate are enabled. Checkout links and licence benefit mappings are configured for all 51 products.
- The user completed a $0 order using their discount. Polar shows it as Paid and issued a perpetual suite key. The installer file benefit was added retrospectively and shows Granted for that order. Two-device activation, customer deactivation, signed native activation/refresh against this key, and a nonzero card payment remain unverified. No payment was made by the agent.

The storefront is connected to live Polar checkout. The zero-value customer order verifies key and download-grant delivery; it does not verify a charged card transaction. The signed offline cache is an entitlement check; the AGPL licence preserves users' rights to inspect, modify and rebuild the software, and no claim of uncrackable protection is made.
