// OUTPUT: All tests passed

#include "test"

main() {
	TEST_TRUE(float(-1) == -1.0);
	TEST_TRUE(float(0) == 0.0);
	TEST_TRUE(float(1) == 1.0);
	TestExit();
}
