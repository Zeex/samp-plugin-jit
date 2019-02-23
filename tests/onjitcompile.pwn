// OUTPUT: OK
// OUTPUT: OK

#include <jit>
#include "test"

forward OnJITCompile();

main() {
	printf("%s", IsJITPresent() ? ("OK") : ("FAIL"));
}

public OnJITCompile() {
	printf("%s", IsJITPresent() ? ("FAIL") : ("OK"));
	return 1;
}
