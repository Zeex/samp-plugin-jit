#include <a_samp>

public call_print(s[]);

main() {
	CallLocalFunction("call_print", "s", "hi there");
	print("OK");
}

public call_print(s[]) {
	print(s);
}
