cmake_minimum_required(VERSION 2.8)
project(jit)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	include("cmake/GlobalBuildConfig.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules"
)

include(GetGitRevisionDescription)
include(SampPlugin)
include(CTest)

add_subdirectory("lib")

include_directories("src/SDK")
include_directories("src/SDK/amx")

set(SOURCES
	"src/jit.cpp"
	"src/jit.h"
	"src/SDK/amx/amx.h"
	"src/SDK/amx/amxaux.c"
	"src/SDK/amx/amxaux.h"
	"src/SDK/amx/getch.h"
	"src/SDK/amx/sclinux.h"
	"src/SDK/plugin.h"
	"src/SDK/plugincommon.h"
	"src/SDK/amxplugin.cpp"
	"src/amxpathfinder.cpp"
	"src/amxpathfinder.h"
	"src/configreader.cpp"
	"src/configreader.h"
	"src/fileutils.cpp"
	"src/fileutils.h"
	"src/os.h"
	"src/options.cpp"
	"src/options.h"
	"src/plugin.cpp"
	"src/version.h"
)

if(MSVC)
	set_source_files_properties("src/plugin.cpp" "src/SDK/amx/amxaux.c" PROPERTIES
		COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS")
endif()

if(WIN32)
	list(APPEND SOURCES
		"src/os-win32.cpp"
		"src/plugin.def"
		"src/plugin.rc"
	)
elseif(UNIX)
	list(APPEND SOURCES
		"src/os-unix.cpp"
	)
endif()

source_group("lib" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/lib/[^/\\]+\\..*")
source_group("src" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/[^/\\]+\\..*")
source_group("src\\SDK" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/[^/\\]+\\..*")
source_group("src\\SDK\\amx" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/amx/[^/\\]+\\..*")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

git_describe(description --match v[0-9]*.[0-9]**)
if(description)
	string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
	string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
else()
	message(STATUS "Failed to get version from Git, will read VERSION.txt")
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" version)
	string(STRIP ${version} version)
endif()

string(REGEX REPLACE "[.-]" "," version_rc ${version})
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc ${version_rc})

set(PROJECT_VERSION    ${version})
set(PROJECT_VERSION_RC ${version_rc})

add_samp_plugin(${PROJECT_NAME} ${SOURCES})

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/version.h"
	@ONLY
)

if(WIN32)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.rc.in"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.rc"
		@ONLY
	)
endif()

target_link_libraries(${PROJECT_NAME} asmjit subhook)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}")

if(MSVC)
	set(PDB_NAME "${PROJECT_NAME}.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ".")
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-linux")
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)

add_subdirectory("tests")
