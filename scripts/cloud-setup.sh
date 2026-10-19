#!/usr/bin/env bash
set -euo pipefail
# Run in the existing isolated cloud task. No extra Git worktree is needed.
repo=/workspace/JerzyBigMuffCollectionVST3
sysroot=/workspace/sysroot
cmake=/workspace/tools/python/cmake/data/bin/cmake
ninja=/workspace/tools/python/bin/ninja
mkdir -p /workspace/tools /workspace/apt/lists/partial /workspace/apt/cache/archives/partial "$sysroot" /workspace/cache/config
if [[ ! -x "$cmake" || ! -x "$ninja" ]]; then
 python -m pip install --target /workspace/tools/python cmake==4.4.4 ninja==1.13.2
fi
cat > /workspace/apt/sources.list <<'SOURCES'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
SOURCES
apt_options=(-o Dir::Etc::sourcelist=/workspace/apt/sources.list -o Dir::Etc::sourceparts=- -o Dir::State::lists=/workspace/apt/lists -o Dir::Cache=/workspace/apt/cache -o APT::Sandbox::User=agent -o Debug::NoLocking=1)
apt-get "${apt_options[@]}" update
apt-get "${apt_options[@]}" --download-only --no-install-recommends -y install libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig1-dev libxrender-dev libgl1-mesa-dev g++-mingw-w64-x86-64-posix wine64 xvfb ngspice
for archive in /workspace/apt/cache/archives/*.deb; do dpkg-deb -x "$archive" "$sysroot"; done
python - <<'PY'
from pathlib import Path
root=Path('/workspace/sysroot/usr/lib/x86_64-linux-gnu')
for p in root.glob('*.so'):
 if p.is_symlink() and not p.exists():
  name=p.readlink().name;source=Path('/usr/lib/x86_64-linux-gnu')/name
  target=root/name
  if source.exists() and not target.exists() and not target.is_symlink():target.symlink_to(source)
for name in ('gcc','g++'):
 p=Path('/workspace/sysroot/usr/bin')/('x86_64-w64-mingw32-'+name)
 if not p.exists():p.symlink_to(p.name+'-posix')
p=Path('/workspace/sysroot/usr/lib/wine/wineserver')
if not p.is_symlink():
 backup=p.with_name('wineserver.debian')
 if not backup.exists():p.rename(backup)
 else:p.unlink()
 p.symlink_to('wineserver64')
p=Path('/workspace/sysroot/usr/share/wine/wine')
if not p.exists():p.symlink_to('.')
p=Path('/workspace/sysroot/usr/lib/x86_64-linux-gnu/wine/x86_64-windows/zlib1.dll')
if not p.exists():p.symlink_to('/workspace/sysroot/usr/x86_64-w64-mingw32/lib/zlib1.dll')
include=Path('/workspace/tools/win-headers');include.mkdir(exist_ok=True)
for name in ('Windows.h','Dbghelp.h'):
 p=include/name
 if not p.exists():p.symlink_to('/workspace/sysroot/usr/x86_64-w64-mingw32/include/'+name.lower())
PY
if [[ ! -d /workspace/deps/JUCE7/.git ]]; then
 git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git /workspace/deps/JUCE7
fi
[[ $(git -C /workspace/deps/JUCE7 rev-parse HEAD) == 4f43011b96eb0636104cb3e433894cda98243626 ]]
cat > /workspace/tools/wine <<'WINE'
#!/usr/bin/env python3
import os
import sys
from pathlib import Path
env = os.environ.copy()
env.update(LD_LIBRARY_PATH='/workspace/sysroot/usr/lib/x86_64-linux-gnu'+(':'+env['LD_LIBRARY_PATH'] if env.get('LD_LIBRARY_PATH') else ''), XDG_CACHE_HOME='/workspace/cache', XDG_CONFIG_HOME='/workspace/cache/config', WINEPREFIX='/workspace/wine-ready', WINEDEBUG='-all', DISPLAY=env.get('DISPLAY', ':99'))
prefix=Path(env['WINEPREFIX'])/'drive_c/windows/system32'
prefix.mkdir(parents=True,exist_ok=True)
zlib=prefix/'zlib1.dll'
if not zlib.exists():
 import shutil
 shutil.copyfile('/workspace/sysroot/usr/x86_64-w64-mingw32/lib/zlib1.dll',zlib)
args = [('Z:'+arg if arg.startswith('/') else arg) for arg in sys.argv[1:]]
exe='/workspace/sysroot/usr/lib/wine/wine64'
os.execve(exe,[exe,*args],env)
WINE
chmod +x /workspace/tools/wine
export PATH=/workspace/sysroot/usr/bin:/workspace/tools/python/bin:$PATH
export XDG_CACHE_HOME=/workspace/cache XDG_CONFIG_HOME=/workspace/cache/config
export PKG_CONFIG_PATH=/workspace/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:/workspace/sysroot/usr/lib/pkgconfig
export CMAKE_BUILD_PARALLEL_LEVEL=2
# Configure native helper explicitly: JUCE bootstraps it without outer prefix flags.
for build in build-linux7 build-win7; do
 "$cmake" -S /workspace/deps/JUCE7 -B "$repo/$build/juce/tools" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" -DJUCE_BUILD_HELPER_TOOLS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$sysroot/usr" -DCMAKE_CXX_FLAGS="-I$sysroot/usr/include -I$sysroot/usr/include/freetype2" -DCMAKE_EXE_LINKER_FLAGS="-L$sysroot/usr/lib/x86_64-linux-gnu"
done
"$cmake" -S "$repo" -B "$repo/build-linux7" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR=/workspace/deps/JUCE7 -DCMAKE_PREFIX_PATH="$sysroot/usr" -DCMAKE_CXX_FLAGS="-I$sysroot/usr/include -I$sysroot/usr/include/freetype2" -DCMAKE_EXE_LINKER_FLAGS="-L$sysroot/usr/lib/x86_64-linux-gnu" -DCMAKE_SHARED_LINKER_FLAGS="-L$sysroot/usr/lib/x86_64-linux-gnu"
"$cmake" --build "$repo/build-linux7" --parallel 2
"$cmake" -S "$repo" -B "$repo/build-win7" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" -DCMAKE_TOOLCHAIN_FILE="$repo/cmake/mingw.cmake" -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR=/workspace/deps/JUCE7 -DCMAKE_CXX_FLAGS=-I/workspace/tools/win-headers -DCMAKE_RC_FLAGS=-I/workspace/sysroot/usr/x86_64-w64-mingw32/include -DCMAKE_CROSSCOMPILING_EMULATOR=/workspace/tools/wine
# Manifest generation executes Windows code. An X server is needed by Wine.
if [[ ! -S /tmp/.X11-unix/X99 ]]; then
 /workspace/sysroot/usr/bin/Xvfb :99 -screen 0 1280x1024x24 -nolisten tcp > /workspace/xvfb.log 2>&1 &
 for attempt in {1..50}; do [[ -S /tmp/.X11-unix/X99 ]] && break; sleep .1; done
fi
export DISPLAY=:99
"$cmake" --build "$repo/build-win7" --parallel 2
/workspace/tools/python/cmake/data/bin/ctest --test-dir "$repo/build-linux7" --output-on-failure
/workspace/tools/python/cmake/data/bin/ctest --test-dir "$repo/build-win7" --output-on-failure

mkdir -p /workspace/validators/windows
if [[ ! -x /workspace/validators/pluginval ]]; then
 curl -fL --max-time 120 https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip -o /workspace/pluginval-linux.zip
 python - <<'PYZIP'
import zipfile
with zipfile.ZipFile('/workspace/pluginval-linux.zip') as z:z.extractall('/workspace/validators')
PYZIP
 chmod +x /workspace/validators/pluginval
fi
if [[ ! -f /workspace/validators/windows/pluginval.exe ]]; then
 curl -fL --max-time 120 https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Windows.zip -o /workspace/pluginval-windows.zip
 python - <<'PYZIP'
import zipfile
with zipfile.ZipFile('/workspace/pluginval-windows.zip') as z:z.extractall('/workspace/validators/windows')
PYZIP
fi
/workspace/validators/pluginval --strictness-level 5 --validate "$repo/build-linux7/JerzyMuff_artefacts/Release/VST3/Jerzy Muff Collection.vst3"
/workspace/tools/wine /workspace/validators/windows/pluginval.exe --strictness-level 5 --output-dir /workspace/validation-windows --validate "$repo/build-win7/JerzyMuff_artefacts/Release/VST3/Jerzy Muff Collection.vst3"
