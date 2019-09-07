// OUTPUT: All tests passed

#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatcmp(-1.0, 1.0) == -1);
	TEST_TRUE(floatcmp(1.0, -1.0) == 1);
	TEST_TRUE(floatcmp(1.0, 1.0) == 0);

	TEST_TRUE(floatcmp(0.0, SNAN) == -1);
	TEST_TRUE(floatcmp(SNAN, 0.0) == 1);

	TEST_TRUE(floatcmp(0.0, QNAN) == -1);
	TEST_TRUE(floatcmp(QNAN, 0.00) == -1);

	TEST_TRUE(floatcmp(0.0, NEG_INF) == 1);
	TEST_TRUE(floatcmp(NEG_INF, 0.0) == -1);

	TEST_TRUE(floatcmp(0.0, POS_INF) == -1);
	TEST_TRUE(floatcmp(POS_INF, 0.0) == 1);

	TestExit();
}
