# CMakeLists.txt for goblincamp
# TODO: blame

cmake_minimum_required (VERSION 2.6)

project (GOBLINCAMP)

# The version number.
set (GOBLINCAMP_VERSION_MAJOR 0)
set (GOBLINCAMP_VERSION_MINOR 22)

# TODO: configure version here, use in source
configure_file (
  "${GOBLINCAMP_SOURCE_DIR}/version.hpp.in"
  "${GOBLINCAMP_BINARY_DIR}/version.hpp"
  )

# add the binary tree to the search path for include files
# so that we will find version.hpp
#include_directories("${PROJECT_BINARY_DIR}")

#include_directories ("${PROJECT_SOURCE_DIR}/include")

# damned snake
add_subdirectory (vendor/python-modules)
#add_subdirectory (lib)

# main goblincamp stuff
add_subdirectory (src)

#install (FILES "${GOBLICAMP_BINARY_DIR}/version.hpp"
#         DESTINATION include)

SET (CMAKE_INSTALL_PREFIX "./pkg/usr")
install (DIRECTORY lib DESTINATION share/goblincamp)
install (FILES terminal.png DESTINATION share/goblincamp)
