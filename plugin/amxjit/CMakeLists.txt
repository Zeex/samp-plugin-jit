include(AmxConfig)

set(AMXJIT_BACKENDS "asmjit" CACHE STRING "List of backends to build")

set(sources
  amxdisasm.cpp
  amxdisasm.h
  amxopcode.cpp
  amxopcode.h
  amxptr.cpp
  amxptr.h
  callconv.h
  compiler.cpp
  compiler.h
  jit.cpp
  jit.h
  macros.h
)

foreach(backend IN LISTS AMXJIT_BACKENDS)
  list(APPEND sources compiler-${backend}.cpp compiler-${backend}.h)
endforeach()

add_library(amxjit STATIC ${sources})

set_property(TARGET amxjit APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG DEBUG)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET amxjit APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

target_link_libraries(amxjit asmjit)
