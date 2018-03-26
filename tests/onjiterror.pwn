// OUTPUT: .*OnJITError\(\) called.*

#include <a_samp>
#include <jit>
#include "test"

main() {
	TestExit();
}

public OnJITError() {
	#emit jrel 0
	print("OnJITError() called");
}
