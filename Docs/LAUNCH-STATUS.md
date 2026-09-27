# Hungry Ghost Audio launch

This file records evidence, not marketing promises. The requested launch comprises 50 functioning processors, a Cloudflare storefront at hungryghostaudio.com, Polar checkout and delivery, and signed device licences.

## Verified before suite work

- Reverb 0.2.0: built, DSP and integration tests passed, pluginval level 5 passed; installed in REAPER.
- FERAL 0.1.1: built, DSP and integration tests passed, pluginval level 5 passed; installed in REAPER.

## Verified suite progress

- 48 additional DSP engines pass stereo/mono processing at 22.05, 44.1, 48, 96 and 192 kHz, parameter extremes and host block-size independence.
- Gain, DC rejection, notch rejection, compressor ratio, sample-peak ceiling and delay timing checks pass.
- All 48 new native processors pass saved state, A/B switching, 8193-sample blocks and editor rendering at three sizes.
- Worker API tests: 5 passed, including checkout allowlisting, cross-origin rejection and standard RSA signature interoperability.
- Cloudflare staging deployment responds HTTP 200 at https://hungry-ghost-audio.nikrich.workers.dev. Purchasing is closed.
- Complete development source published at https://github.com/HungryGhostStudios/hungry-ghost-audio under AGPLv3.

## In progress

- Build and host validation for all 50 VST3 packages; sonic evaluation and interface inspection.
- Production storefront, checkout, downloads and licence activation.
- Suite-wide testing and release packaging.

No full-suite release or live purchasing is verified yet.
