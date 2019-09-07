// OUTPUT: All tests passed

#include "test"

main() {
	TEST_TRUE(clamp(60, 50, 100) == 60);
	TEST_TRUE(clamp(50, 50, 100) == 50);
	TEST_TRUE(clamp(20, 50, 100) == 50);
	TEST_TRUE(clamp(10, 50, 100) == 50);
	TEST_TRUE(clamp(0, 50, 100) == 50);
	TEST_TRUE(clamp(-10, 50, 100) == 50);
	TEST_TRUE(clamp(100, 50, 100) == 100);
	TEST_TRUE(clamp(1000, 50, 100) == 100);
	TestExit();
}

