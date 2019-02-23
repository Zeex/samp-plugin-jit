// OUTPUT: 123

#include "test"

forward f();

main() {
	new x = 123;
	CallLocalFunction("f", "");
	printf("%d", x);
}

public f() {
}
