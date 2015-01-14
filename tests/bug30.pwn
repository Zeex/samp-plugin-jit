// OUTPUT: CallLocalFunction returned

#include <a_samp>
#include "test"

#if debug < 1
	#error Compile this script with run-time checks turned on
#endif

public Test();

public OnGameModeInit() {
	CallLocalFunction("Test", "");
	printf("CallLocalFunction returned");
}

main() {
	TestExit();
}

public Test()
{
	new a[2];
	new b = -1;
	a[b] = 0;
}
