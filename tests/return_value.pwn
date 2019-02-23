#include "test"

public f();

main() {
	TEST_TRUE(CallLocalFunction("f", "") == 133);
}

public f() {
	return g() + 10;
}

g() {
	return 123;
}
