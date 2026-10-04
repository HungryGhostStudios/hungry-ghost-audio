# HAUNT — vocal pitch correction preview

HAUNT is a separate, optional JUCE effect for one vocal line. The first stage is real-time correction; recorded-note editing is the next stage. It is not part of the released 50-product catalogue, installer, licence service or store. The current Windows preview is 0.1.2. The VST3 identity and parameter/state layout are unchanged from 0.1.0. Do not advertise this prototype as a finished competitor or as a low-latency tracking product.

## Working controls

- Chromatic, major, minor, major pentatonic and custom note sets; twelve clickable allowed-note keys.
- Retune 0–250 ms, correction strength, sustained-note humanize and vibrato preservation.
- Independent formant shift, pitch transposition and formant compensation.
- MIDI note targets with exact octaves, per-channel note ownership and sustain pedal. With MIDI targeting enabled and no notes held, automatic correction relaxes to zero; manual transposition remains active.
- Wide, low, mid and high detection ranges; concert A adjustable from 420 to 460 Hz.
- Latency-aligned dry/wet, smoothed output level, unity-gain bypass, A/B recall and copy.
- Natural, Hard Tune and Modern starting points. These change the four correction/expression controls; they do not reset the key, scale, mix, formant or transpose settings.
- A resizable native editor with a six-second input-pitch trace and target line. The target line is the requested note, **not an independently measured output-pitch trace**. Confidence describes detector periodicity, not a probability that the singer is correct.

The amount of humanize matters only when retune time is above zero. Vibrato preservation preserves fast movement around an adaptive pitch centre. Version 0.1.2 resets that centre at target changes and follows arriving notes faster before settling into sustained-vibrato handling. Expressive settings require adjacent target candidates to persist (longer near the boundary); this avoids many vibrato-induced note switches without permanently trapping slightly detuned notes. Hard Tune at zero retune time and MIDI targets skip this added confirmation time. This is a musical heuristic, not a complete phrase or intention classifier.

Brief missed detections retain the previous correction for up to 25 ms before releasing it. Longer unvoiced sections release correction; MIDI note release and empty scales still remove the target. The audio continues through the same shifter, avoiding dry/wet switching at consonants. Dedicated consonant recognition and protection remain future work.

## Audio architecture

`Source/Haunt/PitchEngine` is independent of JUCE. It uses a decimated, low-pass-filtered difference-function pitch detector, fractional-period refinement, allowed-note hysteresis and a fixed 512-sample processing quantum. The two audio channels retain their stereo relationship. Detection follows a stable stronger channel rather than summing potentially phase-opposed inputs.

Version 0.1.1 replaces the previous custom spectral mapping with the unmodified Rubber Band 4.0.0 LiveShifter and its formant processing. The pinned source and GPL-2.0-or-later licence are in `ThirdParty/rubberband`. The build uses the upstream single-file compilation unit, built-in FFT/BQ resampler, and linked-channel processing. HAUNT supplies its own detector, targeting, expression controls, audio scheduling and native interface. This change addresses an observed harmonic-integrity defect in our original configuration; it is not a claim that swapping libraries alone solves natural vocal tuning.

Preparation allocates delay and spectral buffers. Audio processing uses fixed detector storage, cached parameter atomics and preallocated sample buffers. Long, unrelated MIDI SysEx messages are skipped without copying their payload. GUI history and state-bank operations stay outside the audio callback.

The reported delay includes the shifter's start delay and the quantum queue. At 48 kHz the current preview reports 2,624 samples (54.67 ms), before the audio interface and host buffers. This is slightly longer than 0.1.0. The shifter is primed at unity outside the processing callback. With correction, transpose and formant shift all disabled, audio follows the original delayed path directly. Detection still needs a voiced onset before it can select a target. This is a quality-development baseline, not the final live-monitoring latency target.

## Build and checks

```powershell
cmake -S . -B build -DHG_BUILD_HAUNT_PREVIEW=ON
cmake --build build --config Release --target HAUNT_VST3 HAUNT_Standalone hg_haunt_dsp_tests hg_haunt_tests --parallel 4
ctest --test-dir build -C Release -R "^hg_haunt_" --output-on-failure
```

Use the repository's pinned JUCE 9.0.2, or pass `JUCE_SOURCE_DIR`. The option defaults to OFF so normal suite releases are unaffected. `HG_BUILD_PLUGINS=OFF` still permits the independent DSP checks. On Apple the optional target also declares AU, but this preview has not yet been built, signed, notarized or validated on macOS.

DSP checks measure the actual corrected waveform independently of the displayed pitch. They cover harmonic tracking at 44.1/48/96 kHz, positive and negative detuning, fixed transposition, formant compensation, exact bypass/neutral latency, MIDI/sustain, stereo phase opposition, host block-size invariance, noise/silence, invalid samples/settings and C++ heap allocations during processing. Native integration checks exercise parameter attachments, keyboard notes, saved A/B states, host bypass, MIDI delivery and editor captures at three sizes. Synthetic signals establish those properties only; they do not establish natural-vocal quality.

