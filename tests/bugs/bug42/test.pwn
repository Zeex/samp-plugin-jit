#include <a_samp>
#include <test>

stock TestMod(n, m, expected) {
	if (n % m != expected) {
		printf("%d %% %d != %d", n, m, expected);
	}
}

public OnGameModeInit() {
	TestMod(-5, 10, 5);
	TestMod(5, -10, -5);
	TestMod(-5, -10, -5);
	TestMod(5, 10, 5);
}

main() {
	TestExit();
}
