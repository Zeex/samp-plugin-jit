#include "test"

main() {
	f(0xdead, 0xbeef, 0xf00d);
}

f(...) {
	TEST_TRUE(numargs() == 3);
	TEST_TRUE(getarg(0) == 0xdead);
	TEST_TRUE(getarg(1) == 0xbeef);
	TestExit();
}
