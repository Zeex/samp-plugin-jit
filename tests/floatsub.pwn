// OUTPUT: All tests passed

#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatsub(0.0, 0.0) == 0.0);
	TEST_TRUE(floatsub(1.0, 1.0) == 0.0);
	TEST_TRUE(floatsub(-1.0, -1.0) == 0.0);
	TEST_TRUE(floatsub(POS_INF, 1.0) == POS_INF);
	TEST_TRUE(floatsub(NEG_INF, 1.0) == NEG_INF);
	TestExit();
}
