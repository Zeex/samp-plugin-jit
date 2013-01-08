include(SampPluginTest)

macro(test name)
	file(READ "${name}/output.txt" TEST_OUTPUT)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/full_output.txt.in"
		"${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt"
	)
	add_samp_plugin_test("${name}"
		SCRIPT   "${CMAKE_CURRENT_SOURCE_DIR}/${name}/test"
		TARGET   "${PROJECT_NAME}"
		OUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt"
	)
endmacro()

test("bugs/bug8")
test("bugs/bug21")
test("bugs/bug24")
test("bugs/bug30")

test("intrinsics/float")
test("intrinsics/floatabs")
test("intrinsics/floatadd")
test("intrinsics/floatsub")
test("intrinsics/floatmul")
test("intrinsics/floatdiv")
test("intrinsics/floatsqroot")
test("intrinsics/floatlog")

test("misc/native_call")
