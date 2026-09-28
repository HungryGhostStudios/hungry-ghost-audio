# Suite 0.2.0 interfaces and deeper controls

The suite uses one metalwork and typography system, with layouts chosen around each processor's job. Controls are positioned on initial editor creation as well as resize; this fixes missing knobs on first open in 0.1.0.

| Tools | Main workflow | Additional controls in Shape |
|---|---|---|
| RIFT, BLOOM | Transfer curve and gain reduction history | Detector high-pass, key audition |
| CLAW | Fast rack compressor with gain reduction meter | Detector high-pass, key audition |
| VELVET | Optical-style reduction needle and large threshold/makeup | Detector high-pass, key audition |
| BOND | Bus input/output meters and reduction needle | Key audition; key filter remains on front panel |
| CRUSH, DUCK | Parallel / keyed waveform comparison | Detector high-pass, key audition |
| HUSH, GATE | Sibilance spectrum / gate waveform | Detector high-pass, key audition |
| PUNCH, TAIL | Attack / sustain workflow and waveform | Front-panel envelope controls |
| GUARD, EDGE | Vertical peak meters and large ceiling / clipping control | Front-panel timing / shaping controls |
| RIDE | Level fader and waveform comparison | Front-panel response / range controls |
| SHADE, CONTOUR, FOCUS | Console strips and filter response | SHADE individual band widths |
| TILT, WEIGHT, AIR, VOID, CROSS | Filter response and focused controls | TILT shelf shape |
| EMBER, IRON, FURNACE | Drive knob, static transfer curve | Pre-drive low cut |
| FOLD, RECT, SHARDS, STATIC | Shaping or sample-hold view | Pre-stage low cut |
| ECHO, PING, SLAP, DUB | Repeat timing, stereo / feedback workflow | Host tempo sync, note divisions, fallback tempo, repeat low cut |
| REEL | Blender-rendered metal spools with fixed scene lighting | Host tempo sync, note divisions, fallback tempo, repeat low cut |
| TEETH | Comb period display | Repeat low cut |
| CHOIR, FLUX, PHASE, DRIFT | Modulation shape / depth | Tempo sync and divisions |
| PULSE, ORBIT | LFO amplitude / stereo movement | Tempo sync and divisions |
| RING | Carrier oscillator | Front-panel frequency / depth |
| WIDE, CENTER, SHADOW | Measured output vectorscope and correlation | Mono audition |
| TRIM, REVERSE, CLEAN | Gain fader / polarity switches / focused filtering | REVERSE mono audition |

Factory plus two musical starting presets are available for each of the 48 newer processors. Factory preserves the original mix defaults. A/B state includes advanced controls; legacy sessions restore missing advanced settings to neutral defaults.

Response, transfer and repeat-timing displays show parameter settings. Waveforms, spectra, peak meters, reduction meters and stereo scopes show measured audio. The reel motion is visual artwork, not a measurement of physical tape transport. Rendering each rotation under fixed lighting avoids rotating baked highlights.

Tempo sync keeps the original free-time or rate parameter intact. Host tempo takes priority; fallback tempo is used when unavailable. Detector audition outputs the filtered key. Mono audition outputs the stereo sum and can expose cancellation. These operations affect audio and are included in saved state.

The existing REVERB and FERAL retain their specialised editors and product identities. This release does not claim true-peak limiting, transparent linear-phase EQ, oversampling in every processor, or feature parity with third-party suites.
