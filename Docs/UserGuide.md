# Hungry Ghost Audio · Windows and macOS

Suite release 0.2.0. Windows 10 or 11 uses x64 VST3. macOS 11 and later uses universal VST3 and Audio Units on Intel and Apple Silicon. Choose Audio Units for Logic Pro and VST3 for REAPER or other compatible hosts. REVERB is version 0.3.0; FERAL is 0.2.0; the remaining effects are 0.2.0. The same purchase covers both platforms.

## Install on Windows

1. Close your audio host. The installer never closes it for you.
2. Run `HungryGhostSuite-0.2.0-Setup.exe`, select your effects and choose Install. The default is your standard user VST3 folder: `%LOCALAPPDATA%\Programs\Common\VST3`. Installing there does not require administrator access.
3. Open your host and rescan that folder. In REAPER, open Preferences → Plug-ins → VST, add the folder to the scan paths if needed, and use Re-scan.

The ZIP downloads provide the same tested binaries for manual installation. Copy the folders **inside** `VST3` to your user VST3 folder, or to `C:\Program Files\Common Files\VST3` with administrator access. Copy each whole `.vst3` folder, including Contents; do not extract just the inner binary. Choose either the user location or shared location for an installation to avoid duplicate versions.

The installer verifies its embedded payload and installed files. If replacing files, it saves backups in `%LOCALAPPDATA%\HungryGhostAudio\InstallBackups`; if copying fails, it restores the files it changed. Close the host before restoring a backup. To uninstall, remove the corresponding Hungry Ghost `.vst3` folders from the location you installed to. Other vendors' plugins, host configuration, projects and presets stay outside this process.

These initial releases are not signed with a Windows code-signing certificate. Release SHA-256 checksums identify the published packages; licence signatures verify purchase entitlements inside the plugin. They serve different purposes.

## Install on Mac

1. Close your audio hosts and download `HungryGhostSuite-0.2.0-macOS-Universal.pkg` from the website's Mac download.
2. Open the package in Apple's Installer. Use Customize to select the effects and VST3 / Audio Unit formats you want. Administrator approval is required for the shared plugin folders.
3. Complete installation, reopen your audio host and rescan plugins if needed.

VST3 bundles install to `/Library/Audio/Plug-Ins/VST3`; Audio Units install to `/Library/Audio/Plug-Ins/Components`. Keep the whole `.vst3` or `.component` bundle intact. To uninstall an effect, close your hosts and remove only its Hungry Ghost bundles from those locations. Existing projects, presets and licence caches are outside the installer payload.

The public Mac installer is Developer ID signed, notarized by Apple and stapled. No security-bypass commands are needed. Its published SHA-256 identifies the exact installer. Every bundle contains native Intel and Apple Silicon code; the release validators checked both architectures. See [Mac release evidence](macOS.md) for the tested scope.

## Trial and activation

The suite starts a shared 30-day trial on its first launch. No account or audio upload is needed for the trial. When it ends, unlicensed effects pass audio through; they do not insert noise or mute your project. Trial status is assessed when you open a plugin instance, so a running session is not interrupted midway through a take.

