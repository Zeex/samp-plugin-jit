cmake_minimum_required(VERSION 2.8)

project(jit)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)
include(GetGitRevisionDescription)

set(ASMJIT_BUILD_STATIC_LIBRARY TRUE)
add_subdirectory("lib/asmjit")

include_directories("lib/asmjit")
include_directories("src/SDK")
include_directories("src/SDK/amx")

add_definitions(-DASMJIT_STATIC)

set(PLUGIN_NAME "${CMAKE_PROJECT_NAME}")
set(PLUGIN_DEF "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.def")
set(PLUGIN_DESCRIPTION "JIT plugin for SA-MP server")
set(PLUGIN_COPYRIGHT "Copyright (c) 2011-2012 Zeex")

set(PLUGIN_SOURCES
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
	"src/hook.cpp"
	"src/hook.h"
	"src/os.h"
	"src/options.cpp"
	"src/options.h"
	"src/plugin.cpp"
	"src/plugin.def"
	"src/plugin.rc"
	"src/version.h"
)

if(MSVC)
	set_source_files_properties("src/plugin.cpp" "src/SDK/amx/amxaux.c" PROPERTIES
		COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS")
endif()

if(WIN32)
	list(APPEND PLUGIN_SOURCES "src/hook-win32.cpp")
	list(APPEND PLUGIN_SOURCES "src/os-win32.cpp")
elseif(UNIX)
	list(APPEND PLUGIN_SOURCES "src/hook-unix.cpp")
	list(APPEND PLUGIN_SOURCES "src/os-unix.cpp")
endif()

source_group("lib" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/lib/[^/\\]+\\..*")
source_group("src" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/[^/\\]+\\..*")
source_group("src\\SDK" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/[^/\\]+\\..*")
source_group("src\\SDK\\amx" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/amx/[^/\\]+\\..*")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET AsmJit PROPERTY FOLDER "lib")

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

set(PLUGIN_VERSION ${version})
set(PLUGIN_VERSION_RC ${version_rc})

include(SampPlugin)

target_link_libraries(${PLUGIN_TARGET} AsmJit)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.rc.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.rc"
	@ONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/version.h"
	@ONLY
)

install(TARGETS ${PLUGIN_TARGET} LIBRARY DESTINATION ".")

if(MSVC)
	set(PDB_NAME "jit.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ".")
endif()

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PLUGIN_VERSION}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PLUGIN_VERSION}-linux")
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
