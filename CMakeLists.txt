cmake_minimum_required(VERSION 3.25)

project(RequirementsManager CXX)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig)
find_package(Boost 1.90 CONFIG QUIET COMPONENTS uuid)
find_package(cereal CONFIG QUIET)
find_package(FRTypes CONFIG QUIET)
set(USING_BOOST_EXTERNAL FALSE)
set(USING_CEREAL_EXTERNAL FALSE)

include(ExternalProject)

if (NOT Boost_FOUND)
  set(USING_BOOST_EXTERNAL TRUE)
  if (WINDOWS)
    set(BOOST_BOOTSTRAP "./bootstrap.bat")
  else()
    set(BOOST_BOOTSTRAP "./bootstrap.sh")
  endif()

  if (EMSCRIPTEN)
    set(BOOST_EMSCRIPTEN_TOOLSET="toolset=empscripten")
  else()
    set(BOOST_EMSCRIPTEN_TOOLSET="")
  endif()

  set(BOOST_EXTERNAL_PREFIX "BoostExternal")
  set(BOOST_EXTERNAL_SOURCE "${BOOST_EXTERNAL_PREFIX}/source")
  set(BOOST_EXTERNAL_INSTALL "${BOOST_EXTERNAL_PREFIX}/install")
  
  # Build Boost Depdendencies. This is very likely to e
  # required in emscripten or for the rare C++ developer
  # who doesn't actually have Boost installed on their
  # system. I just need uuid for this project, so
  # it should be fairly painless to build as part of
  # this system. I'm planning to only build it statically
  # though, since the main use case of this fork in the
  # code is emscripten and that'll want static libraries.
  
  ExternalProject_Add(BoostExternal
    GIT_REPOSITORY "https://github.com/boostorg/boost.git"
    GIT_SUBMODULES "libs/uuid" "libs/config" "libs/system" "libs/headers" "libs/type_traits" "libs/throw_exception" "libs/assert" "libs/static_assert" "tools/build" "tools/boost_install"
    GIT_SUBMODULES_RECURSE FALSE
    PREFIX "${BOOST_EXTERNAL_PREFIX}"
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND ${BOOST_BOOTSTRAP} --prefix=<INSTALL_DIR>
    BUILD_COMMAND "./b2" "${BOOST_EMSCRIPTEN_TOOLSET}" "variant=release" "link=static" "--with-uuid" "--build-dir=${CMAKE_CURRENT_BINARY_DIR}/${BOOST_EXTERNAL_BUILD}" --prefix=<INSTALL_DIR> install
     INSTALL_COMMAND ""       
  )

  ExternalProject_Get_Property(BoostExternal INSTALL_DIR)  
  set(Boost_INCLUDE_DIRS "${INSTALL_DIR}/include")
    
endif()

if (NOT cereal_FOUND)
  set(USING_CEREAL_EXTERNAL TRUE)
  set(CEREAL_EXTERNAL_PREFIX "CerealExternal")
  set(CEREAL_EXTERNAL_SOURCE "${CEREAL_EXTERNAL_PREFIX}/source")
  set(CEREAL_EXTERNAL_BUILD "${CEREAL_EXTERNAL_PREFIX}/build")
  set(CEREAL_EXTERNAL_INSTALL "${CEREAL_EXTERNAL_PREFIX}/install")
  
  ExternalProject_Add(CerealExternal
    GIT_REPOSITORY "https://github.com/USCiLab/cereal.git"
    PREFIX "${CEREAL_EXTERNAL_PREFIX}"
    SOURCE_DIR "${CEREAL_EXTERNAL_SOURCE}"
    BINARY_DIR "${CEREAL_EXTERNAL_BUILD}"
    INSTALL_DIR "${CEREAL_EXTERNAL_INSTALL}"
    CMAKE_ARGS -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  )

  ExternalProject_Get_Property(CerealExternal INSTALL_DIR)
  set(cereal_INCLUDE_DIRS "${INSTALL_DIR}/include")
