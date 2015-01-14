// OUTPUT: 0: 1
// OUTPUT: 1: 1
// OUTPUT: 2: 1
// OUTPUT: 3: 1

#include <a_samp>
#include "test"

static Float:test_cases[][3] = {
	{1.0, 2.0, 0.0},
	{2.0, 2.0, 1.0},
	{4.0, 2.0, 2.0},
	{8.0, 2.0, 3.0}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, floatlog(test_cases[i][0], test_cases[i][1]) == test_cases[i][2]);
	}
	TestExit();
}
