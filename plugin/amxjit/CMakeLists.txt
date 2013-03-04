include(AmxConfig)

set(AMXJIT_ALL_BACKENDS "asmjit")

if(NOT DEFINED AMXJIT_BUILD_BACKENDS)
  set(AMXJIT_BUILD_BACKENDS "all")
endif()

if(AMXJIT_BUILD_BACKENDS STREQUAL "all")
  set(AMXJIT_BUILD_BACKENDS ${AMSJIT_ALL_BACKENDS})
endif()

mark_as_advanced(AMXJIT_BUILD_BACKENDS)

set(sources
  amxptr.cpp
  amxptr.h
  callconv.h
  compiler.cpp
  compiler.h
  disasm.cpp
  disasm.h
  jit.cpp
  jit.h
  macros.h
  opcode.cpp
  opcode.h
)

foreach(backend IN LISTS AMXJIT_BUILD_BACKENDS)
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
