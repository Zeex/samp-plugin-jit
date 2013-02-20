include(AmxConfig)
include(SampPlugin)

include_directories(.)
include_directories(amx)
include_directories(subhook)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DSUBHOOK_STATIC)

if(UNIX)
  add_definitions(-DLINUX)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

configure_file(version.h.in version.h @ONLY)
configure_file(plugin.rc.in plugin.rc @ONLY)

add_samp_plugin(plugin
  amx/amx.h
  amx/getch.h
  amx/sclinux.h
  amxplugin.cpp
  plugin.cpp
  plugin.def
  plugin.h
  plugincommon.h
  ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

set_property(TARGET plugin APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG DEBUG)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET plugin APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

if(MINGW)
  set_property(TARGET plugin APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,--enable-stdcall-fixup")
endif()

add_subdirectory(amxjit)

set(SUBHOOK_STATIC TRUE)
add_subdirectory(subhook)

target_link_libraries(plugin amxjit subhook)

install(TARGETS plugin LIBRARY DESTINATION ".")
