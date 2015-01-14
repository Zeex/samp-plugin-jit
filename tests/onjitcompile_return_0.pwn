// OUTPUT: OK
// OUTPUT: \[jit\] Compilation was disabled
// OUTPUT: OK

#include <a_samp>
#include "test"

forward f();
forward OnJITCompile();

stock bool:IsJITPresent() {
	#emit zero.pri
	#emit lctrl 7
	#emit retn
	return false; // make compiler happy
}

main() {
	printf("%s", IsJITPresent() ? ("FAIL") : ("OK"));
	TestExit();
}

public OnJITCompile() {
	printf("%s", IsJITPresent() ? ("FAIL") : ("OK"));
	return 0;
}
