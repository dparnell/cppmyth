cmake_minimum_required (VERSION 2.6 FATAL_ERROR)
cmake_policy (VERSION 2.6)

add_subdirectory (${CMAKE_SOURCE_DIR}/cppmyth)
add_subdirectory (${CMAKE_SOURCE_DIR}/test)
add_subdirectory (${CMAKE_SOURCE_DIR}/demo)
add_subdirectory (${CMAKE_SOURCE_DIR}/fuse)

###############################################################################
# install targets
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
