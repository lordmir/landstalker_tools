CMAKE_MINIMUM_REQUIRED(VERSION 3.28)

PROJECT(landstalker_tools VERSION 1.0.0)

option(LANDSTALKER_BUILD_SHARED "Build liblandstalker as a shared library" ON)
option(INSTALL_DEPS "Automatically download and build dependencies" ON)

include(dependencies.cmake)

UpdateSubmodules()
InstallDependencies()
add_subdirectory(modules/liblandstalker EXCLUDE_FROM_ALL)

CONFIGURE_FILE(landstalker_tools.h.in landstalker_tools.h)

SET(TCLAP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tclap/include")
SET(RAPIDCSV_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcsv/src")
INCLUDE_DIRECTORIES(${TCLAP_INCLUDE_DIR} ${RAPIDCSV_INCLUDE_DIR})
ADD_SUBDIRECTORY(src)

INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "${landstalker_tools_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${landstalker_tools_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${landstalker_tools_VERSION_PATCH}")
INCLUDE(CPack)
