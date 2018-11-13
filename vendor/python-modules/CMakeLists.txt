find_package (ZLIB)

# TODO: proper search for python, perhaps from top-level CMakeLists
include_directories (/usr/include/python2.7)

file (GLOB pm_src . *.c)
add_library (python-modules ${pm_src})
