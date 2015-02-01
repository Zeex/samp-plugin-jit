#include "test"

main() {
	TEST_TRUE(floatfract(0.0) == 0.0);

	TEST_TRUE(floatfract(0.5) == 0.5);
	TEST_TRUE(floatfract(1.5) == 0.5);

	TEST_TRUE(floatfract(-0.5) == 0.5);
	TEST_TRUE(floatfract(-1.5) == 0.5);

	TestExit();
}
