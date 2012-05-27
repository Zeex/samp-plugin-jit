cmake_minimum_required(VERSION 2.8)

project(jit)

list(APPEND CMAKE_MODULE_PATH "${jit_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)
include(GetGitRevisionDescription)

set(ASMJIT_BUILD_STATIC_LIBRARY TRUE)
add_subdirectory("jit/asmjit")

include_directories("jit")
include_directories("jit/asmjit")
include_directories("plugin/SDK")
include_directories("plugin/SDK/amx")

add_definitions(-DASMJIT_STATIC)

set(PLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(PLUGIN_DEF "${CMAKE_CURRENT_SOURCE_DIR}/plugin/plugin.def")
set(PLUGIN_DESCRIPTION "JIT plugin for SA-MP server")
set(PLUGIN_COPYRIGHT "Copyright (c) 2011-2012 Zeex")

set(PLUGIN_SOURCES
	"jit/jit.cpp"
	"jit/jit.h"
	"plugin/SDK/amx/amx.h"
	"plugin/SDK/amx/amxaux.c"
	"plugin/SDK/amx/amxaux.h"
	"plugin/SDK/amx/getch.h"
	"plugin/SDK/amx/sclinux.h"
	"plugin/SDK/plugin.h"
	"plugin/SDK/plugincommon.h"
	"plugin/SDK/amxplugin.cpp"
	"plugin/amxname.cpp"
	"plugin/amxname.h"
	"plugin/configreader.cpp"
	"plugin/configreader.h"
	"plugin/jump-x86.cpp"
	"plugin/jump-x86.h"
	"plugin/options.cpp"
	"plugin/options.h"
	"plugin/plugin.cpp"
	"plugin/plugin.def"
	"plugin/plugin.rc"
	"plugin/version.h"
)

source_group("jit" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/jit/[^/\\]+\\..*")
source_group("plugin" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/plugin/[^/\\]+\\..*")
source_group("plugin\\SDK" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/plugin/SDK/[^/\\]+\\..*")
source_group("plugin\\SDK\\amx" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/plugin/SDK/amx/[^/\\]+\\..*")

find_package(Git)
if(GIT_FOUND)
	# Get latest version tag from Git history
	git_describe(description --match v[0-9]*.[0-9]**)
	string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
	string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
endif()

if(version STREQUAL "")
	# Read version from VERSION.txt (updated on each release)
	message(STATUS "Failed to get version from Git, will read VERSION.txt")
	file(READ "${jit_SOURCE_DIR}/VERSION.txt" version)
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
	"${jit_SOURCE_DIR}/plugin/plugin.rc.in"
	"${jit_SOURCE_DIR}/plugin/plugin.rc"
	@ONLY
)
configure_file(
	"${jit_SOURCE_DIR}/plugin/version.h.in"
	"${jit_SOURCE_DIR}/plugin/version.h"
	@ONLY
)

install(TARGETS jit LIBRARY DESTINATION ".")

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
