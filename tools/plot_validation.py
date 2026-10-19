#!/usr/bin/env python3
"""Export the saved ngspice/C++ comparison as standalone report figures."""
import argparse
import pathlib
import subprocess
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
p=argparse.ArgumentParser();p.add_argument('--results',required=True,type=pathlib.Path);p.add_argument('--probe',required=True,type=pathlib.Path);p.add_argument('--ngspice',required=True,type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,default=pathlib.Path('docs/validation'));a=p.parse_args()
a.results=a.results.resolve();a.probe=a.probe.resolve();a.ngspice=a.ngspice.resolve();a.output.mkdir(parents=True,exist_ok=True)
case=a.results/'13_440_0.05_1_0.5_0.7';x=np.loadtxt(case/'cpp.csv',delimiter=',');ref=np.loadtxt(case/'spice.csv',skiprows=1);y=np.interp(x[:,0],ref[:,0],ref[:,1]);keep=x[:,0]>.032
fig,ax=plt.subplots(figsize=(9,4));ax.plot(x[keep,0]*1000,y[keep]*1000,label='ngspice reference',color='#262626',linewidth=2);ax.plot(x[keep,0]*1000,x[keep,2]*1000,label='C++ circuit solver',color='#d16816',linestyle='--');ax.set(xlabel='Time (ms)',ylabel='Output (mV)',title='Supa ToneBender: nominal circuit, 440 Hz / 50 mV input');ax.legend();ax.grid(alpha=.2);fig.tight_layout();fig.savefig(a.output/'supa-waveform.png',dpi=180);plt.close(fig)
fig,ax=plt.subplots(figsize=(9,4))
for tone in (0,.5,1):
 path=a.results/f'tone_curve_{tone}';path.mkdir(exist_ok=True)
 out=subprocess.run([str(a.probe),'13','192000','440','.001','.01','1',str(tone),'.7','--spice'],capture_output=True,text=True,check=True).stdout
 (path/'reference.cir').write_text(out);subprocess.run([str(a.ngspice),'-b','reference.cir'],cwd=path,capture_output=True,check=True)
 ac=np.loadtxt(path/'ac.csv',skiprows=1);gain=20*np.log10(np.maximum(np.hypot(ac[:,1],ac[:,2]),1e-12));ax.semilogx(ac[:,0],gain,label=f'Tone {tone:g}')
ax.set(xlabel='Frequency (Hz)',ylabel='Small-signal voltage gain (dB)',title='Supa ToneBender: nominal-circuit AC analysis in ngspice');ax.legend();ax.grid(which='both',alpha=.2);fig.tight_layout();fig.savefig(a.output/'supa-tone.png',dpi=180)
