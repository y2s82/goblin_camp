# Helper to extract Boost header dependencies from GC source files
# Scans INCLUDE env var to find Boost path automatically
import sys, os, fnmatch, re, collections, shutil

boost = None if len(sys.argv) < 2 else sys.argv[1]

if boost is None:
    paths = os.environ.get('INCLUDE', '').split(os.pathsep)
    for path in paths:
        if os.path.exists(os.path.join(path, 'boost', 'version.hpp')):
            boost = path
            break

if boost is None:
    print 'Boost not found.'
    sys.exit(1)

HERE = os.path.dirname(__file__)

input  = os.path.normpath(HERE + '/../Goblin Camp/')
output = os.path.normpath(HERE + '/../vendor/boost/')

src = []
for root, dirs, files in os.walk(input):
    src.extend(os.path.join(root, fn) for fn in fnmatch.filter(files, '*.[ch]pp'))
for root, dirs, files in os.walk(os.path.join(output, 'libs')):
    src.extend(os.path.join(root, fn) for fn in fnmatch.filter(files, '*.cpp'))

CPP_INCLUDE = re.compile(r'''
    ^[#]\s*
    (?:
        include                                                       |
        define \s* BOOST_PP_(?:ITERATION_PARAMS|FILENAME|ITERATE)_\d+ |
        define \s* BOOST_PP_(?:LOCAL_ITERATE|INCLUDE_SELF)\(\)        |
        define \s* BOOST_(?:USER|COMPILER|PLATFORM|STDLIB)_CONFIG     |
        define \s* BOOST_ABI_(?:PRE|SUF)FIX
    )
    .*
    ([<"].*\.[hi]pp[">])
    .*
    $
''', re.I | re.X)

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

mpl = os.path.join(boost, 'boost', 'mpl', 'aux_', 'preprocessed')
for root, dirs, files in os.walk(mpl):
    for fn in files:
        found.add(os.path.normpath(os.path.join(root, fn)))

print '*** Boost headers'

for fn in sorted(found):
    name = fn[len(boost) + 1:]
    
    print '\t', name
    
    try:
        os.makedirs(os.path.join(output, os.path.dirname(name)))
    except OSError:
        pass
    
    shutil.copy(fn, os.path.join(output, name))
