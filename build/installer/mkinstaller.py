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
import os, sys, shutil, subprocess, codecs
from contextlib import closing
import pefile

VC2008_CRT = frozenset(('msvcp90.dll', 'msvcr90.dll'))
VC2010_CRT = frozenset(('msvcp100.dll', 'msvcr100.dll'))
SYSTEM_DLL = frozenset((
    'kernel32.dll', 'user32.dll', 'gdi32.dll', 'opengl32.dll', 'winmm32.dll',
    'advapi32.dll', 'ntdll.dll', 'winmm.dll', 'rpcrt4.dll', 'secur32.dll',
    'msvcrt.dll', 'dbghelp.dll', 'shell32.dll', 'shlwapi.dll'
))

def findDLL(fn):
    for path in PATH:
        if os.path.exists(os.path.join(path, fn)):
            return os.path.join(path, fn)

redist = None
def setRedist(dll, target):
    global redist
    
    assert redist is None or redist == target, 'Conflicting C++ runtimes: ' + dll
    redist = target

def gatherDLLs(exe):
    for entry in exe.DIRECTORY_ENTRY_IMPORT:
        lname = entry.dll.lower()
        name  = entry.dll
        if lname in VC2008_CRT:
            setRedist(name, '2008')
        elif lname in VC2010_CRT:
            setRedist(name, '2010')
        elif lname not in SYSTEM_DLL:
            name = findDLL(name)
            DLLs.add(name)
            gatherDLLs(pefile.PE(name))

assert len(sys.argv) == 2, 'Usage: mkinstaller <version>'
assert os.path.exists('build'), 'Run from project root.'
assert os.path.exists(os.path.join('build', 'dist', 'release', 'goblin-camp.exe')), 'Run "bjam variant=release install" first.'

exe  = pefile.PE(os.path.join('build', 'dist', 'release', 'goblin-camp.exe'))
DLLs = set()

PATH = os.environ['PATH'].split(os.pathsep)
if sys.platform == 'win32':
    PATH.insert(0, os.path.join(os.environ['SystemRoot'], 'system32'))

gatherDLLs(exe)
files = set(os.listdir(os.path.join('build', 'dist', 'release')))

if os.path.exists(os.path.join('build', 'dist', 'installer')):
    shutil.rmtree(os.path.join('build', 'dist', 'installer'))

os.makedirs(os.path.join('build', 'dist', 'installer', 'src'))

for fn in files:
    print '\tPREPARE %s' % fn
    shutil.copy(
        os.path.join('build', 'dist', 'release', fn),
        os.path.join('build', 'dist', 'installer', 'src')
    )

for fn in DLLs:
    print '\tDLL %s' % fn
    shutil.copy(
        fn,
        os.path.join('build', 'dist', 'installer', 'src')
    )

print '\tINCLUDE dbghelp.dll'
shutil.copy(
    os.path.join('build', 'installer', 'redists', 'dbghelp.dll'),
    os.path.join('build', 'dist', 'installer', 'src')
)
print '\tINCLUDE %s/vcredist_x86.exe' % redist
shutil.copy(
    os.path.join('build', 'installer', 'redists', 'vc%s' % redist, 'vcredist_x86.exe'),
    os.path.join('build', 'dist', 'installer', 'src')
)

manifest = [fn for fn in files if fn[-4:] != '.pdb']

with closing(codecs.open(os.path.join('build', 'installer', 'base.nsi'), 'r', 'utf-8')) as fp:
    template = fp.read()

template = template.replace(
    u'%%_GC_INSTALL_MANIFEST_%%', u'\n    '.join(ur'File "src\%s"' % fn for fn in manifest)
)
template = template.replace(
    u'%%_GC_UNINSTALL_MANIFEST_%%', u'\n    '.join(ur'Delete "$INSTDIR\%s"' % fn for fn in manifest)
)
template = template.replace(u'%%_GC_VCREDIST_VERSION_%%', unicode(redist))
template = template.replace(u'%%_GC_VERSION_%%', unicode(sys.argv[1]))

print '\tSCRIPT'
with closing(codecs.open(os.path.join('build', 'dist', 'installer', 'installer.nsi'), 'w', 'utf-8')) as fp:
    fp.write(template)

cwd = os.getcwd()
os.chdir(os.path.join('build', 'dist', 'installer'))
print '\tMAKENSIS'
subprocess.Popen('makensis /V3 installer.nsi', shell = True).communicate()
