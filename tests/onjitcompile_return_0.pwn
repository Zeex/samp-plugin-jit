// OUTPUT: OK
// OUTPUT: \[jit\] Compilation was disabled
// OUTPUT: OK

#include <a_samp>
#include <jit>
#include "test"

forward OnJITCompile();

main() {
	printf("%s", IsJITPresent() ? ("FAIL") : ("OK"));
	TestExit();
}

public OnJITCompile() {
	printf("%s", IsJITPresent() ? ("FAIL") : ("OK"));
	return 0;
}
