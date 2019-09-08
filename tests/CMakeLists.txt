include(AddSAMPPluginTestPR)

find_package(PawnCC REQUIRED)
find_package(PluginRunner REQUIRED)

set(_amx_files "")

macro(add_test_script name)
  set(_compile_flags "")
  foreach(line ${_test_code})
    string(REGEX MATCHALL "FLAGS: .*" flags ${line})
    if(flags)
      string(REPLACE "FLAGS: " "" flags ${flags})
      list(APPEND _compile_flags ${flags})
    endif()
  endforeach()

  list(APPEND _compile_flags
    "${CMAKE_CURRENT_SOURCE_DIR}/${name}.pwn"
    "-\;+"
    "-(+"
    "-i${PluginRunner_DIR}/include"
    "-i${PROJECT_SOURCE_DIR}/include"
    "-o${CMAKE_CURRENT_BINARY_DIR}/${name}"
  )
  string(REPLACE ";" " " _compile_flags_str "${_compile_flags}")

  if(UNIX)
    string(REPLACE "\;" "\\$<SEMICOLON>" _compile_flags "${_compile_flags}")
    string(REPLACE "(" "\\(" _compile_flags "${_compile_flags}")
  endif()

  add_custom_command(
    OUTPUT            ${CMAKE_CURRENT_BINARY_DIR}/${name}.amx
    COMMAND           ${PawnCC_EXECUTABLE} ${_compile_flags}
    COMMENT           "Compiling test ${name}: ${PawnCC_EXECUTABLE} ${_compile_flags_str}"
    DEPENDS           ${name}.pwn test.inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  list(APPEND _amx_files ${CMAKE_CURRENT_BINARY_DIR}/${name}.amx)
endmacro()

file(STRINGS test.list _tests)

foreach(name ${_tests})
  file(STRINGS ${name}.pwn _test_code)

  set(_test_output "")
  foreach(line ${_test_code})
    string(REGEX MATCHALL "OUTPUT: .*" output ${line})
    if(output)
      string(REPLACE "OUTPUT: " "" output ${output})
      set(_test_output "${_test_output}${output}\n")
    endif()
  endforeach()

  string(REPLACE "<TEST_OUTPUT>" "\n${_test_output}" _full_test_output "
Loaded plugin: .*jit.*
Loaded script: .*${name}.*<TEST_OUTPUT>"
  )
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${name}.out" ${_full_test_output})

  add_test_script(${name})

  set(_targets jit)
  set(_env PATH=${_path})

  if(name MATCHES sleep)
    list(APPEND _targets jit_sleep)
    list(APPEND _env JIT_SLEEP=1)
  endif()

  add_samp_plugin_test(${name}
    TARGETS            ${_targets}
    SCRIPT             ${CMAKE_CURRENT_BINARY_DIR}/${name}
    OUTPUT_FILE        ${CMAKE_CURRENT_BINARY_DIR}/${name}.out
    TIMEOUT            3
    WORKING_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}
  )

  if(WIN32)
    set(_path "${PluginRunner_DIR};$ENV{Path}")
    string(REPLACE ";" "\\$<SEMICOLON>" _path "${_path}")
  else()
    set(_path "${PluginRunner_DIR}:$ENV{PATH}")
  endif()

  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT ${_env})
endforeach()

add_custom_target(jit_tests ALL DEPENDS ${_amx_files})
