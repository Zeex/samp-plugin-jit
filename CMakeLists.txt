cmake_minimum_required(VERSION 2.8)

project(jit)

list(APPEND CMAKE_MODULE_PATH "${jit_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)
include(GetGitRevisionDescription)

include_directories(".")
include_directories("amx")

set(ASMJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/asmjit")
set(ASMJIT_BUILD_LIBRARY TRUE)
add_definitions(-DASMJIT_API=)
include_directories(${ASMJIT_DIR})
add_subdirectory("asmjit")

add_library(jit MODULE
	"amx/amx.h"
	"amx/getch.h"
	"amx/sclinux.h"
	"amxplugin.cpp"
	"jit.cpp"
	"jit.h"
	"jump-x86.cpp"
	"jump-x86.h"
	"plugin.cpp"
	"plugin.h"
	"plugin.rc"
	"plugincommon.h"
)

target_link_libraries(jit AsmJit)

if(UNIX OR MINGW)
	list(APPEND COMPILE_FLAGS 
		"-Wno-attributes"
	)
	if(UNIX)
		add_definitions(-DLINUX)
	endif()
endif()

if(WIN32)
	if(MINGW)
		list(APPEND LINK_FLAGS "-Wl,--kill-at,--enable-stdcall-fixup")
	elseif(MSVC)
		list(APPEND LINK_FLAGS "/DEF:${jit_SOURCE_DIR}/plugin.def")
	endif()
endif()

foreach(flag ${COMPILE_FLAGS})
	set(COMPILE_FLAGS_ "${COMPILE_FLAGS_} ${flag}")
endforeach()
if(NOT ${COMPILE_FLAGS_} STREQUAL "")
	set_target_properties(jit PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS_})
endif()

foreach(flag ${LINK_FLAGS})
	set(LINK_FLAGS_ "${LINK_FLAGS_} ${flag}")
endforeach()
if(NOT ${LINK_FLAGS} STREQUAL "")
	set_target_properties(jit PROPERTIES LINK_FLAGS ${LINK_FLAGS_})
endif()

# Don't prefix output DLL with "lib"
set_target_properties(jit PROPERTIES PREFIX "")

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

set(PLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(PLUGIN_DESCRIPTION "JIT compiler plugin for SA-MP server")
set(PLUGIN_COPYRIGHT "Copyright (c) 2011-2012 Zeex")
set(PLUGIN_VERSION ${version})
set(PLUGIN_VERSION_RC ${version_rc})

configure_file(
	"${jit_SOURCE_DIR}/plugin.rc.in"
	"${jit_SOURCE_DIR}/plugin.rc"
	@ONLY
)
configure_file(
	"${jit_SOURCE_DIR}/version.h.in"
	"${jit_SOURCE_DIR}/version.h"
	@ONLY
)

message(STATUS "JIT plugin version: ${version}")

install(TARGETS jit LIBRARY DESTINATION ".")
install(FILES "README.txt" DESTINATION ".")

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
