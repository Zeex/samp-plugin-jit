// OUTPUT: OK
// OUTPUT: OK

#include <a_samp>
#include <jit>
#include "test"

forward OnJITCompile();

main() {
	printf("%s", IsJITPresent() ? ("OK") : ("FAIL"));
	TestExit();
}

public OnJITCompile() {
	printf("%s", IsJITPresent() ? ("FAIL") : ("OK"));
	return 1;
}
