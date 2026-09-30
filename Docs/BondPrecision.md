# BOND Precision — 0.3.0

BOND is a stereo bus compressor with a dedicated control surface and two processing models. New instances use Precision. Sessions saved before this update open in Original in both A/B banks; the existing parameter IDs, ranges and VST3 identity are retained.

## Compression

Threshold and ratio establish the amount of compression. Attack controls transient preservation; release controls recovery. Precision adds a 0–24 dB soft knee and a 0–60 dB maximum reduction range. Range limits compression, not output peaks: BOND is not a brickwall limiter.

Auto Release adapts recovery to recent compression. The Release setting remains the base time; short events recover faster and sustained reduction recovers more slowly. Compare with Auto off when setting the groove.

## Detector

Peak responds to transient amplitude. RMS follows signal energy with a 15 ms averaging time. Feed-forward detects the input. Feedback detects the compressed internal signal before makeup, mix and output gain. A connected external sidechain uses feed-forward detection even when Feedback is selected.

The high-pass and low-pass filters affect the detector, not the full-band audio. Precision uses 12 dB/octave Butterworth sections. The low-pass remains a filter at its maximum 20 kHz setting; it is not an Off switch. Listen to Key auditions the signal driving the detector. Turn it off to return to the processed programme.

## Stereo and output

At 0% Stereo Link, left and right compress independently. At 100%, the stronger detector level drives both channels. Intermediate settings blend towards linked detection. The two reduction meters show the applied gain reduction before makeup and the dry/wet blend; bus input/output readings show the strongest channel peak in dBFS. Meter bars cover 0–24 dB; numeric readings continue beyond that.

Makeup adjusts the compressed path, Dry/Wet blends it with the original, and Output trims the final result. No automatic loudness compensation is applied. Level-match with Output when evaluating a setting.

## Starting points

- **Mix glue:** gentle ratio, slower attack, RMS feedback detection, full link, limited reduction and automatic release.
- **Open drum bus:** peak feed-forward detection, slower attack and partial stereo linking.
- **Parallel weight:** stronger RMS compression with a 50% dry/wet blend.

Threshold depends on the source level. Start with a few decibels of reduction, then adjust by ear. Presets leave every control editable and automatable.

## Interface and compatibility

The native JUCE editor uses vector controls, calibrated scales and editable numeric fields. It resizes from 85% to 150% with high-density rendering. Double-click a control to restore its parameter default. A/B, Copy, Bypass and Factory retain their existing functions. Factory resets to Precision; Original disables controls that do not affect its retained processing model.

The refined controls use recessed metal push switches with state lamps, inset selector windows, matching popup menus and a machined stereo-link fader. Enabled dials and the fader accept keyboard focus and arrow-key adjustments; controls expose descriptive accessibility titles.

The Hungry Ghost character pass adds blackened metal, restrained edge wear and an etched spectral motif. Functional legends and numeric readouts stay clean; illuminated indicators still represent actual control states and measured levels. This pass changes the appearance only.

BOND is included in the suite's Windows x64 VST3 and universal macOS VST3 / Audio Unit packages. Release validation, signing and publication status is recorded in [the release notes](ReleaseNotes.md); the Windows standalone is a developer preview target. Existing licensing and trial behaviour applies. Close the audio host before replacing a plugin and retain the previous version for comparison.
