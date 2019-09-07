// OUTPUT: should print this
// OUTPUT: All tests passed

#include "test"

main() {
	new x = 0;
	new s[] = "should print this";
	#emit push.adr s
	#emit push.c 4
	#emit const.alt 0xdeadbeef
	#emit sysreq.c print
	#emit stor.s.alt x
	#emit stack 8
	TEST_TRUE(x == 0xdeadbeef);
	TestExit();
}
