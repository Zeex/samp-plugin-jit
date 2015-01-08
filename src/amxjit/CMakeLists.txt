include(AmxConfig)

set(AMXJIT_ALL_BACKENDS "asmjit;llvm")

if(NOT DEFINED AMXJIT_BUILD_BACKENDS)
  set(AMXJIT_BUILD_BACKENDS "all")
endif()

if(AMXJIT_BUILD_BACKENDS STREQUAL "all")
  set(AMXJIT_BUILD_BACKENDS ${AMSJIT_ALL_BACKENDS})
endif()

mark_as_advanced(AMXJIT_BUILD_BACKENDS)

foreach(backend IN LISTS AMXJIT_BUILD_BACKENDS)
  string(TOUPPER ${backend} backend)
  set(AMXJIT_${backend} TRUE)
endforeach()

set(AMXJIT_SOURCES
  amxref.cpp
  amxref.h
  compiler.cpp
  compiler.h
  cstdint.h
  disasm.cpp
  disasm.h
  logger.cpp
  logger.h
  macros.h
  opcode.cpp
  opcode.h
)

foreach(backend IN LISTS AMXJIT_BUILD_BACKENDS)
  list(APPEND AMXJIT_SOURCES compiler-${backend}.cpp compiler-${backend}.h)
endforeach()

add_library(amxjit STATIC ${AMXJIT_SOURCES})

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET amxjit APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

if(AMXJIT_ASMJIT)
  target_link_libraries(amxjit asmjit)
endif()

if(AMXJIT_LLVM)
  llvm_map_components_to_libraries(LLVM_LIBRARIES JIT Support X86)
  target_link_libraries(amxjit ${LLVM_LIBRARIES})
endif()
