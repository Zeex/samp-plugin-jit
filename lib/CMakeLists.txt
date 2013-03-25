set(ASMJIT_STATIC TRUE)
add_subdirectory("asmjit")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY INCLUDE_DIRECTORIES
             "${CMAKE_CURRENT_SOURCE_DIR}/asmjit/src/")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY COMPILE_DEFINITIONS
             "ASMJIT_API=")

set(SUBHOOK_STATIC TRUE)
add_subdirectory("subhook")
