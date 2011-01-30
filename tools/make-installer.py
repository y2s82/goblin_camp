# A tool to create NSIS installer out of the release build.
from __future__ import with_statement
import sys, os, shutil, subprocess, re
gatherDLLs = __import__('gather-dlls')

here = os.path.dirname(os.path.realpath(__file__))
root = os.path.normpath(here + '/..')

VERSION_RE = re.compile(r'^constant\s+GC_VERSION\s+:\s+(.*?)\s+;$', re.M)

if len(sys.argv) < 2:
    print 'Usage: make-installer x86/x64'
    sys.exit(1)

arch = sys.argv[1]
with open(os.path.join(root, 'Jamroot'), 'r') as fp:
    version = VERSION_RE.search(fp.read()).group(1)

out     = os.path.join(root, 'build', 'installer-{0}'.format(arch))
src     = os.path.join(root, 'build', 'bin-release-{0}'.format(arch))
baseNSI = os.path.join(here, 'installer', 'base.nsi')
outNSI  = os.path.join(out, 'goblin-camp.nsi')
redist  = os.path.join(here, 'installer', 'redists')

if not os.path.exists(os.path.join(src, 'goblin-camp.exe')):
    print '** Create the release build first.'
    sys.exit(1)

try:
    os.makedirs(out)
except OSError:
    pass

dlls = gatherDLLs.FindMeDLLs(os.path.join(src, 'goblin-camp.exe'))
dlls.copy()

manifest = { 'install': [], 'uninstall': [], 'uninstall-dirs': [] }

for root, dirs, files in os.walk(src):
    root = root[len(src) + 1:].strip()
    
    manifest['install'].append('SetOutPath "$INSTDIR\\{0}"'.format(root))
    for dn in dirs:
        manifest['uninstall-dirs'].insert(0, 'RMDir "$INSTDIR\\{0}"'.format(os.path.join(root, dn)))
    
    for fn in files:
        if fn[-3:] not in ('exe', 'dat', '.py', 'png', 'txt', 'dll', 'zip'): continue
        manifest['install'].append('File "{0}\\{1}"'.format(src, os.path.join(root, fn)))
        manifest['uninstall'].append('Delete "$INSTDIR\\{0}"'.format(os.path.join(root, fn)))

with open(baseNSI, 'r') as fp:
    nsi = fp.read()

nsi = nsi.replace('%%_GC_PLATFORM_%%', arch)
nsi = nsi.replace('%%_GC_VERSION_%%', version)
nsi = nsi.replace('%%_GC_REDIST_%%', redist)
nsi = nsi.replace('%%_GC_SOURCE_%%', src)
nsi = nsi.replace('%%_GC_INSTALL_MANIFEST_%%', '\n'.join(manifest['install']))
nsi = nsi.replace('%%_GC_UNINSTALL_MANIFEST_%%', '\n'.join(manifest['uninstall']))
nsi = nsi.replace('%%_GC_UNINSTALL_MANIFEST_DIRS_%%', '\n'.join(manifest['uninstall-dirs']))

with open(outNSI, 'w') as fp:
    fp.write(nsi)

command = ['makensis', '/V3']

if gatherDLLs.CRT_2008 in dlls.crts:
    command.append('/DGC_BUNDLE_MSVC2008')
if gatherDLLs.CRT_2010 in dlls.crts:
    command.append('/DGC_BUNDLE_MSVC2010')

command.append(outNSI)
subprocess.call(command, shell = True)
