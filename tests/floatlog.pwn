#include "float_extras"
#include "test"

main() {
	TEST_TRUE(floatlog(1.0, 2.0) == 0.0);
	TEST_TRUE(floatlog(2.0, 2.0) == 1.0);
	TEST_TRUE(floatlog(4.0, 2.0) == 2.0);
	TEST_TRUE(floatlog(8.0, 2.0) == 3.0);
	TEST_TRUE(floatlog(POS_INF, 10.0) == POS_INF);
	TestExit();
}
