# HAUNT — vocal pitch correction preview

HAUNT is a separate, optional JUCE effect for one vocal line. The first stage is real-time correction; recorded-note editing is the next stage. It is not part of the released 50-product catalogue, installer, licence service or store. The current Windows preview is 0.1.0, with its own VST3 identity. Do not advertise this prototype as a finished competitor or as a low-latency tracking product.

## Working controls

- Chromatic, major, minor, major pentatonic and custom note sets; twelve clickable allowed-note keys.
- Retune 0–250 ms, correction strength, sustained-note humanize and vibrato preservation.
- Independent formant shift, pitch transposition and formant compensation.
- MIDI note targets with exact octaves, per-channel note ownership and sustain pedal. With MIDI targeting enabled and no notes held, automatic correction relaxes to zero; manual transposition remains active.
- Wide, low, mid and high detection ranges; concert A adjustable from 420 to 460 Hz.
- Latency-aligned dry/wet, smoothed output level, unity-gain bypass, A/B recall and copy.
- Natural, Hard Tune and Modern starting points. These change the four correction/expression controls; they do not reset the key, scale, mix, formant or transpose settings.
- A resizable native editor with a six-second input-pitch trace and target line. The target line is the requested note, **not an independently measured output-pitch trace**. Confidence describes detector periodicity, not a probability that the singer is correct.

The amount of humanize matters only when retune time is above zero. Vibrato preservation currently preserves fast movement around a slowly changing pitch estimate; it does not yet distinguish intentional scoops, vibrato and note transitions as separate musical events.

## Audio architecture

`Source/Haunt/PitchEngine` is independent of JUCE. It uses a decimated, low-pass-filtered difference-function pitch detector, fractional-period refinement, allowed-note hysteresis and a fixed 64-sample processing quantum. The two audio channels retain their stereo relationship. Detection follows a stable stronger channel rather than summing potentially phase-opposed inputs.

The spectral shifter and formant processor use the unmodified, MIT-licensed Signalsmith Stretch 1.3.2 and Signalsmith Linear headers. Exact upstream commits and original licences are in `ThirdParty/signalsmith-stretch`. HAUNT supplies its own detector, targeting, expression controls, audio scheduling and native interface. This is not Signalsmith's separate commercial Compose engine.

Preparation allocates delay and spectral buffers. Audio processing uses fixed detector storage, cached parameter atomics and preallocated sample buffers. Long, unrelated MIDI SysEx messages are skipped without copying their payload. GUI history and state-bank operations stay outside the audio callback.

The reported delay includes spectral input/output latency and the quantum queue. At 48 kHz the current preview reports 2,368 samples (49.33 ms), before the audio interface and host buffers. The 48 ms spectral window and a smooth, monotonic harmonic-aware frequency map improve low-register accuracy. Detection also needs a voiced onset before it can select a target. This is a quality-development baseline, not the final live-monitoring latency target. Shorter windows must pass output-frequency, transient and listening checks before replacing it.

## Build and checks

```powershell
cmake -S . -B build -DHG_BUILD_HAUNT_PREVIEW=ON
cmake --build build --config Release --target HAUNT_VST3 HAUNT_Standalone hg_haunt_dsp_tests hg_haunt_tests --parallel 4
ctest --test-dir build -C Release -R "^hg_haunt_" --output-on-failure
```

Use the repository's pinned JUCE 9.0.2, or pass `JUCE_SOURCE_DIR`. The option defaults to OFF so normal suite releases are unaffected. `HG_BUILD_PLUGINS=OFF` still permits the independent DSP checks. On Apple the optional target also declares AU, but this preview has not yet been built, signed, notarized or validated on macOS.

DSP checks measure the actual corrected waveform independently of the displayed pitch. They cover harmonic tracking at 44.1/48/96 kHz, positive and negative detuning, fixed transposition, formant compensation, exact bypass/neutral latency, MIDI/sustain, stereo phase opposition, host block-size invariance, noise/silence, invalid samples/settings and C++ heap allocations during processing. Native integration checks exercise parameter attachments, keyboard notes, saved A/B states, host bypass, MIDI delivery and editor captures at three sizes. Synthetic signals establish those properties only; they do not establish natural-vocal quality.

On the initial Windows check, the 24-case output grid (73.42–1046.50 Hz, ±32 cents, formant compensation both on and off, 48 kHz) stayed within 1.76 cents of its targets. Neutral wet output nulled against the reported-delay-aligned input at approximately −124.9 dB. Six seconds of stereo processing with a formant shift took approximately 2.35 seconds on this development machine; no C++ heap allocations were observed in that processing loop. These are a synthetic baseline and a local timing sample, not a cross-machine performance claim. CPU cost and monitoring latency both need substantial improvement.

The first Windows VST3 preview passed pluginval strictness 5 (seed 2130, GUI tests disabled), plus native editor/state/parameter/MIDI/bypass checks. PE and VST3 metadata both report 0.1.0. The exact validated VST3 binary has SHA-256 `aa4434a16d6554fdc85206fe52deff4045c99bb31972c0a781b67229df1793c2`. The local preview deliverable includes the validation logs and manifest; this is not a macOS or universal-host certification.

## Next stage: recording and detailed note editing

The same plugin should offer Live and Edit views. Live keeps key/scale, retune and expressive controls immediately available. Edit adds an actual captured phrase with detected note segments, input/output traces and editing at the note level. No nonfunctional Edit button is included in this preview.

1. Establish a repeatable, permission-cleared vocal evaluation set: low/high voices, breath, sibilance, glides, rasp, vibrato, note boundaries and backing spill. Compare level-matched renders blindly and retain regressions.
2. Reduce tracking latency and processing cost while keeping pitch accuracy, stereo coherence and consonant quality. Measure on realistic hardware and buffers; a short synthetic benchmark is not a dropout guarantee.
3. Add captured-audio and pitch-event storage outside the callback, a host-time model, note segmentation, undoable edits and deterministic offline rendering. Each note needs independent pitch centre, drift, vibrato, transition and formant overrides.
4. Add safe phrase capture/import, looping, audition and scale-aware editing. Preserve the live preset when entering Edit. Define how edited phrases follow transport, seeks, tempo and project recall before adding ARA integration.
5. Validate real host sessions, automation, render/export, reopen and platform installers before adding HAUNT to the paid catalogue. No new price or bundle entitlement has been published.

The practical quality bar is convincing vocals and predictable musical control. Synthetic pitch tolerances alone are not sufficient for release.
