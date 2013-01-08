#include <a_samp>

#include "../constants"

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
}
