#include <jit>
#include "test"

Test() {
	TEST_TRUE(IsJITPresent());
	TEST_TRUE(__JIT);
}

public OnJITCompile() {
	TEST_TRUE(!IsJITPresent());
	TEST_TRUE(__JIT);
	return 1;
}

main() {
	Test();
	TestExit();
}
