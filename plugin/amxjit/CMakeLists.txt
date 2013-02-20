include(AmxConfig)

include_directories(asmjit/src)

add_definitions(-DASMJIT_API=)

add_library(amxjit STATIC
  amxdisasm.cpp
  amxdisasm.h
  amxopcode.cpp
  amxopcode.h
  amxptr.cpp
  amxptr.h
  callconv.h
  compiler.cpp
  compiler.h
  compiler-asmjit.cpp
  compiler-asmjit.h
  jit.cpp
  jit.h
  macros.h
)

set_property(TARGET amxjit APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG DEBUG)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET amxjit APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

set(ASMJIT_STATIC TRUE)
add_subdirectory(asmjit)

target_link_libraries(amxjit asmjit sdk)
