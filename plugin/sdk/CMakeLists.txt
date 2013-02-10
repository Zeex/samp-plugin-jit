include(AmxConfig)

if(UNIX)
  add_definitions(-DLINUX)
endif()

include_directories(amx)

add_library(sdk STATIC
  amx/amx.h
  amx/getch.h
  amx/sclinux.h
  amxplugin.cpp
  plugin.h
  plugincommon.h
)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET sdk APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()
