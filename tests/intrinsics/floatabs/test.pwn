#include <a_samp>
#include <float_const>
#include <test>

static Float:test_cases[][2] = {
	{0.0, 0.0},
	{1.0, 1.0},
	{-1.0, 1.0},
	{POS_INF, POS_INF},
	{NEG_INF, POS_INF}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, floatabs(test_cases[i][0]) == test_cases[i][1]);
	}
	TestExit();
}
