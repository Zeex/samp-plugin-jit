include(PluginTest)

macro(prepare_test name)
  file(READ ${name}/output.txt TEST_OUTPUT)
  string(STRIP "${TEST_OUTPUT}" TEST_OUTPUT)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/full_output.txt.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt
  )
endmacro()

macro(test name)
  prepare_test(${name})
  add_plugin_test(${name}
    TARGET   plugin
    SCRIPT   ${CMAKE_CURRENT_SOURCE_DIR}/${name}/test
    OUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt
    TIMEOUT  0.5
  )
endmacro()

test(bugs/bug8)
test(bugs/bug21)
test(bugs/bug24)
test(bugs/bug30)
test(bugs/bug36)
test(bugs/jrel)

test(intrinsics/float)
test(intrinsics/floatabs)
test(intrinsics/floatadd)
test(intrinsics/floatsub)
test(intrinsics/floatmul)
test(intrinsics/floatdiv)
test(intrinsics/floatsqroot)
test(intrinsics/floatlog)

test(misc/frame_address)
test(misc/halt)
test(misc/halt_deep)
test(misc/indirect_jump)
test(misc/native_call)
test(misc/nested_exec)
test(misc/ojc)
test(misc/ojc_return_0)
test(misc/presence)
test(misc/return_value)
test(misc/switch)
