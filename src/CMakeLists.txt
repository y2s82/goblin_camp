# globbing sources can be dangerous, but let's assert there are no stray
# source files in the tree.
file(GLOB gc_src . *.cpp data/*.cpp scripting/*.cpp scripting/_gcampapi/*.cpp UI/*.cpp)
file(GLOB gc_platform_src . platform/unix/*.cpp platform/linux/*.cpp)
file(GLOB gc_render_src . tileRenderer/*.cpp tileRenderer/sdl/*.cpp)
add_executable (goblincamp ${gc_src} ${gc_platform_src} ${gc_render_src})

# Make sure the compiler can find include files
include_directories (${GOBLINCAMP_SOURCE_DIR}/include)
include_directories (${GOBLINCAMP_SOURCE_DIR}/vendor/python-modules)

include_directories(/usr/include
                    /usr/include/libtcod
		    /usr/include/SDL)

set (LIB_PATH /usr/lib)
find_library (PNG png ${LIB_PATH})
find_library (TCOD tcod ${LIB_PATH})

find_package (SDL)
find_package (SDL_image)
find_package (ZLIB)

# TODO: which ones are needed?
find_package (GLEW)
find_package (OpenGL)
find_package (GLUT)

# TODO: fix python selection
#set (PythonLibs_FIND_VERSION 2.7.5)
#set (Python_ADDITIONAL_VERSIONS 2.7)
#find_package (PythonLibs REQUIRED)
#message (${PYTHON_INCLUDE_DIRS})
#message (${PYTHON_LIBRARIES})
#include_directories (${PYTHON_INCLUDE_DIRS})
#link_directories (${PYTHON_LIBRARY_DIRS})

set (PYLIB_PATH /usr/lib/x86_64-linux-gnu)
message("-- NOTE: CMake prefers python 3 to 2, so it is being specified manually.")
set (PYTHON_EXECUTABLE /usr/bin/python2)
set (PYTHON_INCLUDE_DIRS /usr/include/python2.7)
set (PYTHON_LIBRARIES ${PYLIB_PATH}/libpython2.7.so)

include_directories (${PYTHON_INCLUDE_DIRS})
link_directories (${PYLIB_PATH}/python2.7)

# TODO: update as in http://preney.ca/paul/archives/107
set (Boost_USE_MULTITHREAD ON)
find_package (Boost COMPONENTS date_time filesystem iostreams python random serialization system thread REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})

# link the executable to the libraries
target_link_libraries (goblincamp ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} GL png SDL SDL_image tcod z python-modules pthread)

# have to use c++11 or gnu++11, because lambda expressions
set_target_properties (goblincamp PROPERTIES COMPILE_FLAGS "-std=gnu++11 -Wall")

# add the install targets
install (TARGETS goblincamp DESTINATION bin)
