include(SampPluginTest)

macro(test name)
	file(READ "${name}.out.in" TEST_OUTPUT)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/test.out.in"
		"${CMAKE_CURRENT_SOURCE_DIR}/${name}.out"
	)
	add_samp_plugin_test(${name} TARGET ${PROJECT_NAME} OUT_FILE "${name}.out")
endmacro()

test("intrinsics/float")
test("intrinsics/floatabs")
test("intrinsics/floatadd")
test("intrinsics/floatsub")
test("intrinsics/floatmul")
test("intrinsics/floatdiv")
test("intrinsics/floatsqroot")
