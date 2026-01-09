cmake_minimum_required(VERSION 3.25)

project(RequirementsManager CXX)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig)
find_package(Boost 1.90 CONFIG QUIET COMPONENTS uuid)
find_package(cereal CONFIG QUIET)
find_package(FRTypes CONFIG QUIET)
set(USING_BOOST_EXTERNAL FALSE)
set(USING_CEREAL_EXTERNAL FALSE)

SET(FRRequirementsManager_VERSION_MAJOR 0)
set(FRRequirementsManager_VERSION_MINOR 1)
set(FRRequirementsManager_VERSION ${FRRequirementsManager_VERSION_MAJOR}.${FRRequirementsManager_VERSION_MINOR})

include(ExternalProject)

set (EXTERNAL_INCLUDE_DIRS "")
set (PYTHON_LINK_LIBRARIES "")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/external_projects.cmake")

option(BUILD_TESTS "Build unit tests" OFF)

#
# The Python API requires nanobind, which you can
# download and build separately from
# https://github.com/wjakob/nanobind. I would
# set this up as an external project but it's got
# some cmake integration I don't really want to
# ferret out of an external project to use.
#
option(BUILD_PYTHON_API "Build python API" ON)

# Build the REST server, currently only in the
# python API.
option(BUILD_REST_SERVER "Build REST server into the Python API" ON)

#
# The pqxx support requires libpqxx, which you can
# download and build separately from
# https://github.com/jtv/libpqxx. That in turn
# needs a postgresql dev package.
#
# This is currently only used for createtables
# and a couple of tests in the test directory
#
option(BUILD_PQXX_SUPPORT "Build PostgreSQL database support" ON)

