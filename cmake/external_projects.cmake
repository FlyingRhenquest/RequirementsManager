# External projects for RequirementsManager (Common to both
# native and emscripten)


###############################################################
#                        -= BOOST =-

if (NOT Boost_FOUND)
  message(STATUS "boost libraries not found -- building as external.")
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
  
  # Build Boost Depdendencies. This is very likely to be
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
    # Everything after this point is needed by signals2
    # TODO: Find a replacement for signals2
    "libs/signals2" "libs/smart_ptr" "libs/core" "libs/move" "libs/optional" "libs/function" "libs/bind" "libs/mpl" "libs/preprocessor" "libs/iterator" "libs/mp11" "libs/variant" "libs/detail" "libs/type_index" "libs/container_hash" "libs/utility" "libs/integer" "libs/parameter"
    GIT_SUBMODULES_RECURSE FALSE
    PREFIX "${BOOST_EXTERNAL_PREFIX}"
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND ${BOOST_BOOTSTRAP} --prefix=<INSTALL_DIR>
    BUILD_COMMAND "./b2" "${BOOST_EMSCRIPTEN_TOOLSET}" "variant=release" "link=static" "--with-uuid" "--build-dir=${CMAKE_CURRENT_BINARY_DIR}/${BOOST_EXTERNAL_BUILD}" --prefix=<INSTALL_DIR> install
     INSTALL_COMMAND ""       
  )

  ExternalProject_Get_Property(BoostExternal INSTALL_DIR)  
  set(Boost_INCLUDE_DIRS "${INSTALL_DIR}/include")
  list(APPEND EXTERNAL_INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
endif()

##############################################################
#                  -= Cereal =-

if (NOT cereal_FOUND)
  message(STATUS "cereal library not found -- building as external")
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
  list(APPEND EXTERNAL_INCLUDE_DIRS "${cereal_INCLUDE_DIRS}")
endif()

##############################################################
#                 -= FRTypes =-

if (NOT FRTypes_FOUND)
  message(STATUS "FlyingRhenquest Type Library not found -- building as external")
  set(USING_FRTYPES_EXTERNAL TRUE)
  set(FRTYPES_EXTERNAL_PREFIX "FrtypesExternal")
  set(FRTYPES_EXTERNAL_SOURCE "${FRTYPES_EXTERNAL_PREFIX}/source")
  set(FRTYPES_EXTERNAL_BUILD "${FRTYPES_EXTERNAL_PREFIX}/build")
  set(FRTYPES_EXTERNAL_INSTALL "${FRTYPES_EXTERNAL_PREFIX}/install")

  ExternalProject_Add(FRTypesExternal
    GIT_REPOSITORY "https://github.com/FlyingRhenquest/types.git"
    GIT_TAG "main"
    PREFIX "${FRTYPES_EXTERNAL_PREFIX}"    
    SOURCE_DIR "${FRTYPES_EXTERNAL_SOURCE}"
    BINARY_DIR "${FRTYPES_EXTERNAL_BUILD}"
    INSTALL_DIR "${FRTYPES_EXTERNAL_INSTALL}"
    CMAKE_ARGS -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  )

  ExternalProject_Get_Property(FRTypesExternal INSTALL_DIR)
  set(FRTypes_INCLUDE_DIR "${INSTALL_DIR}/include")
  list(APPEND EXTERNAL_INCLUDE_DIRS "${FRTypes_INCLUDE_DIR}")
endif()

