// OUTPUT: CallLocalFunction returned

#include "test"

public DoHalt();

main() {
	CallLocalFunction("do_halt", "");
	print("CallLocalFunction returned");
}

public DoHalt() {
	#emit halt 1
}
