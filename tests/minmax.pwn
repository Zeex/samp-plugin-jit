#include "test"

main() {
	TEST_TRUE(min(4, 5) == 4);
	TEST_TRUE(max(4, 5) == 5);
	TEST_TRUE(min(-4, 5) == -4);
	TEST_TRUE(max(-4, 5) == 5);
	TEST_TRUE(min(4, -5) == -5);
	TEST_TRUE(max(4, -5) == 4);
	TEST_TRUE(min(0, 0) == 0);
	TEST_TRUE(max(0, 0) == 0);
	TEST_TRUE(min(cellmin, 0) == cellmin);
	TEST_TRUE(max(cellmin, 0) == 0);
	TEST_TRUE(min(cellmin, cellmax) == cellmin);
	TEST_TRUE(max(cellmin, cellmax) == cellmax);
}

