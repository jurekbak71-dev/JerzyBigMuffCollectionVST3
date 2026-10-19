#!/usr/bin/env python3
"""Render reproducible guitar comparisons through MuffProcessor (requires numpy, ffmpeg).
Usage: python tools/render_audio_demos.py SOURCE.wav RENDERER OUTPUT_DIRECTORY
Uses the first 12 seconds of the documented GuitarML input; never its TS9 output.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import numpy as np

RATE = 44100
DURATION = 12
TAIL = 1
ROOT = Path(__file__).resolve().parents[1]
SOURCE_COMMIT = 'ee0983bb02af1e2db476466614d6421473beb01d'
SOURCE_URL = f'https://github.com/GuitarML/GuitarLSTM/blob/{SOURCE_COMMIT}/data/ts9_test1_in_FP32.wav'


def run(args, **kw):
    return subprocess.run(list(map(str, args)), check=True, **kw)


def stats(x):
    assert np.all(np.isfinite(x))
    return {'peak': float(np.max(np.abs(x))), 'rms': float(np.sqrt(np.mean(x.astype(float)**2)))}


def wav(raw, dest, codec):
    run(['ffmpeg', '-v', 'error', '-y', '-f', 'f32le', '-ar', RATE, '-ac', 1,
         '-i', raw, '-c:a', codec, dest])


def main():
    source, renderer, out = map(lambda x: Path(x).resolve(), sys.argv[1:])
    out.mkdir(parents=True, exist_ok=True)
    work = out / 'source'; work.mkdir(exist_ok=True)
    rawdir = out / 'raw-gain'; rawdir.mkdir(exist_ok=True)
    listen = out / 'wav'; listen.mkdir(exist_ok=True)
    previews = out / 'mp3'; previews.mkdir(exist_ok=True)
    fragment = work / 'guitar-original-excerpt.f32'
    run(['ffmpeg', '-v', 'error', '-y', '-i', source, '-t', DURATION,
         '-ar', RATE, '-ac', 1, '-f', 'f32le', fragment])
    original = np.fromfile(fragment, dtype='<f4')
    assert len(original) == RATE * DURATION
    # Fixed input calibration for every render: 250 mV peak in the circuit model.
    input_gain = .25 / stats(original)['peak']
    dry = original * input_gain
    input_path = work / 'guitar-calibrated.f32'; dry.astype('<f4').tofile(input_path)
    wav(fragment, work / 'guitar-original-excerpt.wav', 'pcm_f32le')
    dry = np.pad(dry, (0, RATE * TAIL)).astype('<f4')
    dry.tofile(rawdir / '00_dry.f32')
    models = json.loads((ROOT / 'circuits/revisions.json').read_text())['models']
    cases = [(i+1, m['id'], m['name'], .5, .5) for i, m in enumerate(models)]
    cases += [(14, 'supa_low_sustain', 'Supa: Sustain 15%', .15, .5),
              (14, 'supa_high_sustain', 'Supa: Sustain 90%', .9, .5),
              (14, 'supa_tone_10', 'Supa: Tone 10%', .5, .1),
              (14, 'supa_tone_90', 'Supa: Tone 90%', .5, .9),
              (13, 'rams_low_sustain', "Ram's Head: Sustain 15%", .15, .5),
              (13, 'rams_high_sustain', "Ram's Head: Sustain 90%", .9, .5)]
    records = [{'stem': '00_dry', 'name': 'Dry guitar', 'circuit': None,
                'sustain': None, 'tone': None, 'volume': None}]
    for n, (c, slug, name, sustain, tone) in enumerate(cases, 1):
        stem = f'{n:02d}_{slug}'; dest = rawdir / f'{stem}.f32'
        result = run([renderer, '--render-audio', input_path, dest, c, sustain,
                      tone, .5, RATE, TAIL], capture_output=True, text=True,
                     env=dict(os.environ, DISPLAY=os.environ.get('DISPLAY', ':99')))
        assert 'solver_failures=0' in result.stdout, result.stdout
        records.append(dict(stem=stem, name=name, circuit=c, sustain=sustain,
                            tone=tone, volume=.5, processor_result=result.stdout.strip()))
        print(stem, result.stdout.strip(), flush=True)
    # Equal RMS over the same 12 s performance, with >=1 dB peak headroom on all files.
    arrays = [np.fromfile(rawdir / (r['stem']+'.f32'), dtype='<f4') for r in records]
    assert all(len(x) == RATE*(DURATION+TAIL) for x in arrays)
    rms = [stats(x[:RATE*DURATION])['rms'] for x in arrays]
    assert min(rms) > 1e-7
    target = min(10**(-24/20), min(10**(-1/20)*r/stats(x)['peak'] for x,r in zip(arrays,rms)))
    matched = []
    for record, x, r in zip(records, arrays, rms):
        stem = record['stem']; gain = target/r; y = (x*gain).astype('<f4')
        assert stats(y)['peak'] <= 10**(-1/20)+1e-6
        temp = work / 'matched.f32'; y.tofile(temp)
        wav(temp, listen / (stem+'.wav'), 'pcm_s24le')
        wav(rawdir / (stem+'.f32'), rawdir / (stem+'.wav'), 'pcm_f32le')
        run(['ffmpeg', '-v', 'error', '-y', '-i', listen/(stem+'.wav'),
             '-c:a', 'libmp3lame', '-b:a', '192k', previews/(stem+'.mp3')])
        record.update(raw=stats(x), listening=stats(y), listening_gain_db=float(20*np.log10(gain)))
        matched.append(y)
    # A/B: the identical complete phrase dry, Supa mid, Ram's Head mid.
    montage = np.concatenate([matched[0], np.zeros(RATE), matched[14],
                              np.zeros(RATE), matched[13]]).astype('<f4')
    montage.tofile(work/'comparison.f32')
    wav(work/'comparison.f32', listen/'AB_dry_supa_rams.wav', 'pcm_s24le')
    run(['ffmpeg', '-v', 'error', '-y', '-i', listen/'AB_dry_supa_rams.wav',
         '-c:a', 'libmp3lame', '-b:a', '192k', previews/'AB_dry_supa_rams.mp3'])
    manifest = dict(source_url=SOURCE_URL, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                    source_credit='GuitarML / Keith Bloemer, Fender Telecaster bridge pickup, tone/volume max',
                    source_license='GPL-3.0 (upstream repository LICENSE.txt)',
                    source_start_seconds=0, source_excerpt_seconds=DURATION,
                    rate=RATE, channels=1, tail_seconds=TAIL, input_gain=input_gain,
                    calibrated_input_peak_volts=.25, volts_per_full_scale=1,
                    block_size=256, oversampling=4, preroll_seconds=.1,
                    alignment='Reported plugin latency removed; oversampling IIR retains frequency-dependent phase',
                    target_listening_rms_dbfs=float(20*np.log10(target)),
                    normalization_window_seconds=DURATION,
                    audio_path='Full MuffProcessor compiled from Source/Plugin.cpp; no amp/cab/EQ/limiter',
                    vst3_host_test=False, records=records)
    manifest['processor_source_sha256'] = {str(f.relative_to(ROOT)): hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted((ROOT/'Source').rglob('*')) if f.is_file()}
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    for f in out.rglob('*.f32'): f.unlink()
    print('COMPLETE', len(records), 'files; RMS dBFS', manifest['target_listening_rms_dbfs'], flush=True)

if __name__ == '__main__':
    main()
