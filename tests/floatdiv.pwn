// OUTPUT: 0: 1
// OUTPUT: 1: 1
// OUTPUT: 2: 1
// OUTPUT: 3: 1
// OUTPUT: 4: 1
// OUTPUT: 5: 1
// OUTPUT: 6: 1
// OUTPUT: 7: 1
// OUTPUT: 8: 1
// OUTPUT: 9: 1

#include <a_samp>
#include <float_const>
#include <test>

static Float:test_cases[][3] = {
	{1.0, 0.0, POS_INF},
	{-1.0, 0.0, NEG_INF},
	{1.0, 1.0, 1.0},
	{1.0, -1.0, -1.0},
	{-1.0, 1.0, -1.0},
	{-1.0, -1.0, 1.0},
	{1.0, POS_INF, 0.0},
	{1.0, NEG_INF, 0.0},
	{-1.0, POS_INF, 0.0},
	{-1.0, NEG_INF, 0.0}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, floatdiv(test_cases[i][0], test_cases[i][1]) == test_cases[i][2]);
	}
	TestExit();
}
