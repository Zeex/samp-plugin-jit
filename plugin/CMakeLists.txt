include(AmxConfig)
include(SampPlugin)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

configure_file(version.h.in version.h @ONLY)
configure_file(plugin.rc.in plugin.rc @ONLY)

add_samp_plugin(plugin
  plugin.cpp
  plugin.def
  ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries(plugin jit sdk subhook)

install(TARGETS plugin LIBRARY DESTINATION ".")