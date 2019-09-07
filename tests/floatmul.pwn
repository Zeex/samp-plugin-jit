// OUTPUT: All tests passed

#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatmul(0.0, 0.0) == 0.0);
	TEST_TRUE(floatmul(1.0, 1.0) == 1.0);
	TEST_TRUE(floatmul(1.0, -1.0) == -1.0);
	TEST_TRUE(floatmul(-1.0, 1.0) == -1.0);
	TEST_TRUE(floatmul(-1.0, -1.0) == 1.0);
	TEST_TRUE(floatmul(POS_INF, POS_INF) == POS_INF);
	TEST_TRUE(floatmul(NEG_INF, NEG_INF) == POS_INF);
	TestExit();
}
