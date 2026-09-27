# Hungry Ghost Audio launch

Updated 28 September 2026. This file records verified release evidence and remaining launch requirements.

## Published and verified

- The [0.1.0 release](https://github.com/HungryGhostStudios/hungry-ghost-audio/releases/tag/v0.1.0) includes 50 Windows x64 VST3 effects, a selectable installer, individual downloads and complete AGPLv3 corresponding source with JUCE 9.0.2.
- All 50 exact release binaries passed pluginval 1.0.4 at strictness 5, seed 2130, with GUI tests disabled. Native editor checks separately render all 50 interfaces at three sizes.
- Four native check targets passed. Processor checks cover six sample rates from 22.05 to 384 kHz, mono/stereo, empty and large blocks, saved state, non-finite input recovery, bypass latency and external sidechain behaviour. DSP checks include compressor ratios, filter rejection, delay timing and an oversampling alias measurement.
- The installer passed payload verification, individual selection, replacement backup and rollback after a blocked copy. The installed binaries in REAPER's configured VST3 scan folder match all 50 release-manifest hashes. Installed REVERB, FERAL and RIFT also passed pluginval at strictness 5. A listening session in REAPER has not been completed.
- All 55 published release assets have GitHub SHA256 digests matching local files. Complete-source ZIP integrity and an offline build using its bundled JUCE source were checked.
- [hungryghostaudio.com](https://hungryghostaudio.com) serves the Cloudflare storefront, 50 native plugin screenshots and verified download links. The mobile 390-pixel layout has no horizontal overflow or broken images. The www hostname redirects to the canonical domain, preserving paths and queries.
- Thirteen storefront/API tests passed, including checkout destination checks, purchase gating, standard RSA SHA-256 interoperability, device binding, entitlement expiry and provider failure handling.
- Polar has 50 individual products and a complete-suite product, with 51 checkout links and separate perpetual licence benefits. Each benefit allows two activations and customer-controlled device deactivation. The production signing service rejects invalid keys through Polar.

The [full CI run for the release integration](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36357462119) passed. The two subsequent runs covering the www redirect and its routing configuration were still building at the time of this update: [redirect](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36358395433), [routing](https://github.com/HungryGhostStudios/hungry-ghost-audio/actions/runs/36358593840). Local storefront checks and deployed redirect checks passed after those changes.

## Pricing

Launch prices are $15–$99 for individual effects and $299 for the complete suite. REVERB is $25, compared with [Valhalla VintageVerb's $50 list price](https://valhalladsp.com/shop/reverb/valhalla-vintage-verb/). FERAL is $99, compared with [FabFilter Pro-Q 4's $199 list price](https://www.fabfilter.com/shop/pro-q-4-equalizer-plug-in?currency=USD). The same FabFilter shop currently lists its Total Bundle at $1,069. These are price-positioning references checked on 28 September 2026; they are not claims of identical features, platform support or sound quality. Sales, account discounts and taxes can change the comparison.

## Required before paid purchasing opens

- The merchant must connect their own payout account and provide a support email at [Polar's account-review page](https://polar.sh/dashboard/hungry-ghost-audio/finance/account). Submit for review is disabled while these are missing. Polar may require review time after submission.
- Confirm that real checkout accepts payment, then verify an order, customer licence delivery, two-device activation, customer deactivation, signed native activation and refresh against the real purchased key.
- Enable each product's purchase gate only after those checks succeed. The live configuration intentionally remains `ready:false`.

Paid purchasing and real customer key delivery are not yet verified. This is a published trial/source release, not a completed paid launch. The signed offline cache is an entitlement check; the AGPL licence preserves users' rights to inspect, modify and rebuild the software, and no claim of uncrackable protection is made.
