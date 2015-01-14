// OUTPUT: 0: 1
// OUTPUT: 1: 1
// OUTPUT: 2: 1
// OUTPUT: 3: 1
// OUTPUT: 4: 1

#include <a_samp>
#include "float_const"
#include "test"

static Float:test_cases[][3] = {
	{0.0, 0.0, 0.0},
	{1.0, -1.0, 0.0},
	{-1.0, 1.0, 0.0},
	{POS_INF, POS_INF, POS_INF},
	{NEG_INF, NEG_INF, NEG_INF}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, floatadd(test_cases[i][0], test_cases[i][1]) == test_cases[i][2]);
	}
	TestExit();
}
