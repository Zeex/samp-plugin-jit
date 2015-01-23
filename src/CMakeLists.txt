include(AMXConfig)
include(AddSAMPPlugin)

set(JIT_BACKENDS "asmjit" CACHE STRING "List of JIT backends to build")

foreach(backend IN LISTS JIT_BACKENDS)
  string(TOUPPER ${backend} backend)
  set(JIT_${backend} TRUE)
  add_definitions(-DJIT_${backend})
endforeach()

if(JIT_LLVM)
  find_package(LLVM REQUIRED)
endif()

include_directories(
  amx
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

if(JIT_LLVM)
  include_directories(${LLVM_INCLUDE_DIRS})
endif()

add_definitions(${LLVM_DEFINITIONS})

if(UNIX)
  add_definitions(-DAMXJIT_RELOCATE_OPCODES)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

configure_file(version.h.in version.h @ONLY)
configure_file(plugin.rc.in plugin.rc @ONLY)

set(JIT_SOURCES
  amx/amx.h
  amx/getch.h
  amx/sclinux.h
  amxplugin.cpp
  amxservice.h
  configreader.cpp
  configreader.h
  jit.h
  jit.cpp
  logprintf.cpp
  logprintf.h
  os.h
  plugin.cpp
  plugin.def
  plugin.h
  plugincommon.h
  ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

if(WIN32)
  list(APPEND JIT_SOURCES os-win32.cpp)
else()
  list(APPEND JIT_SOURCES os-unix.cpp)
endif()

add_samp_plugin(jit ${JIT_SOURCES})
set_property(TARGET jit APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG DEBUG)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET jit APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

if(MINGW)
  set_property(TARGET jit APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,--enable-stdcall-fixup")
endif()

if(JIT_LLVM)
  link_directories(${LLVM_LIBRARY_DIRS})
endif()

set(AMXJIT_BUILD_BACKENDS ${JIT_BACKENDS})
add_subdirectory(amxjit)

target_link_libraries(jit amxjit subhook)

install(TARGETS jit LIBRARY DESTINATION ".")
