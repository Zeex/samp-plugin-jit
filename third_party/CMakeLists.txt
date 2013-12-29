set(ASMJIT_STATIC TRUE)
set(SUBHOOK_STATIC TRUE)

add_definitions(-DASMJIT_API=)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY COMPILE_DEFINITIONS
             ASMJIT_API= SUBHOOK_STATIC)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/asmjit/asmjit/src
            ${CMAKE_CURRENT_SOURCE_DIR}/subhook)

add_subdirectory(asmjit/asmjit)
add_subdirectory(subhook)

file(RELATIVE_PATH this_folder ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET asmjit PROPERTY FOLDER ${this_folder})
set_property(TARGET subhook PROPERTY FOLDER ${this_folder})