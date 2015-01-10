// OUTPUT: 123

#include <a_samp>
#include <test>

forward f();

main() {
	new x = 123;
	CallLocalFunction("f", "");
	printf("%d", x);
	TestExit();
}

public f() {
}