The 0.1.2 phrasing regressions add ascending/descending steps and 200-ms glides at 44.1/48/96 kHz, in natural and hard tuning settings. They measure actual output pitch after settling. At 48 kHz, the natural rising-glide error fell from 51.75 cents in the accepted 0.1.1 baseline to about 7.1 cents. On a four-second synthetic phrase with a 25-cent sharp centre and 38-cent, 6-Hz vibrato, target switches after the initial half-second fell from 42 to zero. The corrected output retains about 37 cents of vibrato depth, with its centre within 0.4 cents at all three rates. These figures describe these particular test signals, not all voices. Tests also verify scale changes, empty scales, and correction hold/release on detector dropout. Run these with `hg_haunt_dsp_tests --phrasing-only`; `--phrasing-report` prints metrics without enforcing thresholds for historical comparisons.

The new harmonic-integrity regression uses a sustained 60-harmonic vowel-like signal, a fixed 32-cent downward shift, and an independent projection of the final two seconds onto the expected 220-Hz harmonic series. Unwanted residual must be below -35 dB, with formant compensation both off and on. The 0.1.0 engine fails at -6.32 dB; the replacement passes at -61.89 dB and -54.02 dB respectively. This check detects artifacts the original sine-wave tests missed. Run just this regression with `hg_haunt_dsp_tests --quality-only`.

The 24-case output grid (73.42–1046.50 Hz, ±32 cents, formant compensation on/off, 48 kHz) stayed within 0.75 cents of its targets. The disabled-correction path nulled exactly against the aligned input. Six seconds of stereo processing with a formant shift took approximately 0.60 seconds on this development machine; no C++ heap allocations were observed in that processing loop. These are a synthetic baseline and a local timing sample, not a cross-machine performance claim. Monitoring latency still needs substantial improvement.

Two attributed LibriSpeech spoken-voice clips and a short noise burst were also used for local rendering comparisons. These checks isolate a small fixed pitch shift and do not validate singing, automatic note transitions or the user's reported example. The exact vocal recording and a listening comparison remain necessary before claiming the sound is fixed. Source clips were obtained from librosa's official example collection; their CC-BY-4.0 notices accompany any delivered comparisons.

The 0.1.1 Windows VST3 passed pluginval strictness 5 (seed 2130, GUI tests disabled) and native controls/state/MIDI/bypass checks. The exact validated and installed binary has SHA-256 `1908a71512f016a7a80e003be2f3ef99b077740ccd18fe81483af243e093efbc`; both PE and VST3 versions are 0.1.1. The preview package includes its logs and checksum manifest. macOS remains unvalidated for HAUNT.

The 0.1.2 Windows VST3 also passes pluginval strictness 5 with the same settings, the expanded DSP regressions and native integration checks. Its validated SHA-256 is `7b0ded3b09768fe4ceef984242ff9108e1322ad1b721a1833f3e2c67c9ac2fd4`, with matching PE/VST3 version 0.1.2. Harmonic-integrity results and the 2,624-sample delay are unchanged; no C++ processing allocations were observed. The 0.1.1 package is retained as the listening-comparison baseline.

Upstream reference: [Rubber Band LiveShifter API](https://breakfastquay.com/rubberband/code-doc/classRubberBand_1_1RubberBandLiveShifter.html). Benchmark voice metadata: [librosa example recordings](https://librosa.org/doc/main/recordings.html).

## Next stage: recording and detailed note editing

The same plugin should offer Live and Edit views. Live keeps key/scale, retune and expressive controls immediately available. Edit adds an actual captured phrase with detected note segments, input/output traces and editing at the note level. No nonfunctional Edit button is included in this preview.

1. Establish a repeatable, permission-cleared vocal evaluation set: low/high voices, breath, sibilance, glides, rasp, vibrato, note boundaries and backing spill. Compare level-matched renders blindly and retain regressions.
2. Reduce tracking latency and processing cost while keeping pitch accuracy, stereo coherence and consonant quality. Measure on realistic hardware and buffers; a short synthetic benchmark is not a dropout guarantee.
3. Add captured-audio and pitch-event storage outside the callback, a host-time model, note segmentation, undoable edits and deterministic offline rendering. Each note needs independent pitch centre, drift, vibrato, transition and formant overrides.
4. Add safe phrase capture/import, looping, audition and scale-aware editing. Preserve the live preset when entering Edit. Define how edited phrases follow transport, seeks, tempo and project recall before adding ARA integration.
5. Validate real host sessions, automation, render/export, reopen and platform installers before adding HAUNT to the paid catalogue. No new price or bundle entitlement has been published.

The practical quality bar is convincing vocals and predictable musical control. Synthetic pitch tolerances alone are not sufficient for release.
