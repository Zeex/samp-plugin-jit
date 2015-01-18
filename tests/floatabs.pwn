#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatabs(0.0) == 0.0);
	TEST_TRUE(floatabs(1.0) == 1.0);
	TEST_TRUE(floatabs(-1.0) == 1.0);
	TEST_TRUE(floatabs(POS_INF) == POS_INF);
	TEST_TRUE(floatabs(NEG_INF) == POS_INF);
	TestExit();
}
