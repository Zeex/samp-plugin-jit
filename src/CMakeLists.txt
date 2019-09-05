include(AMXConfig)
include(AddSAMPPlugin)

include_directories(
  amx
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

if(UNIX)
  add_definitions(-DAMXJIT_RELOCATE_OPCODES)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

configure_file(jit.rc.in jit.rc @ONLY)
configure_file(version.h.in version.h @ONLY)

set(JIT_SOURCES
  amx/amx.h
  amx/getch.h
  amx/sclinux.h
  amxhandler.h
  amxplugin.cpp
  jit.cpp
  jit.def
  jithandler.h
  jithandler.cpp
  logprintf.cpp
  logprintf.h
  os.h
  plugin.h
  plugincommon.h
  ${CMAKE_CURRENT_BINARY_DIR}/jit.rc
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

if(WIN32 OR CYGWIN)
  list(APPEND JIT_SOURCES os-win32.cpp)
else()
  list(APPEND JIT_SOURCES os-unix.cpp)
endif()

add_samp_plugin(jit ${JIT_SOURCES})

if(BUILD_SLEEP_PLUGIN)
  add_samp_plugin(jit-sleep
    amx/amx.h
    amx/getch.h
    amx/sclinux.h
    amxplugin.cpp
    jit-sleep.cpp
    jit-sleep.def
    plugin.h
    plugincommon.h
  )
endif()

add_subdirectory(amxjit)
target_link_libraries(jit amxjit configreader subhook)

install(TARGETS jit LIBRARY DESTINATION ".")
