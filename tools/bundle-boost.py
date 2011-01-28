# Helper to extract Boost header dependencies from GC source files
# Scans INCLUDE env var to find Boost path automatically
import sys, os, fnmatch, re, collections, shutil

# input is path to Goblin Camp\src
# output is path to where Boost deps should be copied
# boost is path to where Boost is (if not provided, then
if len(sys.argv) < 3:
    print 'Usage: bundle-boost.py <sources> <output> [boost path]'
    sys.exit(1)

input, output = sys.argv[1:3]
boost = None if len(sys.argv) < 4 else sys.argv[3]

if boost is None:
    paths = os.environ.get('INCLUDE', '').split(os.pathsep)
    for path in paths:
        if os.path.exists(os.path.join(path, 'boost', 'version.hpp')):
            boost = path
            break

if boost is None:
    print 'Boost not found.'
    sys.exit(1)

src = []
for root, dirs, files in os.walk(input):
    src.extend(os.path.join(root, fn) for fn in fnmatch.filter(files, '*.cpp'))
for root, dirs, files in os.walk(os.path.join(output, 'libs')):
    src.extend(os.path.join(root, fn) for fn in fnmatch.filter(files, '*.cpp'))

CPP_INCLUDE = re.compile(r'^#\s*include\s*([<"].*\.[hi]pp[">])$', re.I)

found    = set()
searched = set()

print '*** Searching the codebase'

def search(fn, cwd, isBoost):
    paths = [boost]
    
    if isBoost and fn.startswith('"'):
        paths.append(cwd)
    
    fn = fn[1:-1]
    
    for path in paths:
        full = os.path.join(path, fn)
        if os.path.exists(full):
            return os.path.normpath(full)

def parse(fn, isBoost = False):
    if fn in searched: return
    searched.add(fn)
    
    base = os.path.dirname(fn)
    queue = set()
    
    with open(fn, 'r') as fp:
        for line in fp:
            m = CPP_INCLUDE.match(line)
            if m is None: continue
            
            inc = search(m.group(1), base, isBoost)
            
            if inc is None or not inc.startswith(boost): continue
            
            found.add(inc)
            queue.add(inc)
    
    for inc in queue:
        parse(inc, True)

for fn in src:
    parse(fn)

print '*** Boost headers'

for fn in sorted(found):
    name = fn[len(boost) + 1:]
    
    print '\t', name
    
    try:
        os.makedirs(os.path.join(output, os.path.dirname(name)))
    except OSError:
        pass
    
    shutil.copy(fn, os.path.join(output, name))
