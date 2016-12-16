#include "test"

main() {
	new got, hs;
	// Calling "heapspace()" puts a value on the stack, so actally decreases the
	// heap space available slightly.  Take account of this.  "TEST_TRUE" also ends
	// up putting some stuff on the stack, so don't do the comparisons in there
	// directly.
	#emit PUSH.C 0
	#emit HEAP 0
	#emit LCTRL 4
	#emit SUB
	#emit STOR.S.pri got
	#emit POP.pri
	hs = heapspace();
	TEST_TRUE(got == hs);
	#emit HEAP 4
	hs = heapspace();
	TEST_TRUE(got - 4 == hs);
	#emit HEAP 4
	hs = heapspace();
	TEST_TRUE(got - 8 == hs);
	#emit HEAP 4
	hs = heapspace();
	TEST_TRUE(got - 12 == hs);
	#emit HEAP 0xFFFFFFF4
}

