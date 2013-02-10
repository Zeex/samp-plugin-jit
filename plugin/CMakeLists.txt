include(AmxConfig)
include(SampPlugin)

include_directories(.)
include_directories(jit/asmjit/src)
include_directories(sdk)
include_directories(sdk/amx)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
  add_definitions(-DLINUX)
endif()

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

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET plugin APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

add_subdirectory(jit)
add_subdirectory(sdk)

target_link_libraries(plugin jit sdk subhook)

install(TARGETS plugin LIBRARY DESTINATION ".")
