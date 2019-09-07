// OUTPUT: All tests passed

#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatadd(0.0, 0.0) == 0.0);
	TEST_TRUE(floatadd(1.0, -1.0) == 0.0);
	TEST_TRUE(floatadd(-1.0, 1.0) == 0.0);
	TEST_TRUE(floatadd(POS_INF, 1.0) == POS_INF);
	TEST_TRUE(floatadd(NEG_INF, 1.0) == NEG_INF);
	TEST_TRUE(floatadd(POS_INF, POS_INF) == POS_INF);
	TEST_TRUE(floatadd(NEG_INF, NEG_INF) == NEG_INF);
	TestExit();
}
