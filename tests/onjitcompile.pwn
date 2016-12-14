// OUTPUT: OK
// OUTPUT: OK

#include <a_samp>
#include <jit>
#include "test"

forward OnJITCompile();

main() {
	printf("%s", IsJITEnabled() ? ("OK") : ("FAIL"));
	TestExit();
}

public OnJITCompile() {
	printf("%s", IsJITPresent() ? ("OK") : ("FAIL"));
	return 1;
}
