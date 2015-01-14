// OUTPUT: JIT is running: yes

#include <a_samp>
#include <jit>
#include "test"

Test() {
	new bool:jit = IsJITPresent();
	printf("JIT is running: %s", (jit) ? ("yes") : ("no"));
}

main() {
	Test();
	TestExit();
}
