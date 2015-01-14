// OUTPUT: hi there
// OUTPUT: OK

#include <a_samp>
#include "test"

public call_print(s[]);

main() {
	CallLocalFunction("call_print", "s", "hi there");
	print("OK");

	TestExit();
}

public call_print(s[]) {
	print(s);
}
