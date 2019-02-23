// FLAGS: -d0
// OUTPUT: OK
// OUTPUT: OK
// OUTPUT: OK
// OUTPUT: OK

#include "test"

#if debug > 0
	#error This code will not work properly with debug level > 0
#endif

TestSctrl6() {
	#emit lctrl 6
	#emit add.c 56
	#emit sctrl 6

	print("sctrl 6 broken");
	return;

	print("OK");
}

TestSctrl6Fail() {
	#emit const.pri 0xffffffff
	#emit sctrl 6

	print("OK");
}

TestJumpPri() {
	#emit lctrl 6
	#emit add.c 52
	#emit jump.pri

	print("jump.pri broken");
	return;

	print("OK");
}

TestJumpPriFail() {
	#emit const.pri 0xffffffff
	#emit jump.pri

	print("OK");
}

main() {
	TestSctrl6();
	TestSctrl6Fail();
	TestJumpPri();
	TestJumpPriFail();
}
