#include "test"

main() {
	TEST_TRUE(clamp(60, 0, 100) == 60);
	TEST_TRUE(clamp(50, 0, 100) == 50);
	TEST_TRUE(clamp(20, 0, 100) == 20);
	TEST_TRUE(clamp(10, 0, 100) == 10);
	TEST_TRUE(clamp(0, 0, 100) == 0);
	TEST_TRUE(clamp(-10, 0, 100) == 0);
	TEST_TRUE(clamp(100, 0, 100) == 100);
	TEST_TRUE(clamp(1000, 0, 100) == 100);
}

