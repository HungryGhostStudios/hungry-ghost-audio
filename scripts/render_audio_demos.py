"""Render original, reproducible listening previews through the released VST3s.

Requires numpy, pedalboard 0.9.25 and ffmpeg. Run with --plugins <VST3 folder>
and --manifest <release manifest.json>. No external samples are used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import wave

import numpy as np
from pedalboard import load_plugin

ROOT = Path(__file__).resolve().parents[1]
SR, LENGTH, ACTIVE = 48000, 14, 9.6

def note(midi):
    return 440 * 2 ** ((midi - 69) / 12)

def synth(sustained=False):
    audio = np.zeros((2, SR * LENGTH), dtype=np.float64)
    chords = [(50, 57, 60, 64, 69), (46, 53, 57, 60, 65), (53, 60, 64, 67, 72), (48, 55, 60, 62, 67)]
    for bar, chord in enumerate(chords):
        events = [(bar * 2.4, pitch, 2.15, voice) for voice, pitch in enumerate(chord)] if sustained else [
            (bar * 2.4 + step * .3, chord[step % len(chord)] + (12 if step % 3 == 2 else 0), .82, step % len(chord)) for step in range(8)]
        for start, pitch, duration, voice in events:
            t = np.arange(int(duration * SR)) / SR
            hz = note(pitch)
            env = np.minimum(t / (.065 if sustained else .006), 1) * np.exp(-t / (1.3 if sustained else .24))
            env *= np.minimum((duration - t) / .06, 1)
            tone = sum(np.sin(2 * np.pi * hz * h * t + .15 * voice) / h ** (1.8 if sustained else 1.5) for h in range(1, 8)) * env
            pan = .2 + voice * .15
            offset = round(start * SR)
            audio[0, offset:offset+len(t)] += tone * np.sqrt(1-pan)
            audio[1, offset:offset+len(t)] += tone * np.sqrt(pan)
    return audio / np.max(np.abs(audio)) * .42

def drums():
    audio = np.zeros((2, SR * LENGTH), dtype=np.float64)
    rng = np.random.default_rng(2130)
    for step in range(32):
        start = step * .3
        t = np.arange(round(.27 * SR)) / SR
        hat_noise = rng.standard_normal(len(t))
        hat = (hat_noise - np.roll(hat_noise, 1)) * np.exp(-t / .022) * .09
        hit = hat
        if step % 4 == 0 or step in [7, 15, 23]:
            hit = hit + .8 * np.sin(2*np.pi*(48*t + 80*.02*(1-np.exp(-t/.02)))) * np.exp(-t/.1)
        if step % 4 == 2:
            hit = hit + .26 * rng.standard_normal(len(t)) * np.exp(-t/.065) + .25*np.sin(2*np.pi*185*t)*np.exp(-t/.05)
        hit *= np.minimum((.27-t)/.02, 1)
        offset = round(start * SR)
        for channel in range(2):
            audio[channel, offset:offset+len(t)] += hit
    return audio / np.max(np.abs(audio)) * .6

DEMOS = [
    dict(id='reverb', title='REVERB', source='plucks', description='A small phrase, an enormous hall.', settings=dict(character='Hall', decay=4.6, mix=38, tone=-10, pre_delay=28)),
    dict(id='feral', title='FERAL', source='drums', description='Per-band control and bus compression.', settings=dict(threshold=-24, ratio=3, attack=12, release=130, band_2_gain=-3, band_2_range=6, band_4_range=7)),
    dict(id='rift', title='RIFT', source='drums', description='Controlled peaks, with the attack intact.', settings=dict(threshold_db=-24, ratio=4, attack_ms=18, release_ms=110, makeup_db=4)),
    dict(id='ember', title='EMBER', source='chords', description='More harmonics. More weight.', settings=dict(drive_db=15, tone_hz=9000, mix=.7)),
    dict(id='echo', title='ECHO', source='plucks', description='Warm repeats that keep the phrase moving.', settings=dict(time_ms=375, feedback=44, tone_hz=6500, motion=10, mix=.35)),
    dict(id='choir', title='CHOIR', source='chords', description='A wider, gently shifting ensemble.', settings=dict(rate_hz=.35, depth=60, delay_ms=20, feedback=18, mix=.6)),
]

def encode(audio, wav, mp3, ffmpeg):
    with wave.open(str(wav), 'wb') as file:
        file.setnchannels(2)
        file.setsampwidth(2)
        file.setframerate(SR)
        file.writeframes((audio.T * 32767).round().astype('<i2').tobytes())
    subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', '-i', str(wav), '-codec:a', 'libmp3lame', '-b:a', '192k', str(mp3)], check=True)
    decoded = np.frombuffer(subprocess.check_output([ffmpeg, '-hide_banner', '-loglevel', 'error', '-i', str(mp3), '-f', 'f32le', '-']), dtype='<f4')
    assert len(decoded) == SR * LENGTH * 2 and np.all(np.isfinite(decoded))
    assert np.max(np.abs(decoded)) < 1, 'Encoded audio must not clip'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--plugins', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--ffmpeg', default=shutil.which('ffmpeg'))
    args = parser.parse_args()
    if not args.ffmpeg:
        raise RuntimeError('ffmpeg is required')
    products = {p['id']: p for p in json.loads(args.manifest.read_text())['products']}
    sources = dict(plucks=synth(), chords=synth(True), drums=drums())
    target = ROOT / 'site' / 'public' / 'audio'
    target.mkdir(exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory() as scratch:
        for demo in DEMOS:
            product = products[demo['id']]
            binary = next((args.plugins / product['bundle']).glob('Contents/x86_64-win/*.vst3'))
            digest = hashlib.sha256(binary.read_bytes()).hexdigest()
            assert digest == product['binarySHA256'], 'The demo must use the exact released binary'
            plugin = load_plugin(str(binary))
            for key, value in demo['settings'].items():
                setattr(plugin, key, value)
            dry = sources[demo['source']].astype(np.float32)
            wet = plugin(dry.copy(), SR, buffer_size=512)
            assert wet.shape == dry.shape and np.all(np.isfinite(wet))
            assert np.sqrt(np.mean((wet-dry)**2)) > .001, 'Processing must audibly change the source'
            count = round(ACTIVE * SR)
            dry_rms = np.sqrt(np.mean(dry[:, :count] ** 2))
            wet_rms = np.sqrt(np.mean(wet[:, :count] ** 2))
            trim = float(dry_rms / wet_rms)
            wet *= trim
            scale = min(1., .89 / max(float(np.max(np.abs(dry))), float(np.max(np.abs(wet)))))
            dry, wet = dry * scale, wet * scale
            for name, audio in [('dry', dry), ('wet', wet)]:
                encode(audio, Path(scratch) / 'render.wav', target / f"{demo['id']}-{name}.mp3", args.ffmpeg)
            records.append({**demo, 'duration': LENGTH, 'sampleRate': SR, 'binarySHA256': digest, 'wetTrimDb': round(float(20*np.log10(trim)), 3), 'peakDbFS': round(float(20*np.log10(max(np.max(np.abs(dry)), np.max(np.abs(wet))))), 3)})
            print(demo['title'], records[-1]['peakDbFS'], 'dBFS; exact release binary')
    (target / 'manifest.json').write_text(json.dumps(dict(renderer='pedalboard 0.9.25', source='Original procedurally composed audio; no external samples', comparison='Active-region RMS matched; same MP3 encoding; four-second tail space', demos=records), indent=2) + '\n')

if __name__ == '__main__':
    main()
