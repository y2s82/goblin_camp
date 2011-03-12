# This tool analyses given executable image, and copies all non-system, non-CRT DLLs to its directory.
# It will also warn about usage of multiple CRT versions, and can be used as a library, to determine
# which CRT redistributable(s) should be used.
import os, sys, shutil, itertools
import pefile

def _enum(ns, names):
    for name in names.split():
        ns[name] = name

def S(*args): return frozenset(args)

_enum(globals(), 'CRT_60 CRT_2008 CRT_2010 CRT_2008_DBG CRT_2010_DBG')

CRT_DLLS = {
    CRT_60:       S('msvcrt.dll'),
    CRT_2008:     S('msvcp90.dll',   'msvcr90.dll'),
    CRT_2008_DBG: S('msvcp90d.dll',  'msvcr90d.dll'),
    CRT_2010:     S('msvcp100.dll',  'msvcr100.dll'),
    CRT_2010_DBG: S('msvcp100d.dll', 'msvcr100d.dll')
}

SYSTEM_DLLS = S(
    'kernel32.dll', 'user32.dll', 'gdi32.dll', 'opengl32.dll', 'winmm32.dll',
    'advapi32.dll', 'ntdll.dll', 'winmm.dll', 'rpcrt4.dll', 'secur32.dll',
    'dbghelp.dll', 'shell32.dll', 'shlwapi.dll', 'kernelbase.dll',
)

class FindMeDLLs(object):
    def __init__(self, exe):
        self.pe   = pefile.PE(exe)
        self.dir  = os.path.dirname(os.path.realpath(exe))
        self.crts = set()
        
        self.searchPath = os.environ['PATH'].split(os.pathsep)
        
        if 'SystemRoot' in os.environ:
            # system32/SysWOW64 are searched implicitly
            self.searchPath.append(os.path.join(os.environ['SystemRoot'], 'SysWOW64'))
            self.searchPath.append(os.path.join(os.environ['SystemRoot'], 'system32'))
        
        self.dlls = set()
        self.gather(exe, self.pe)
    
    def find(self, dll):
        for path in self.searchPath:
            path = os.path.join(path, dll)
            if not os.path.exists(path): continue
            
            image = pefile.PE(path)
            if image.FILE_HEADER.Machine != self.pe.FILE_HEADER.Machine:
                # consider only DLLs with matching target arch
                continue
            
            return path, image
        
        path = os.path.join(self.dir, dll)
        if os.path.exists(path):
            # this is DLL built alongside GC
            return None, pefile.PE(path)
        
        return None, None
    
    def isCRT(self, dll):
        for crt, names in CRT_DLLS.iteritems():
            if dll.lower() not in names: continue
            return crt
        
        return None
    
    def isSystem(self, dll):
        return dll.lower() in SYSTEM_DLLS
    
    def gather(self, name, image):
        for entry in image.DIRECTORY_ENTRY_IMPORT:
            if self.isSystem(entry.dll): continue
            
            crt = self.isCRT(entry.dll)
            if crt is not None:
                self.crts.add(crt)
                continue
            
            name, image = self.find(entry.dll)
            self.dlls.add((entry.dll, name))
            self.gather(entry.dll, image)
    
    def hasMultipleCRTs(self):
        return len(self.crts) > 1
    
    def copy(self):
        # copy dbghelp.dll from redists
        redist = os.path.join(os.path.dirname(__file__), 'installer', 'redists')
        if self.pe.FILE_HEADER.Machine == pefile.MACHINE_TYPE['IMAGE_FILE_MACHINE_I386']:
            dbghelp = os.path.join(redist, 'dbghelp-x86.dll')
        else:
            dbghelp = os.path.join(redist, 'dbghelp-x64.dll')
        
        shutil.copy(dbghelp, self.dir + '/dbghelp.dll')
        
        for _, dll in self.dlls:
            if dll is None: continue
            shutil.copy(dll, self.dir + '/')
        
        self.dlls.add(('dbghelp.dll', dbghelp))

if __name__ == '__main__':
    finder = FindMeDLLs(sys.argv[1])
    if finder.hasMultipleCRTs():
        print '** Warning: multiple CRTs detected: {0}'.format(', '.join(finder.crts))
    print '** DLLs:'
    finder.copy()
    print '\n'.join('{0} -> {1}'.format(fn, dll if dll is not None else '<provided>') for fn, dll in finder.dlls)
