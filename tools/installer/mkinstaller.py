# Helper tool that:
#   1. Finds all DLLs imported by dist\release\goblin-camp.exe.
#   2. Determines version of Visual C++ Runtime used.
#   3. Copies third-party DLLs to dist\release directory.
#   4. Generates installer manifest including all files in dist and VC++ redistributable package.
#   5. Creates NSIS installer in dist\installer
# Should work with any Python >= 2.5
#
# Assumes release variant was already built, because creating installer for end users
# out of debug version doesn't make much sense.
from __future__ import with_statement
import sys, os, shutil, subprocess
import pefile

VC2008_CRT = [frozenset(('msvcp90.dll', 'msvcr90.dll')), False]
VC2010_CRT = [frozenset(('msvcp100.dll', 'msvcr100.dll')), False]
SYSTEM_DLL = frozenset((
    'kernel32.dll', 'user32.dll', 'gdi32.dll', 'opengl32.dll', 'winmm32.dll',
    'advapi32.dll', 'ntdll.dll', 'winmm.dll', 'rpcrt4.dll', 'secur32.dll',
    'msvcrt.dll', 'dbghelp.dll', 'shell32.dll', 'shlwapi.dll'
))

def findDLL(fn):
    for path in PATH:
        path = os.path.join(path, fn)
        if os.path.exists(path):
            return path

def gatherDLLs(dlls, exe):
    for entry in exe.DIRECTORY_ENTRY_IMPORT:
        lname = entry.dll.lower()
        if lname in VC2008_CRT[0]:
            VC2008_CRT[1] = True
        elif lname in VC2010_CRT[0]:
            VC2010_CRT[1] = True
        elif lname not in SYSTEM_DLL:
            name = findDLL(entry.dll)
            dlls.add(name)
            gatherDLLs(dlls, pefile.PE(name))

version, platform = sys.argv[1:]
PATH = os.environ['PATH'].split(os.pathsep)
if sys.platform == 'win32':
    PATH.insert(0, os.path.join(os.environ['SystemRoot'], 'system32'))

DIST      = os.path.join('build', 'dist')
SOURCE    = os.path.join(DIST, 'release-%s' % platform)
INSTALLER = os.path.join(DIST, 'installer')
EXEC      = os.path.join(SOURCE, 'goblin-camp.exe')
BASENSI   = os.path.join('build', 'installer', 'base.nsi')
OUTNSI    = os.path.join(INSTALLER, 'out.nsi')

if not os.path.exists(SOURCE):
    print 'Run "bjam variant=release dist" first.'
    sys.exit(255)

if os.path.exists(INSTALLER):
    shutil.rmtree(INSTALLER)

execDLLs = set()
gatherDLLs(execDLLs, pefile.PE(EXEC))
os.makedirs(INSTALLER)

manifest = { 'install': [], 'install-pdb': [], 'uninstall': [], 'uninstall-dirs': [] }
command  = ['makensis', '/V3', '/NOCD']

for root, dirs, files in os.walk(SOURCE):
    root = root[len(SOURCE) + 1:].strip()
    
    manifest['install'].append('SetOutPath "$INSTDIR\\%s"' % root)
    for dn in dirs:
        manifest['uninstall-dirs'].insert(0, 'RMDir /r "$INSTDIR\\%s"' % os.path.join(root, dn))
    
    for fn in files:
        m = manifest['install-pdb'] if fn.endswith('.pdb') else manifest['install']
        m.append('File "%s\\%s"' % (SOURCE, os.path.join(root, fn)))
        manifest['uninstall'].append('Delete "$INSTDIR\\%s"' % os.path.join(root, fn))

manifest['install'].append('SetOutPath "$INSTDIR"')
for dll in execDLLs:
    manifest['install'].append('File "%s"' % dll)
    manifest['uninstall'].append('Delete "$INSTDIR\\%s"' % os.path.basename(dll))

if VC2008_CRT[1]:
    command.append('/DGC_BUNDLE_MSVC2008')
if VC2010_CRT[1]:
    command.append('/DGC_BUNDLE_MSVC2010')

with open(BASENSI, 'r') as fp:
    nsi = fp.read()

nsi = nsi.replace('%%_GC_PLATFORM_%%', platform)
nsi = nsi.replace('%%_GC_VERSION_%%', version)
nsi = nsi.replace('%%_GC_INSTALL_MANIFEST_%%', '\n'.join(manifest['install']))
nsi = nsi.replace('%%_GC_INSTALL_MANIFEST_PDB_%%', '\n'.join(manifest['install-pdb']))
nsi = nsi.replace('%%_GC_UNINSTALL_MANIFEST_%%', '\n'.join(manifest['uninstall']))
nsi = nsi.replace('%%_GC_UNINSTALL_MANIFEST_DIRS_%%', '\n'.join(manifest['uninstall-dirs']))

with open(OUTNSI, 'w') as fp:
    fp.write(nsi)

command.append(OUTNSI)
subprocess.Popen(command, shell = True).communicate()