set(HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/fr/RequirementsManager")

set(DATA_HEADER_LIST
  "${CMAKE_CURRENT_SOURCE_DIR}/include/fr/RequirementsManager.h"
  "${HEADER_DIR}/CommitableNode.h"
  "${HEADER_DIR}/GraphNode.h"
  "${HEADER_DIR}/Node.h"
  "${HEADER_DIR}/NodeConnector.h"
  "${HEADER_DIR}/Organization.h"
  "${HEADER_DIR}/Product.h"
  "${HEADER_DIR}/Project.h"
  "${HEADER_DIR}/Requirement.h"
  "${HEADER_DIR}/Story.h"
  "${HEADER_DIR}/TaskNode.h"
  "${HEADER_DIR}/ThreadPool.h"
  "${HEADER_DIR}/UseCase.h"
  "${HEADER_DIR}/UtilityNodes.h"
)

set(DATABASE_HEADER_LIST
  "${HEADER_DIR}/PqDatabase.h"
  "${HEADER_DIR}/PqDatabaseSpecific.h"
  "${HEADER_DIR}/PqNodeFactory.h"
)

set(LIBRARY_SOURCE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/NodeConnector.cpp"
)

# If we're not building the enscripten wasm bindings,
# build the library and python bindings as usual

if (NOT EMSCRIPTEN)
  set(Boost_USE_STATIC_LIBS ON)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGE_VENDOR "Bruce Ide")
  set(CPACK_PACKAGE_CONTACT "Bruce Ide")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Bruce Ide")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RequirementsManager Binaries and Python API")
  set(CPACK_PACKAGE_VERSION "${FRRequirementsManager_VERSION}")
  include(CPack)

  set(INSTALL_TARGET_LIST "")

  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(nanobind CONFIG REQUIRED)
  find_package(libpqxx CONFIG QUIET)

  if (BUILD_REST_SERVER)
    # If this isn't found you can install Pistache or disable
    # BUILD_REST_SERVER in your build
    pkg_check_modules(Pistache REQUIRED IMPORTED_TARGET libpistache)
    list(APPEND PYTHON_LINK_LIBRARIES PkgConfig::Pistache)
  endif()

  # Option list for python API
  set(PYTHON_API_OPTION_LIST "")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/external_projects_native.cmake")
  
  message(STATUS "Building C++ native library and python bindings")
  
  add_library(RequirementsManager SHARED
    ${LIBRARY_SOURCE}
  )  
  add_library(FR::RequirementsManager ALIAS RequirementsManager)

  list(APPEND INSTALL_TARGET_LIST RequirementsManager)

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
    ${EXTERNAL_INCLUDE_DIRS}
  )

  # Python support
  
  if (BUILD_PYTHON_API)
    
    nanobind_add_module(FRRequirements
      NB_STATIC STABLE_ABI FREE_THREADED
      ${CMAKE_CURRENT_SOURCE_DIR}/src/RequirementsManagerPythonApi.cpp
    )
    
    target_include_directories(FRRequirements PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
      ${EXTERNAL_INCLUDE_DIRS}
    )

    list(APPEND INSTALL_TARGET_LIST FRRequirements)
    list(APPEND PYTHON_LINK_LIBRARIES "FR::RequirementsManager")
    
    if (BUILD_PQXX_SUPPORT)
      list(APPEND PYTHON_LINK_LIBRARIES "libpqxx::pqxx")
    endif()
    
    target_link_libraries(FRRequirements PUBLIC
      ${PYTHON_LINK_LIBRARIES}
    )
    
    if (BUILD_REST_SERVER)
      list(APPEND PYTHON_API_OPTION_LIST -DBUILD_REST_SERVER)      
    endif()
    if (BUILD_PQXX_SUPPORT)
      list(APPEND PYTHON_API_OPTION_LIST -DINCLUDE_PQXX_SUPPORT)
    endif()
    
    target_compile_options(FRRequirements PUBLIC ${PYTHON_API_OPTION_LIST})
    
  endif()

  # SQL Support
  
  if (BUILD_PQXX_SUPPORT)
    add_executable(CreateTables
      "${CMAKE_CURRENT_SOURCE_DIR}/src/CreateTables.cpp"
    )

    list(APPEND INSTALL_TARGET_LIST CreateTables)
    # Funnily enough don't need requirements manager ndoes for this
    target_link_libraries(CreateTables PUBLIC
      libpqxx::pqxx
    )
  endif()

  if (BUILD_TESTS)
    add_subdirectory(test)
  endif()
  
  if (BUILD_REST_SERVER)
    find_package(Boost COMPONENTS program_options REQUIRED)
    
    add_executable(RestServer
      "${CMAKE_CURRENT_SOURCE_DIR}/src/RestService.cpp"
    )

    list(APPEND INSTALL_TARGET_LIST RestServer)
    
    target_link_libraries(RestServer PRIVATE
      FR::RequirementsManager
      libpqxx::pqxx
      PkgConfig::Pistache
      Boost::program_options
    )
  endif()

  # Install Support
  
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FRRequirementsManagerConfigVersion.cmake"
    VERSION "${FRRequirementsManager_VERSION}"
    COMPATIBILITY AnyNewerVersion
  )

  configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FRRequirementsManagerConfig.cmake"
    INSTALL_DESTINATION lib/cmake/FRRequirementsManager
  )

  install(TARGETS ${INSTALL_TARGET_LIST}
    EXPORT RequirementsManager_export
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
  )

  INSTALL(EXPORT RequirementsManager_export
    FILE RequirementsManagerTargts.cmake
    NAMESPACE FR::
    DESTINATION lib/cmake/FRRequirementsManager
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FRRequirementsManagerConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FRRequirementsManagerConfig.cmake"
    DESTINATION lib/cmake/FRRequirementsManager
  )
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
    ${EXTERNAL_INCLUDE_DIRS}
  )
  
  add_executable(FRRequirementsManagerModule
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmscriptenApi.cpp
  )

  target_link_libraries(FRRequirementsManagerModule PUBLIC RequirementsManager)
  add_dependencies(RequirementsManager BoostExternal CerealExternal FRTypesExternal)

  target_link_options(FRRequirementsManagerModule PRIVATE
    "-sSINGLE_FILE=1"
    "-sMODULARIZE"
    "-sEXPORT_NAME=FRRequirementsManager"
    "--bind"
    "-sFETCH=1"
  )
  if (NOT PROJECT_IS_TOP_LEVEL)
    set(EXTERNAL_INCLUDE_DIRS "${EXTERNAL_INCLUDE_DIRS}" PARENT_SCOPE)
  endif()

endif()
