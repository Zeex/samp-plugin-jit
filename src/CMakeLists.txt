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
configure_file(pluginversion.h.in pluginversion.h @ONLY)

set(SAMP_SDK_SOURCES
  amx/amx.h
  amx/getch.h
  amx/sclinux.h
  amxplugin.cpp
  plugin.h
  plugincommon.h
)

set(JIT_SOURCES
  ${SAMP_SDK_SOURCES}
  jit.cpp
  jit.def
  jithandler.h
  jithandler.cpp
  logprintf.cpp
  logprintf.h
  ${CMAKE_CURRENT_BINARY_DIR}/jit.rc
  ${CMAKE_CURRENT_BINARY_DIR}/pluginversion.h
)

add_samp_plugin(jit ${JIT_SOURCES})

add_samp_plugin(jit_sleep
  ${SAMP_SDK_SOURCES}
  jit_sleep.cpp
  jit_sleep.def
)

add_subdirectory(amxjit)
target_link_libraries(jit amxjit configreader subhook)

install(TARGETS jit LIBRARY DESTINATION ".")
