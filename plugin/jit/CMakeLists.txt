include(AmxConfig)

include_directories(asmjit/src)

add_definitions(-DASMJIT_STATIC)

add_library(jit STATIC
  amxdisasm.cpp
  amxdisasm.h
  amxopcode.cpp
  amxopcode.h
  amxptr.cpp
  amxptr.h
  backend.h
  backend-asmjit.cpp
  backend-asmjit.h
  callconv.h
  compiler.cpp
  compiler.h
  jit.cpp
  jit.h
  macros.h
)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET jit APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

set(ASMJIT_STATIC TRUE)
add_subdirectory(asmjit)

target_link_libraries(jit asmjit sdk)
