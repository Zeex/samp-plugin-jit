project(jit)

cmake_minimum_required(VERSION 2.8.6)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

include(GetGitRevisionDescription)
include(CTest)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  include(cmake/GlobalBuildConfig.cmake)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

git_describe(description --match v[0-9]*.[0-9]**)
if(description)
  string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
  string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
else()
  message(STATUS "Failed to get version from Git, will read VERSION.txt")
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt version)
  string(STRIP ${version} version)
endif()

string(REGEX REPLACE "[.-]" "," version_rc ${version})
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc ${version_rc})

set(PROJECT_VERSION    ${version})
set(PROJECT_VERSION_RC ${version_rc})

add_subdirectory(third_party)

add_subdirectory(plugin)
set_target_properties(plugin PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

add_subdirectory(tests)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
if(WIN32)
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-win32)
  set(CPACK_GENERATOR ZIP)
elseif(UNIX)
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-linux)
  set(CPACK_GENERATOR TGZ)
endif()

include(CPack)