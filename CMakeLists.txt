cmake_minimum_required(VERSION 2.8)

project(jit)

set(JIT_VERSION_MAJOR  0)
set(JIT_VERSION_MINOR  3)
set(JIT_VERSION_PATCH  3)

list(APPEND CMAKE_MODULE_PATH "${jit_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)

include_directories("./")
include_directories("./amx")

set(ASMJIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/asmjit)
set(ASMJIT_BUILD_LIBRARY TRUE)
add_definitions(-DASMJIT_API=)
include_directories(${ASMJIT_DIR})
add_subdirectory(asmjit)

add_library(jit MODULE
	"amx/amx.h"
	"amx/getch.h"
	"amx/sclinux.h"
	"amxplugin.cpp"
	"configreader.cpp"
	"configreader.h"
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

set(PLUGIN_NAME        ${CMAKE_PROJECT_NAME})
set(PLUGIN_DESCRIPTION "JIT plugin")
set(PLUGIN_COPYRIGHT   "Copyright (c) 2012 Zeex")
set(PLUGIN_VERSION     ${JIT_VERSION_MAJOR}.${JIT_VERSION_MINOR}.${JIT_VERSION_PATCH})
set(PLUGIN_VERSION_RC  ${JIT_VERSION_MAJOR},${JIT_VERSION_MINOR},${JIT_VERSION_PATCH},0)

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

install(TARGETS jit LIBRARY DESTINATION ".")
install(FILES "README.txt" DESTINATION ".")

if(MSVC)
	set(PDB_NAME "jit.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ".")
endif()

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${JIT_VERSION_STRING}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${JIT_VERSION_STRING}-linux")
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
