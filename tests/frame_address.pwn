// Old frame pointer that is stored on the stack must be relative to
// the start of AMX data section.
//
// OUTPUT: 4010

#include <a_samp>
#include "test"

main() {
	printf("%x", f());
	TestExit();
}

f() {
	#emit load.s.pri 0
	#emit retn
	return 0;
}
