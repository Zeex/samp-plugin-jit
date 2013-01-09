// Old frame pointer that is stored on the stack should
// be relative to the AMX data section.

#include <a_samp>
#include <test>

main() {
	printf("%x", f());
	TestExit();
}

f() {
	#emit load.s.pri 0
	#emit retn
	return 0;
}
