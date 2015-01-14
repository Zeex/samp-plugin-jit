// OUTPUT: CallLocalFunction returned

#include <a_samp>
#include "test"

public do_halt();

main() {
	CallLocalFunction("do_halt", "");
	print("CallLocalFunction returned");

	TestExit();
}

public do_halt() {
	#emit halt 1
}
