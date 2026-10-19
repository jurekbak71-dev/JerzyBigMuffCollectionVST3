#!/usr/bin/env python3
"""Compare C++ transient results to ngspice's independent circuit solver.

The exported reference has the same documented simplified device models and
component graph; agreement verifies numerical implementation, not authenticity
of unknown transistor samples or the transcription of the source artwork.
"""
import argparse
import concurrent.futures
import json
import pathlib
import subprocess
import numpy as np

p=argparse.ArgumentParser()
p.add_argument('--probe',required=True,type=pathlib.Path)
p.add_argument('--ngspice',required=True,type=pathlib.Path)
p.add_argument('--output',type=pathlib.Path,default=pathlib.Path('build/spice-validation'))
p.add_argument('--jobs',type=int,default=2)
a=p.parse_args();a.probe=a.probe.resolve();a.ngspice=a.ngspice.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
root=pathlib.Path(__file__).resolve().parents[1]
catalog=json.loads((root/'circuits/revisions.json').read_text())
cases=[(m,f,amp,.5,.5,.5) for m in range(14) for f,amp in [(82,.02),(1000,.2)]]
cases += [(13,f,amp,1,.5,.7) for f in [82,440,3000] for amp in [.005,.05,.5]]
cases += [(13,440,.1,.7,t,.7) for t in [0,1]]
cases += [(13,440,.1,.7,.5,v) for v in [0,1]]

def run(case):
 m,f,amp,s,t,v=case
 name=f'{m}_{f}_{amp}_{s}_{t}_{v}'
 path=a.output/name;path.mkdir(exist_ok=True)
 args=[str(a.probe),str(m),'192000',str(f),str(amp),'.04',str(s),str(t),str(v)]
 circuit=subprocess.run(args+['--spice'],text=True,capture_output=True,check=True).stdout
 (path/'reference.cir').write_text(circuit)
 result=subprocess.run(args,text=True,capture_output=True,check=True)
 (path/'cpp.csv').write_text(result.stdout);(path/'solver.txt').write_text(result.stderr)
 spice=subprocess.run([str(a.ngspice),'-b','reference.cir'],cwd=path,text=True,capture_output=True)
 (path/'ngspice.log').write_text(spice.stdout+spice.stderr)
 if spice.returncode:raise RuntimeError(f'ngspice failed: {path}')
 cpp=np.loadtxt(path/'cpp.csv',delimiter=',');ref=np.loadtxt(path/'spice.csv',skiprows=1)
 expected=np.interp(cpp[:,0],ref[:,0],ref[:,1]);actual=cpp[:,2]
 # Include onset; no gain fitting, time alignment or DC removal.
 scale=max(float(np.linalg.norm(expected)),1e-6)
 nrmse=float(np.linalg.norm(actual-expected)/scale)
 peak=float(np.max(np.abs(actual-expected)))
 passed=nrmse<.025 or peak<2e-6
 return dict(model=catalog['models'][m]['id'],frequency=f,amplitude=amp,sustain=s,tone=t,volume=v,nrmse=nrmse,max_error_volts=peak,passed=passed)

results=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
 for result in pool.map(run,cases):
  results.append(result);print(json.dumps(result),flush=True)
summary=dict(reference='ngspice transient, max step 1/(8*192000), nominal devices at 27 C',metric='Unaligned waveform NRMSE; no gain fit or DC correction',cases=results,passed=sum(r['passed'] for r in results),total=len(results),worst_nrmse=max(r['nrmse'] for r in results))
(a.output/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print(f"{'PASS' if summary['passed']==summary['total'] else 'FAIL'} {summary['passed']}/{summary['total']}, worst NRMSE {summary['worst_nrmse']:.3%}")
raise SystemExit(0 if summary['passed']==summary['total'] else 1)
