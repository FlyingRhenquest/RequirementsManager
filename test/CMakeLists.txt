cmake_minimum_required(VERSION 3.25)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
include(GoogleTest)

if (NOT TARGET FR::RequirementsManager)
  find_package(FRRequirementsManager CONFIG REQUIRED)
endif()

set(TEST_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/DatabaseTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NodeTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CommitableNodeTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RequirementTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/OrganizationTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NodeFactoryTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ProjectTest.cpp
)

add_executable(RequirementsManagerTests
  ${TEST_SRC}
)

target_include_directories(RequirementsManagerTests PUBLIC
  ${Boost_INCLUDE_DIRS}
)

target_link_libraries(RequirementsManagerTests PUBLIC
  GTest::GTest
  GTest::Main
  FR::RequirementsManager
  libpqxx::pqxx
)
