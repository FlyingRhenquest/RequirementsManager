cmake_minimum_required(VERSION 3.25)

project(RequirementsManager)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig)
find_package(Boost 1.86 CONFIG REQUIRED COMPONENTS uuid)
find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(nanobind CONFIG REQUIRED)

option(BUILD_TESTS "Build unit tests" ON)
set(HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/fr/RequirementsManager")

set(HEADER_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/include/fr/RequirementsManager.h"
  "${HEADER_DIR}/CommitableNode.h"
  "${HEADER_DIR}/Node.h"
  "${HEADER_DIR}/NodeConnector.h"
  "${HEADER_DIR}/Organization.h"
  "${HEADER_DIR}/Product.h"
  "${HEADER_DIR}/Project.h"
  "${HEADER_DIR}/Requirement.h"
  "${HEADER_DIR}/Story.h"
  "${HEADER_DIR}/UseCase.h"
  "${HEADER_DIR}/UtilityNodes.h"
)

set(LIBRARY_SOURCE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/NodeConnector.cpp"
)

add_library(RequirementsManager SHARED
  ${LIBRARY_SOURCE}
)
add_library(FR::RequirementsManager ALIAS RequirementsManager)

set_target_properties(RequirementsManager PROPERTIES
  FRAMEWORK TRUE
  PUBLIC_HEADER "${HEADER_LIST}"
)

target_include_directories(RequirementsManager
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

nanobind_add_module(FRRequirements
  NB_STATIC STABLE_ABI FREE_THREADED
  ${CMAKE_CURRENT_SOURCE_DIR}/src/RequirementsManagerPythonApi.cpp
)

target_include_directories(FRRequirements PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(FRRequirements PUBLIC FR::RequirementsManager)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
