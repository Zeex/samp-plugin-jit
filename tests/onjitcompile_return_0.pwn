// OUTPUT: OK
// OUTPUT: \[jit\] Compilation was disabled
// OUTPUT: OK

#include <jit>
#include "test"

forward OnJITCompile();

main() {
	printf("%s", IsJITEnabled() ? ("FAIL") : ("OK"));
	TestExit();
}

public OnJITCompile() {
	printf("%s", IsJITPresent() ? ("OK") : ("FAIL"));
	return 0;
}
