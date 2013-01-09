#include <a_samp>
#include <test>

static Float:test_cases[][2] = {
	{0.0, 0.0},
	{1.0, 1.0},
	{4.0, 2.0},
	{9.0, 3.0},
	{16.0, 4.0},
	{25.0, 5.0}
};

main() {
	for (new i = 0; i < sizeof(test_cases); i++) {
		printf("%d: %d", i, floatsqroot(test_cases[i][0]) == test_cases[i][1]);
	}
	TestExit();
}
