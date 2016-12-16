#include "test"

main() {
	TEST_TRUE(clamp(60, 0, 50) == 50);
	TEST_TRUE(clamp(50, 0, 50) == 50);
	TEST_TRUE(clamp(20, 0, 50) == 20);
	TEST_TRUE(clamp(10, 0, 50) == 10);
	TEST_TRUE(clamp(0, 0, 50) == 0);
	TEST_TRUE(clamp(-10, 0, 50) == 0);
	TEST_TRUE(clamp(100, 0, 50) == 50);
	TEST_TRUE(clamp(1000, 0, 50) == 50);
}

