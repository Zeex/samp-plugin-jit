#include <a_samp>

public do_halt();

main() {
	CallLocalFunction("do_halt", "");
	print("CallLocalFunction returned");
}

public do_halt() {
	#emit halt 1
}
