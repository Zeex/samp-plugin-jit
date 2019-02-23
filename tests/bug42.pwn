#include "test"

// Fixes warning 205: redundant code: constant expression is zero
stock mod(n, m) {
	return n % m;
}

main() {
	TEST_TRUE(mod(-5, 10) == 5);
	TEST_TRUE(mod(5, -10) == -5);
	TEST_TRUE(mod(-5, -10) == -5);
	TEST_TRUE(mod(5, 10) == 5);
}
