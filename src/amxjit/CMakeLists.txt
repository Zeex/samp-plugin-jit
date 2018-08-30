include(AMXConfig)

set(AMXJIT_SOURCES
  amxref.cpp
  amxref.h
  compiler.cpp
  compiler.h
  compiler_impl.cpp
  compiler_impl.h
  cstdint.h
  disasm.cpp
  disasm.h
  logger.cpp
  logger.h
  macros.h
  opcode.cpp
  opcode.h
)

add_library(amxjit STATIC ${AMXJIT_SOURCES})

if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.0")
    set_property(TARGET amxjit APPEND_STRING PROPERTY
                 COMPILE_FLAGS " -Wno-attributes")
  endif()
endif()

target_link_libraries(amxjit asmjit)
