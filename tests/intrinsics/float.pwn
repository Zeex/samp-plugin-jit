#include <a_samp>

#include "constants"

enum int_float {
	if_int,
	Float:if_float
}

static Float:test_cases[][int_float] = {
	{0, 0.0},
	{1, 1.0},
	{-1, -1.0},
	{123, 123.0}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, float(test_cases[i][if_int]) == test_cases[i][if_float]);
	}
}
