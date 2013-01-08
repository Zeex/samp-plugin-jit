#include <a_samp>

native jit_exit(code = 0);

public call_print(s[]);

main() {
	CallLocalFunction("call_print", "s", "hi there");
	print("OK");

	jit_exit();
}

public call_print(s[]) {
	print(s);
}
