cmake_minimum_required(VERSION 2.8.6)
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
	"src/sdk/amx/amx.h"
	"src/sdk/amx/amxaux.c"
	"src/sdk/amx/amxaux.h"
	"src/sdk/amx/getch.h"
	"src/sdk/amx/sclinux.h"
	"src/sdk/plugin.h"
	"src/sdk/plugincommon.h"
	"src/sdk/amxplugin.cpp"
	"src/jit/amxdisasm.cpp"
	"src/jit/amxdisasm.h"
	"src/jit/amxopcode.h"
	"src/jit/amxscript.cpp"
	"src/jit/amxscript.h"
	"src/jit/jit.cpp"
	"src/jit/jit.h"
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
source_group("src\\jit" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/jit/[^/\\]+\\..*")
source_group("src\\sdk" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/[^/\\]+\\..*")
source_group("src\\sdk\\amx" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/amx/[^/\\]+\\..*")

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

if(CMAKE_COMPILER_IS_GNUCXX)
	set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
		COMPILE_FLAGS " -Wno-attributes -Wno-ignored-attributes")
endif()

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

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ".")

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
