// OUTPUT: .*OnJITError\(\) called.*

#include <jit>
#include "test"

main() {
	return 0;
}

public OnJITError() {
	#emit jrel 0
	print("OnJITError() called");
}
