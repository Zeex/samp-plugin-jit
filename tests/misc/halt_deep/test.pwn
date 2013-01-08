#include <a_samp>

native jit_exit(code = 0);

public do_halt();

main() {
	CallLocalFunction("do_halt", "");
	print("CallLocalFunction returned");

	jit_exit();
}

public do_halt() {
	#emit halt 1
}