endif()

if (NOT FRTypes_FOUND)
  set(USING_FRTYPES_EXTERNAL TRUE)
  set(FRTYPES_EXTERNAL_PREFIX "FrtypesExternal")
  set(FRTYPES_EXTERNAL_SOURCE "${FRTYPES_EXTERNAL_PREFIX}/source")
  set(FRTYPES_EXTERNAL_BUILD "${FRTYPES_EXTERNAL_PREFIX}/build")
  set(FRTYPES_EXTERNAL_INSTALL "${FRTYPES_EXTERNAL_PREFIX}/insstall")

  ExternalProject_Add(FRTypesExternal
    GIT_REPOSITORY "https://github.com/FlyingRhenquest/types.git"
    GIT_TAG "main"
    PREFIX "${FRTYPES_EXTERNAL_PREFIX}"    
    SOURCE_DIR "${FRTYPES_EXTERNAL_SOURCE}"
    BINARY_DIR "${FRTYPES_EXTERNAL_BUILD}"
    INSTALL_DIR "${FRTYPES_EXTERNAL_INSTALL}"
    CMAKE_ARGS -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  )

  ExternalProject_Get_Property(FRTypesExternal INSTALL_DIR)
  set(FRTypes_INSTALL_DIR "${INSTALL_DIR}/include")
endif()

option(BUILD_TESTS "Build unit tests" ON)
set(HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/fr/RequirementsManager")

set(DATA_HEADER_LIST
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

# If we're not building the enscripten wasm bindings,
# build the library and python bindings as usual

if (NOT EMSCRIPTEN)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(nanobind CONFIG REQUIRED)
  find_package(libpqxx REQUIRED)
  
  message(STATUS "Building C++ native library and python bindings")
  
  add_library(RequirementsManager SHARED
    ${LIBRARY_SOURCE}
  )  
  
  add_library(FR::RequirementsManager ALIAS RequirementsManager)
  nanobind_add_module(FRRequirements
    NB_STATIC STABLE_ABI FREE_THREADED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RequirementsManagerPythonApi.cpp
  )

  set_target_properties(RequirementsManager PROPERTIES
    FRAMEWORK TRUE
    PUBLIC_HEADER "${DATA_HEADER_LIST}"
  )

  target_include_directories(RequirementsManager
    PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${cereal_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )

  target_include_directories(FRRequirements PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )

  target_link_libraries(FRRequirements PUBLIC
    FR::RequirementsManager
    libpqxx::pqxx
  )

  add_executable(CreateTables
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CreateTables.cpp"
  )

  # Funnily enough don't need requirements manager ndoes for this
  target_link_libraries(CreateTables PUBLIC
    libpqxx::pqxx
  )    

  if (BUILD_TESTS)
    add_subdirectory(test)
  endif()
else()
  message(STATUS "Bulding emscripten bindings")
  set(EMSCRIPTEN_LIBRARY_SOURCE
    ${LIBRARY_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmscriptenApi.cpp
  )

  add_library(RequirementsManager STATIC
    ${LIBRARY_SOURCE}
  )

  add_library(FR::RequirementsManager ALIAS RequirementsManager)

  set_target_properties(RequirementsManager PROPERTIES
    OUTPUT_NAME FRRequirementsManager_wasm
  )

  target_include_directories(RequirementsManager PUBLIC
    "${Boost_INCLUDE_DIRS}"
    "${cereal_INCLUDE_DIRS}"
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )
  
  add_executable(FRRequirementsManagerModule
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmscriptenApi.cpp
  )

  target_link_libraries(FRRequirementsManagerModule PRIVATE RequirementsManager)

  target_link_options(FRRequirementsManagerModule PRIVATE "-sSINGLE_FILE=1" "-sMODULARIZE" "-sEXPORT_NAME=FRRequirementsManager" "--bind")

endif()
