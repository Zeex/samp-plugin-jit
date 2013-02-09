include(AmxConfig)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/amx
)

add_library(sdk STATIC
  amx/amx.h
  amx/getch.h
  amx/sclinux.h
  amxplugin.cpp
  plugin.h
  plugincommon.h
)