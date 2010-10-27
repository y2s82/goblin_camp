# gc-sln generators helper.
# Requires Python 2.6.
#
# This script reads its input, replaces %VARIABLES%, and saves output.
#  - genvcproj 2008/2010/sln <GC_SOURCE_ROOT> <input> <output>
# Expanded variables:
#  - %GC_SOURCE_FILES% to XMLised list of Goblin Camp sources, and _version.cpp_in template
#  - %GC_HEADER_FILES% to XMLised list of Goblin Camp headers
#  - %GC_RESOURCE_FILES% to XMLised list of *.dat files, and _version.rc_in template
#
# NOTE: Generated project will be a "makefile" project, that will
# call bjam to do the build. Sources and headers are included only
# for navigation from within IDE.
import sys, os, fnmatch, itertools

# XXX: Not as good as it could be.

# Project will be put to build\msvc. Source root is ..\..\Goblin Camp.
assert len(sys.argv) >= 5, 'Do not run directly, use build system instead.'

mode, root, input, output = sys.argv[1:5]
userconfig = os.path.realpath(sys.argv[5]) if len(sys.argv) == 6 else None

def getFiles(dir, pattern):
    return fnmatch.filter(os.listdir(dir), pattern)

VS2008_FILE_TPL    = '<File RelativePath="{0}"/>'
VS2010_SRCFILE_TPL = '<ClCompile Include="{0}"/>'
VS2010_HDRFILE_TPL = '<ClInclude Include="{0}"/>'
VS2010_RESFILE_TPL = '<None Include="{0}"/>'

variables = {}

if mode != 'sln':
    sources   = [r'..\templates\_version.cpp_in']
    headers   = []
    resources = [r'..\templates\_version.rc_in']
    
    sources.extend(itertools.imap(
        lambda x: r'..\..\Goblin Camp\src\{0}'.format(x),
        getFiles(os.path.join(root, 'src'), '*.cpp')
    ))
    
    sources.extend(itertools.imap(
        lambda x: r'..\..\Goblin Camp\src\win32\{0}'.format(x),
        getFiles(os.path.join(root, 'src', 'win32'), '*.cpp')
    ))
    
    headers.extend(itertools.imap(
        lambda x: r'..\..\Goblin Camp\include\{0}'.format(x),
        getFiles(os.path.join(root, 'include'), '*.hpp')
    ))
    
    resources.extend(itertools.imap(
        lambda x: r'..\..\Goblin Camp\{0}'.format(x),
        getFiles(root, '*.dat') + getFiles(root, '*.ini')
    ))
    
    sources.sort()
    headers.sort()
    resources.sort()
    
    userconfig = '' if userconfig is None else '--user-config={0} '.format(userconfig)
    command    = 'msvc_bjam.cmd {0}-j2 dist'.format(userconfig)
    
    variables['GC_DEBUG_COMMAND']   = '{0} variant=debug'.format(command)
    variables['GC_RELEASE_COMMAND'] = '{0} variant=release'.format(command)
    
    if mode == '2008':
        variables['GC_SOURCE_FILES']   = '\n\t\t\t'.join(VS2008_FILE_TPL.format(fn) for fn in sources)
        variables['GC_HEADER_FILES']   = '\n\t\t\t'.join(VS2008_FILE_TPL.format(fn) for fn in headers)
        variables['GC_RESOURCE_FILES'] = '\n\t\t\t'.join(VS2008_FILE_TPL.format(fn) for fn in resources)
    elif mode == '2010':
        variables['GC_SOURCE_FILES']   = '\n\t\t'.join(VS2010_SRCFILE_TPL.format(fn) for fn in sources)
        variables['GC_HEADER_FILES']   = '\n\t\t'.join(VS2010_HDRFILE_TPL.format(fn) for fn in headers)
        variables['GC_RESOURCE_FILES'] = '\n\t\t'.join(VS2010_RESFILE_TPL.format(fn) for fn in resources)
    else:
        print 'Invalid command line.'
        sys.exit(255)

with open(input, 'r') as fp:
    template = fp.read()

for key, value in variables.iteritems():
    template = template.replace('%{0}%'.format(key), value)

with open(output, 'w') as fp:
    fp.write(template)
