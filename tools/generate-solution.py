# This tool generates VS2010 solution and projects for Goblin Camp
# source tree.
#
# NOTE: Generated project will be a "makefile" project, that will
# call bjam to do the build.
import sys, os, re, itertools, uuid, pickle, collections

HERE = os.path.dirname(os.path.realpath(__file__))
ROOT = os.path.normpath(HERE + '/..')

tmpDir  = os.path.join(ROOT, 'build', 'tmp')
projDir = os.path.join(ROOT, 'Goblin Camp')

jobs   = int(sys.argv[1]) if len(sys.argv) >= 2 else 4
config = os.path.realpath(sys.argv[2]) if len(sys.argv) >= 3 else None

def genUUID():
    # cannot be lambda, must be pickable
    return '{{{0}}}'.format(str(uuid.uuid4()).upper())

variables = {}

# gather files
sources   = []
headers   = []
resources = []

CPP_RE = re.compile('^.*?[.]c(?:c|pp|pp_in)?$', re.S | re.I)
HPP_RE = re.compile('^.*?[.]h(?:h|pp)?$', re.S | re.I)
DAT_RE = re.compile('^.*?[.](?:dat|py|txt|rc_in|rch)?$', re.S | re.I)

for root, dirs, files in os.walk(projDir):
    for fn in files:
        file = os.path.join(root, fn), root[len(projDir) + 1:]
        if HPP_RE.match(fn) is not None:
            headers.append(file)
        elif CPP_RE.match(fn) is not None:
            sources.append(file)
        elif DAT_RE.match(fn) is not None:
            resources.append(file)

key = lambda x: x[1]
sources.sort(key = key)
headers.sort(key = key)
resources.sort(key = key)

# avoid regenerating UUIDs
try:
    os.makedirs(tmpDir)
except OSError:
    pass

uuidCache = os.path.join(tmpDir, '.uuids')
if os.path.exists(uuidCache):
    uuids = pickle.load(open(uuidCache, 'rb'))
else:
    uuids = collections.defaultdict(genUUID)

# generate filters
filters = set(filter for _, filter in sources + headers + resources if filter)
filters = [(filter, uuids[filter]) for filter in filters]
filters.sort(key = lambda x: x[0])

# project/solution UUIDs
solutionID = uuids['__solution__']
projectID  = uuids['__project__']

pickle.dump(uuids, open(uuidCache, 'wb'), pickle.HIGHEST_PROTOCOL)

# NB: filters are effective only in a separate .filters file
templ = lambda X, D: '\n\t\t'.join('<{0} Include="{1}"><Filter>{2}</Filter></{0}>'.format(X, os.path.realpath(fn), filter) for fn, filter in D)

msvc_bjam = 'bjam {0}-j{1}'.format(
    '--user-config={0} '.format(config) if config is not None else '', jobs
)

# XXX: preprocessor defines are hardcoded in the template

variables = {
    'DIST':          os.path.join(ROOT, 'build'),
    'ROOT':          projDir,
    'SOLUTION_UUID': solutionID,
    'PROJECT_UUID':  projectID,
    'DEBUG_BJAM':    '{0} variant=debug'.format(msvc_bjam),
    'RELEASE_BJAM':  '{0} variant=release'.format(msvc_bjam),
    'SOURCE_FILES':  templ('ClCompile', sources),
    'HEADER_FILES':  templ('ClInclude', headers),
    'OTHER_FILES':   templ('None',      resources),
    'FILTERS':       '\n\t\t'.join(
        '<Filter Include="{0}"><UniqueIdentifier>{1}</UniqueIdentifier></Filter>'.format(f, u) for f, u in filters
    ),
}

templateDir = os.path.join(HERE, 'vs2010-templates')
for fn in os.listdir(templateDir):
    out = fn[:-3]
    
    print '** Creating:', os.path.join(projDir, out)
    
    with open(os.path.join(templateDir, fn), 'rb') as fp:
        template = fp.read()
    
    for k, v in variables.iteritems():
        template = template.replace('%{0}%'.format(k), v)
    
    with open(os.path.join(projDir, out), 'wb') as fp:
        fp.write(template)
