set(ASMJIT_STATIC TRUE)
set(ASMJIT_INSTALL FALSE)
add_subdirectory(asmjit)

set(SUBHOOK_STATIC TRUE)
add_subdirectory(subhook)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY
             COMPILE_DEFINITIONS ASMJIT_STATIC SUBHOOK_STATIC)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY
             INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/asmjit/src
                                 ${CMAKE_CURRENT_SOURCE_DIR}/subhook)

file(RELATIVE_PATH this_folder ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET asmjit PROPERTY FOLDER ${this_folder})
set_property(TARGET subhook PROPERTY FOLDER ${this_folder})
