// OUTPUT: JIT is running: yes

#include <test>

Test() {
	new jit;
	#emit zero.pri
	#emit lctrl 7
	#emit stor.s.pri jit
	printf("JIT is running: %s", (jit)? ("yes"): ("no"));
}

main() {
	Test();
	TestExit();
}
