// OUTPUT: OK

#include <a_samp>
#include "test"

public f();

main() {
	if (CallLocalFunction("f", "") == 133) {
		print("OK");
	}
	TestExit();
}

public f() {
	return g() + 10;
}

g() {
	return 123;
}
