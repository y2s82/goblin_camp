# gc-sln generators helper.
# Requires Python 2.6.
#
# This script reads its input, replaces %VARIABLES%, and saves output.
#  - genvcproj <GC_BUILD_ROOT> <GC_PROJECT_ROOT> <input> <output> <user-config file>
# Expanded variables:
#  - %GC_SOURCE_FILES% to XMLised list of Goblin Camp sources, and _version.cpp_in template
#  - %GC_HEADER_FILES% to XMLised list of Goblin Camp headers
#  - %GC_RESOURCE_FILES% to XMLised list of *.dat files, and _version.rc_in template
#
# NOTE: Generated project will be a "makefile" project, that will
# call bjam to do the build. Sources and headers are included only
# for navigation from within IDE.
import sys, os, re, itertools, uuid, pickle, collections

# XXX: Not as good as it could be.

# Project will be put to build\msvc. Source root is ..\..\Goblin Camp.
assert len(sys.argv) >= 5, 'Do not run directly, use build system instead.'

buildRoot, projectRoot, input, output = sys.argv[1:5]
userConfig = os.path.realpath(sys.argv[5]) if len(sys.argv) == 6 else None

def genUUID():
    # cannot be lambda, must be pickable
    return '{{{0}}}'.format(str(uuid.uuid4()).upper())

variables = {}

# gather files
sources   = [(os.path.join(buildRoot, 'templates', '_version.cpp_in'), r'templates')]
headers   = []
resources = [(os.path.join(buildRoot, 'templates', '_version.rc_in'), r'templates')]

CPP_RE = re.compile('^.*?[.]c(?:c|pp)?$', re.S | re.I)
HPP_RE = re.compile('^.*?[.]h(?:h|pp)?$', re.S | re.I)
DAT_RE = re.compile('^.*?[.](?:dat|py|txt)?$', re.S | re.I)

for root, dirs, files in os.walk(projectRoot):
    for fn in files:
        file = os.path.join(root, fn), root[len(projectRoot) + 1:]
        if HPP_RE.match(fn) is not None:
            headers.append(file)
        elif CPP_RE.match(fn) is not None:
            sources.append(file)
        elif DAT_RE.match(fn) is not None:
            resources.append(file)

key = lambda x: os.path.basename(x[0])
sources.sort(key = key)
headers.sort(key = key)
resources.sort(key = key)

# avoid regenerating UUIDs
uuidCache = os.path.join(os.path.dirname(output), '.uuids')
if os.path.exists(uuidCache):
    uuids = pickle.load(open(uuidCache, 'rb'))
else:
    uuids = collections.defaultdict(genUUID)

# generate filters
filters = set(filter for _, filter in sources + headers + resources if filter)
filters = [(filter, uuids[filter]) for filter in filters]

# project/solution UUIDs
solutionID = uuids['__solution__']
projectID  = uuids['__project__']

pickle.dump(uuids, open(uuidCache, 'wb'), pickle.HIGHEST_PROTOCOL)

# NB: filters are effective only in a separate .filters file
templ = lambda X, D: '\n\t\t\t'.join('<{0} Include="{1}"><Filter>{2}</Filter></{0}>'.format(X, os.path.realpath(fn), filter) for fn, filter in D)

msvc_bjam = 'msvc_bjam.cmd {0}-j2 dist'.format(
    '--user-config={0} '.format(userConfig) if userConfig is not None else ''
)

# XXX: preprocessor defines are hardcoded in the template

variables = {
    'DIST':          os.path.join(buildRoot, 'dist'),
    'ROOT':          projectRoot,
    'SOLUTION_UUID': solutionID,
    'PROJECT_UUID':  projectID,
    'DEBUG_BJAM':    '{0} variant=debug'.format(msvc_bjam),
    'RELEASE_BJAM':  '{0} variant=release'.format(msvc_bjam),
    'SOURCE_FILES':  templ('ClCompile', sources),
    'HEADER_FILES':  templ('ClInclude', headers),
    'OTHER_FILES':   templ('None',      resources),
    'FILTERS':       '\n\t\t\t'.join(
        '<Filter Include="{0}"><UniqueIdentifier>{1}</UniqueIdentifier></Filter>'.format(f, u) for f, u in filters
    ),
}

with open(input, 'r') as fp:
    template = fp.read()

for key, value in variables.iteritems():
    template = template.replace('%{0}%'.format(key), value)

with open(output, 'w') as fp:
    fp.write(template)
