#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatdiv(1.0, 0.0) == POS_INF);
	TEST_TRUE(floatdiv(-1.0, 0.0) == NEG_INF);
	TEST_TRUE(floatdiv(1.0, 1.0) == 1.0);
	TEST_TRUE(floatdiv(1.0, -1.0) == -1.0);
	TEST_TRUE(floatdiv(-1.0, 1.0) == -1.0);
	TEST_TRUE(floatdiv(-1.0, -1.0) == 1.0);
	TEST_TRUE(floatdiv(1.0, POS_INF) == 0.0);
	TEST_TRUE(floatdiv(1.0, NEG_INF) == 0.0);
	TEST_TRUE(floatdiv(-1.0, POS_INF) == 0.0);
	TEST_TRUE(floatdiv(-1.0, NEG_INF) == 0.0);
	TestExit();
}
