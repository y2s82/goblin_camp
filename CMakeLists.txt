# CMakeLists.txt for goblincamp
# TODO: blame

cmake_minimum_required (VERSION 2.6)

# TODO: configure depending on OS
set (CMAKE_INSTALL_PREFIX "./pkg/usr")

project (GOBLINCAMP)
set (GOBLINCAMP_VERSION_MAJOR 0)
set (GOBLINCAMP_VERSION_MINOR 22)

# TODO: configure version here, use in source
configure_file (
  "${GOBLINCAMP_SOURCE_DIR}/version.hpp.in"
  "${GOBLINCAMP_BINARY_DIR}/version.hpp"
  )

# add so that we will find version.hpp
#include_directories ("${PROJECT_BINARY_DIR}")

#install (FILES "${GOBLICAMP_BINARY_DIR}/version.hpp"
#         DESTINATION include)

# TODO: find out if these can be sourced from system
add_subdirectory (vendor/python-modules)

# main source tree
add_subdirectory (src)

# add static files to distribution
install (DIRECTORY lib DESTINATION share/goblincamp)
install (FILES terminal.png DESTINATION share/goblincamp)
