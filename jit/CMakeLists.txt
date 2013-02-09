include(AmxConfig)

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

target_link_libraries(jit asmjit sdk)
