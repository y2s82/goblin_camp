# Make sure the compiler can find include files
include_directories (${GOBLINCAMP_SOURCE_DIR}/include)

# also the special python-modules bindings
include_directories (${GOBLINCAMP_SOURCE_DIR}/vendor/python-modules)

# Make sure the linker can find object files once they are built.
#link_directories (${GOBLINCAMP_BINARY_DIR}/Libs)

# glob goblincamp source files. This can be dangerous, but let's assert
# there are no stray source files in the tree.
file(GLOB gc_src . *.cpp data/*.cpp scripting/*.cpp scripting/_gcampapi/*.cpp UI/*.cpp)
file(GLOB gc_platform_src . platform/unix/*.cpp platform/linux/*.cpp)
file(GLOB gc_render_src . tileRenderer/*.cpp tileRenderer/sdl/*.cpp)
add_executable (goblincamp ${gc_src} ${gc_platform_src} ${gc_render_src})

include_directories(/usr/include
                    /usr/include/libtcod
		    /usr/include/SDL)

SET (LIB_PATH /usr/lib)
find_library(PNG png ${LIB_PATH})
find_library(TCOD tcod ${LIB_PATH})
find_library(TCOD tcodgui ${LIB_PATH})
find_library(TCOD tcodxx ${LIB_PATH})

FIND_PACKAGE(SDL)
FIND_PACKAGE(SDL_image)
FIND_PACKAGE(ZLIB)

# TODO: remove unneeded
#FIND_PACKAGE(GLEW)
FIND_PACKAGE(OpenGL)
#FIND_PACKAGE(GLUT)


# TODO: fix python selection
#SET(PythonLibs_FIND_VERSION 2.7.5)
#SET(Python_ADDITIONAL_VERSIONS 2.7)
# find_package(PythonLibs REQUIRED)
# message(${PYTHON_INCLUDE_DIRS})
# message(${PYTHON_LIBRARIES})

#INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
#LINK_DIRECTORIES(${PYTHON_LIBRARY_DIRS})

message("cmake prefers python 3 to 2, so it is being specified manually")
SET(PYTHON_EXECUTABLE /usr/bin/python2)
SET(PYTHON_INCLUDE_DIRS /usr/include/python2.7)
SET(PYTHON_LIBRARIES /usr/lib/libpython2.7.so)

include_directories(${PYTHON_INCLUDE_DIRS})
link_directories(/usr/lib64/python2.7)

# TODO: update as in http://preney.ca/paul/archives/107
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE(Boost COMPONENTS date_time filesystem iostreams python random serialization system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Link the executable to the libraries.
target_link_libraries (goblincamp ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} GL png SDL SDL_image tcod tcodgui tcodxx z python-modules)

# have to use c++11 or gnu++11, because lambda expressions
set_target_properties(goblincamp PROPERTIES COMPILE_FLAGS "-std=gnu++11 -Wall")
