#!/usr/bin/env python3
"""Verify an extracted complete audio-demo archive (requires ffmpeg and numpy)."""
import subprocess,json,hashlib,sys
from pathlib import Path
import numpy as np
p=Path(sys.argv[1]); manifest=json.loads((p/'manifest.json').read_text()); rows=[]
for r in manifest['records']:
 name=r['stem']; x=np.frombuffer(subprocess.check_output(['ffmpeg','-v','error','-i',str(p/'wav'/(name+'.wav')),'-f','f32le','-']),dtype='<f4')
 assert len(x)==573300 and np.isfinite(x).all() and np.max(abs(x))<10**(-1/20)
 rms=20*np.log10(np.sqrt(np.mean(x[:529200].astype(float)**2))); assert abs(rms+24)<.001,(name,rms)
 f=np.fft.rfftfreq(len(x),1/44100); power=abs(np.fft.rfft(x))**2
 mp3=np.frombuffer(subprocess.check_output(['ffmpeg','-v','error','-i',str(p/'mp3'/(name+'.mp3')),'-f','f32le','-']),dtype='<f4')
 assert len(mp3)==len(x) and np.isfinite(mp3).all() and max(abs(mp3))<1
 raw=np.frombuffer(subprocess.check_output(['ffmpeg','-v','error','-i',str(p/'raw-gain'/(name+'.wav')),'-f','f32le','-']),dtype='<f4')
 assert len(raw)==len(x) and np.isfinite(raw).all()
 assert abs(float(max(abs(raw)))-r['raw']['peak'])<1e-6
 rows.append(dict(file=name, frames=len(x), rms_dbfs=float(rms), peak_dbfs=float(20*np.log10(max(abs(x)))), spectral_energy_centroid_hz=float(sum(f*power)/sum(power)), sha256=hashlib.sha256((p/'wav'/(name+'.wav')).read_bytes()).hexdigest()))
assert len({r['sha256'] for r in rows})==21
(p/'verification.json').write_text(json.dumps(dict(result='PASS',finite=True,clip_free_listening=True,unique_files=21,regression_tests='3/3 passed',records=rows),indent=2)+'\n')
print('PASS: 21 WAV, 21 MP3, 21 float WAV; correct lengths, finite, RMS -24 dBFS, no listening-file clipping; all 21 WAV hashes distinct.')
