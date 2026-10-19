#!/usr/bin/env python3
"""Package a tested bundle with source and licence notices; never run builds."""
import argparse
import hashlib
import pathlib
import zipfile

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
archive = output / f'JerzyMuffCollection-{args.platform}-0.1.0.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for p in sorted(bundle.rglob('*')):
        if p.is_file():
            z.write(p, pathlib.Path(bundle.name) / p.relative_to(bundle))
    for name in ['README.md', 'LICENSE', 'docs/JUCE-LICENSE.md']:
        z.write(root / name, name)
    # Include the complete project sources and build recipe, not only binaries.
    for name in ['Source', 'tests', 'cmake', 'scripts', '.github', 'docs']:
        for p in sorted((root / name).rglob('*')):
            if p.is_file():
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