After purchasing, sign in to [Polar's customer portal](https://polar.sh/hungry-ghost-audio/portal) using your checkout email and copy the `HG_` licence key. The Windows installer is in the File Downloads benefit; platform downloads are also offered at [Downloads](https://hungryghostaudio.com/#downloads). Click the activation button at the bottom of a plugin, paste the key and activate. A suite key covers all 50 tools. An individual key covers that tool. Activating on a device stores a shared signed record, so you can open your other purchased plugins on that device. A Windows computer and a Mac count as separate devices toward the two-device allowance.

Each purchase permits two device activations. Use Manage devices to deactivate an old computer before moving to another. Paid activations work offline for up to 90 days. The plugin refreshes an older cache in the background when the service is available; an offline session retains a still-valid cache. After its expiry, reconnect and use Activate / refresh.

Activation sends your key, activation ID and a hashed device identifier over HTTPS. No audio, preset or project contents are sent. Keys remain in your local user licence cache; do not post them in public bug reports. Keep your system clock correct.

The source is available under AGPLv3. You may inspect, modify, build and redistribute it under those terms. The store licence service checks our packaged releases; it is not a claim that open-source software cannot be changed or rebuilt.

## Common controls

- **Dry / wet** blends the effect with the input. Output adjusts final level. Compare at similar loudness before deciding an effect sounds better.
- **A / B** stores two independent settings for comparison. Copy transfers the current settings to the other bank. Factory returns controls to their defaults.
- **Bypass** preserves the reported plugin latency. Saturation tools and EDGE use 4× oversampling up to 96 kHz, 2× at 192 kHz, and no additional oversampling above that. Hosts compensate the reported delay.
- **Displays** fit each processor: waveforms, spectra, peak and reduction meters, and stereo scopes show measured audio. Response, transfer, LFO shape and repeat timing show settings. See [the interface and control guide](InterfaceUpdate.md).
- **Shape +** opens deeper settings where relevant. Sync follows host tempo, with a fallback if unavailable; free time/rate is retained when sync is disabled. Key audition plays the detector signal. Mono audition plays the stereo sum.
- Drag knobs, use the mouse wheel or enter values in their readouts. Double-click a knob to restore its default. Frequency entry accepts Hz or kHz; dry/wet entry uses percent.

## REVERB

Choose Room, Chamber, Hall, Plate or Cloud, then set decay, size, tone, motion, pre-delay and mix. The tail display shows the current decay shape and measured tail information. Its axis changes on fixed ranges as decay grows. Shape opens extra damping, diffusion, early/late, stereo, ducking and filtering controls. Freeze sustains the current tail; Mix lock preserves the dry/wet setting when changing presets. For an auxiliary send, use 100% wet and control level from the send.

## FERAL

Double-click the graph to add a band. Drag its node to adjust frequency and gain. The node's wheel changes cut frequency on cut bands and Q on the other shapes; Shift-wheel adjusts Q. Select the band to change its filter shape, stereo/mid/side processing and dynamics. Dynamic mode adds threshold, ratio, attack, release, knee and maximum range. Use Ext key with a routed external sidechain. Bus opens the broadband compressor. FERAL supports up to eight bands.

## Dynamics

RIFT provides clean peak compression. CLAW adds a fast peak detector and programme-dependent release. VELVET uses a slower RMS detector for gentler levelling. BOND filters the detector's bass energy before bus compression. CRUSH starts with parallel blending for stronger compression alongside the original signal. Threshold determines where reduction starts; ratio, knee, attack and release shape its response. Makeup is separate from final Output.

HUSH detects the selected sibilance region and applies bounded reduction. GATE uses threshold, hysteresis, hold and floor; BLOOM applies gentler downward expansion. PUNCH shapes fast attacks and slower body; TAIL focuses on sustain while protecting attack. RIDE slowly changes level within its set range and ignores signals below its noise floor. DUCK accepts an external sidechain to reduce the main signal; without a routed key it uses the main signal as its detector.

GUARD is a linked **sample-peak** limiter, not a true-peak mastering limiter. EDGE offers oversampled clipping with adjustable softness. Ceiling is inside the effect before final Output and dry/wet blending. Raising final Output or blending an unclipped dry signal can exceed that setting; check your final output meter.

## Tone

SHADE has three fixed-width bell bands for broad shaping. TILT pivots warmth and brightness together. WEIGHT and AIR provide low and high shelving. VOID rejects a selected region; CROSS passes a region around its centre. CONTOUR combines high-pass and low-pass cuts. FOCUS independently shapes mid and side energy with a bell. Shelving resonance and filter Q change the shape; use moderate settings before making narrow boosts.

## Colour

EMBER adds symmetric soft saturation. IRON introduces asymmetry with bias. FURNACE runs two soft nonlinear stages with adjustable character. FOLD folds peaks into more complex harmonics. RECT blends rectification and bias. These effects use oversampling as described above. SHARDS reduces bit resolution and can add dither; STATIC reduces effective sampling rate with a smoothing filter. Their deliberate quantisation and sampling artifacts are part of their effects.

## Space and motion

ECHO provides clean delay, REEL adds drifting and saturated repeats, PING crosses repeats between channels, SLAP uses short offset stereo reflections, DUB uses stronger saturated feedback, and TEETH uses short delays for pitched comb filtering. Negative feedback changes repeat polarity. High feedback can sustain a long tail. Tone filters the repeats. For an auxiliary send, set Dry / wet to 100%.

CHOIR uses two modulated delay voices; FLUX uses a shorter moving delay with feedback; PHASE uses six moving all-pass stages. PULSE changes amplitude; ORBIT moves stereo balance; RING multiplies audio by an oscillator for metallic sidebands; DRIFT modulates a fractional delay for pitch movement. Rate determines movement speed. Depth and feedback can produce large changes, so watch output level.

## Stereo and utility

WIDE adjusts side energy with bass mono filtering. CENTER collapses low-frequency side information. SHADOW delays one channel for Haas widening: check mono compatibility because delay-based widening can cause cancellation when summed. TRIM adjusts gain and balance. REVERSE independently flips left/right polarity. CLEAN removes DC and subsonic energy with an adjustable high-pass cutoff.

## Source, downloads and help

[Releases and checksums](https://github.com/HungryGhostStudios/hungry-ghost-audio/releases) · [Full source](https://github.com/HungryGhostStudios/hungry-ghost-audio) · [Bug reports](https://github.com/HungryGhostStudios/hungry-ghost-audio/issues)

Include your plugin version, host version, sample rate and steps to reproduce a bug. For purchase or activation questions, use the contact details in your Polar receipt. Do not publish a licence key, purchase email or private project.
