# globbing sources can be dangerous, but let's assert there are no stray
# source files in the tree.

# boost craps out with newer c++
file(GLOB boost_is_shit_src scripting/*.cpp scripting/_gcampapi/*.cpp)
#set_source_files_properties(${boost_is_shit_src} PROPERTIES COMPILE_FLAGS -std=c++14)

file(GLOB gc_src . *.cpp data/*.cpp UI/*.cpp)

file(GLOB gc_platform_src . platform/unix/*.cpp platform/linux/*.cpp)
file(GLOB gc_render_src . tileRenderer/*.cpp tileRenderer/sdl/*.cpp)
set(nonshit_src ${gc_src} ${gc_platform_src} ${gc_render_src})
#set_source_files_properties(${nonshit_src} PROPERTIES COMPILE_FLAGS -std=c++17)
add_executable (goblincamp ${nonshit_src} ${boost_is_shit_src})

# Make sure the compiler can find include files
include_directories (${GOBLINCAMP_SOURCE_DIR}/include)
include_directories (${GOBLINCAMP_SOURCE_DIR}/vendor/python-modules)

include_directories(/usr/include
                    /usr/include/libtcod
		    /usr/include/SDL2)

set (LIB_PATH /usr/lib)
find_library (PNG png ${LIB_PATH})
find_library (TCOD tcod ${LIB_PATH})

set(OpenGL_GL_PREFERENCE GLVND)
find_package (SDL2)
find_package (SDL2_image)
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

set (PYLIB_PATH /usr/lib/)
message("-- NOTE: CMake prefers python 3 to 2, so it is being specified manually.")
set (PYTHON_EXECUTABLE /usr/bin/python2)
set (PYTHON_INCLUDE_DIRS /usr/include/python2.7)
set (PYTHON_LIBRARIES ${PYLIB_PATH}/libpython2.7.so)

include_directories (${PYTHON_INCLUDE_DIRS})
link_directories (${PYLIB_PATH}/python2.7)

# TODO: update as in http://preney.ca/paul/archives/107
set (Boost_USE_MULTITHREAD ON)
set(Boost_PYTHON_VERSION 2.7)
find_package (Boost COMPONENTS date_time filesystem iostreams python random serialization system thread REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})

# link the executable to the libraries
target_link_libraries (goblincamp ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} GL png SDL2 SDL2_image tcod z python-modules pthread X11)

# have to use c++11 or gnu++11, because lambda expressions
set_target_properties (goblincamp PROPERTIES COMPILE_FLAGS "-Wall")

# add the install targets
install (TARGETS goblincamp DESTINATION bin)
