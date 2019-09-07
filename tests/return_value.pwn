// OUTPUT: All tests passed

#include "test"

public f();

main() {
	TEST_TRUE(CallLocalFunction("f", "") == 133);
	TestExit();
}

public f() {
	return g() + 10;
}

g() {
	return 123;
}
