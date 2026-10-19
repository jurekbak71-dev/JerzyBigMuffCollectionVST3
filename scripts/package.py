#!/usr/bin/env python3
"""Package a tested bundle with source and licence notices; never run builds."""
import argparse
import hashlib
import pathlib
import zipfile
import re
import json
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('bundle', type=pathlib.Path)
parser.add_argument('platform', choices=['Win64', 'Linux'])
parser.add_argument('--juce-source', required=True, type=pathlib.Path)
args = parser.parse_args()
if not (args.juce_source / 'modules/juce_core/juce_core.h').is_file():
    raise SystemExit('Complete JUCE source checkout is required for packaging')
root = pathlib.Path(__file__).resolve().parents[1]
bundle = args.bundle.resolve()
if not bundle.is_dir():
    raise SystemExit(f'Bundle is missing: {bundle}')
output = root / 'dist'
output.mkdir(exist_ok=True)
version = re.search(r'project\(JerzyMuffCollection VERSION ([0-9.]+)', (root/'CMakeLists.txt').read_text()).group(1)
archive = output / f'JerzyMuffCollection-{args.platform}-{version}.zip'
binary = bundle/'Contents'/('x86_64-win' if args.platform=='Win64' else 'x86_64-linux')/(bundle.stem+('.vst3' if args.platform=='Win64' else '.so'))
if not binary.is_file() or binary.read_bytes()[:2 if args.platform=='Win64' else 4] != (b'MZ' if args.platform=='Win64' else b'\x7fELF'):
    raise SystemExit('Missing binary or wrong platform in VST3 bundle')
manifest = json.loads((bundle/'Contents/Resources/moduleinfo.json').read_text())
if manifest.get('Version') != version:
    raise SystemExit('VST3 manifest does not match project version')
commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
metadata = dict(version=version, source_commit=commit, platform=args.platform, status='experimental nominal circuit model; hardware calibration unverified')
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    z.writestr('BUILD-INFO.json', json.dumps(metadata, indent=2)+'\n')
    for p in sorted(bundle.rglob('*')):
        if p.is_file():
            z.write(p, pathlib.Path(bundle.name) / p.relative_to(bundle))
    for name in ['README.md', 'LICENSE', 'docs/JUCE-LICENSE.md']:
        z.write(root / name, name)
    # Include the complete project sources and build recipe, not only binaries.
    for name in ['Source', 'tests', 'cmake', 'scripts', '.github', 'docs', 'circuits', 'tools']:
        for p in sorted((root / name).rglob('*')):
            if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc':
                z.write(p, pathlib.Path('source') / p.relative_to(root))
    for name in ['CMakeLists.txt', 'README.md', 'LICENSE', '.gitignore']:
        z.write(root / name, pathlib.Path('source') / name)
    for p in sorted(args.juce_source.rglob('*')):
        if p.is_file() and '.git' not in p.relative_to(args.juce_source).parts:
            z.write(p, pathlib.Path('source/JUCE') / p.relative_to(args.juce_source))
    z.writestr('source/JUCE-SOURCE.txt', 'JUCE 7.0.12 source: https://github.com/juce-framework/JUCE/tree/4f43011b96eb0636104cb3e433894cda98243626\nCMake fetches this exact commit.\n')
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(f'{digest}  {archive.name}\n')
print(archive)
print(digest)
