from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
mini = root / '3rdparty/miniupnp/miniupnp/miniupnpc'
version = (mini / 'version').read_text().splitlines()[0].strip()
win = sys.getwindowsversion()
os_version = f'{win.major}.{win.minor}.{win.build}'
text = (mini / 'miniupnpcstrings.h.in').read_text()
text = re.sub(r'^#define OS_STRING .*$', f'#define OS_STRING "MSWindows/{os_version}"', text, flags=re.M)
text = re.sub(r'^#define MINIUPNPC_VERSION_STRING .*$', f'#define MINIUPNPC_VERSION_STRING "{version}"', text, flags=re.M)
(mini / 'miniupnpcstrings.h').write_text(text)
parts = version.split('.')
assert len(parts) >= 3 and all(p.isdigit() for p in parts[:3])
rc = f'#define LIBMINIUPNPC_DOTTED_VERSION "{version}"\n'
rc += ''.join(f'#define LIBMINIUPNPC_{name}_VERSION {number}\n' for name, number in zip(('MAJOR','MINOR','MICRO'), parts))
(mini / 'rc_version.h').write_text(rc)
print('Generated miniupnpc version headers without Windows Script Host or WMI.')

absl = root / '3rdparty/abseil-cpp'
if not (absl / 'CMakeLists.txt').is_file():
    raise SystemExit('Offline Abseil 20250512.1 source is required at source/3rdparty/abseil-cpp.')
project = root / '3rdparty/protobuf/protobuf_build.vcxproj'
text = project.read_text(encoding='utf-8')
marker = '-Dprotobuf_FORCE_FETCH_DEPENDENCIES=ON ^'
argument = '-DFETCHCONTENT_SOURCE_DIR_ABSL="$(SolutionDir)3rdparty\\abseil-cpp" ^'
if argument not in text:
    assert marker in text
    text = text.replace(marker, marker + '\n        ' + argument)
    project.write_text(text, encoding='utf-8')
print('Configured Protobuf to use the offline Abseil source.')
