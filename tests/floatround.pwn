#include "float_const"
#include "test"

main() {
	TEST_TRUE(floatround(0.4, floatround_round) == 0.0);
	TEST_TRUE(floatround(-0.4, floatround_round) == 0.0);
	TEST_TRUE(floatround(0.6, floatround_round) == 1.0);
	TEST_TRUE(floatround(-0.6, floatround_round) == -1.0);

	TEST_TRUE(floatround(0.4, floatround_unbiased) == 0.0);
	TEST_TRUE(floatround(-0.4, floatround_unbiased) == 0.0);
	TEST_TRUE(floatround(0.6, floatround_unbiased) == 1.0);
	TEST_TRUE(floatround(-0.6, floatround_unbiased) == -1.0);

	TEST_TRUE(floatround(0.4, floatround_floor) == 0.0);
	TEST_TRUE(floatround(-0.4, floatround_floor) == -1.0);
	TEST_TRUE(floatround(0.6, floatround_floor) == 0.0);
	TEST_TRUE(floatround(-0.6, floatround_floor) == -1.0);

	TEST_TRUE(floatround(0.4, floatround_ceil) == 1.0);
	TEST_TRUE(floatround(-0.4, floatround_ceil) == 0.0);
	TEST_TRUE(floatround(0.6, floatround_ceil) == 1.0);
	TEST_TRUE(floatround(-0.6, floatround_ceil) == 0.0);

	TEST_TRUE(floatround(0.4, floatround_tozero) == 0.0);
	TEST_TRUE(floatround(-0.4, floatround_tozero) == 0.0);
	TEST_TRUE(floatround(0.6, floatround_tozero) == 0.0);
	TEST_TRUE(floatround(-0.6, floatround_tozero) == 0.0);

	TestExit();
}
