# External projects used by the native build
#
# This should be in a "if (NOT EMSCRIPTEN)" block after all the
# find_packages have been run for the various packages we're
# using. Currently I'm not trying to build nanobind, so
# we're just doing pqxx at the moment.

include(FetchContent)

if (BUILD_PQXX_SUPPORT AND NOT libpqxx_FOUND)

  # Since libpqxx has cmake support and is not a header
  # only library, I'm using FetchContent for it. Then I
  # can just pull the targets in as part of my main build.
  # libpqxx nicely does an alias target in its build so
  # it should work the same way whether it's found with
  # find_package or built via FetchContent

  set(SKIP_BUILD_TEST TRUE)
  set(BUILD_SHARED_LIBS ON)
  
  FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY "https://github.com/jtv/libpqxx.git"
  )

  FetchContent_MakeAvailable(libpqxx)

endif()
