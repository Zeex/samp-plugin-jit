#include <a_samp>
#include <test>

main() {
	f(0xdead, 0xbeef, 0xf00d);
}

f(...) {
	printf("%d", numargs());
	printf("%x", getarg(0));
	printf("%x", getarg(1));
	printf("%x", getarg(2));

	TestExit();
}
