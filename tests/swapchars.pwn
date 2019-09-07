// OUTPUT: All tests passed

#include "test"

main() {
	TEST_TRUE(swapchars(0) == 0);
	TEST_TRUE(swapchars(1) == 0x01000000);
	TEST_TRUE(swapchars(2) == 0x02000000);
	TEST_TRUE(swapchars(3) == 0x03000000);
	TEST_TRUE(swapchars(4) == 0x04000000);
	TEST_TRUE(swapchars(0b1010101010) == 0b10101010000000100000000000000000);
	TEST_TRUE(swapchars(0b1111000001111000011) == 0b11000011100000110000011100000000);
	TestExit();
}

