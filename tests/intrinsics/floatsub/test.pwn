#include <a_samp>
#include <test>

static Float:test_cases[][3] = {
	{0.0, 0.0, 0.0},
	{1.0, 1.0, 0.0},
	{-1.0, -1.0, 0.0}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, floatsub(test_cases[i][0], test_cases[i][1]) == test_cases[i][2]);
	}
	TestExit();
}
