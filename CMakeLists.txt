cmake_minimum_required(VERSION 2.8)

project(sampjit)

include(CheckIncludeFile)

check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
	add_definitions(-DHAVE_ALLOCA_H)
endif()
check_include_file(inttypes.h HAVE_INTTYPES_H)
if(HAVE_INTTYPES_H)
	add_definitions(-DHAVE_INTTYPES_H)
endif()
check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
	add_definitions(-DHAVE_MALLOC_H)
endif()
check_include_file(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
	add_definitions(-DHAVE_STDINT_H)
endif()

add_library(jit MODULE
	amx/amx.h
	amx/getch.h
	amx/sclinux.h
	jitasm/jitasm.h
	plugin.cpp
	plugin.h
	plugincommon.h
	jit.cpp
	jit.h
)

set_target_properties(jit PROPERTIES PREFIX "")

if(UNIX)
	add_definitions(-DLINUX)
else()
	set(def_path "${sampjit_SOURCE_DIR}/plugin.def")
	if(MINGW)
		set_target_properties(jit PROPERTIES
			LINK_FLAGS "-Wl,--kill-at --def ${def_path}")
	elseif(MSVC)
		set_target_properties(jit PROPERTIES
			LINK_FLAGS "/DEF:${def_path}")
	endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set_target_properties(jit PROPERTIES
		COMPILE_FLAGS "-fno-operator-names")
endif()
