#include <a_samp>

native jit_exit(code = 0);

main() {
	f();
	print("FAIL");
}

f() {
	#emit halt 1
}
