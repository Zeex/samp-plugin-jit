// OUTPUT: CallLocalFunction returned

#include "test"

#if debug < 1
	#error Compile this script with run-time checks turned on
#endif

public Test();

main() {
	CallLocalFunction("Test", "");
	printf("CallLocalFunction returned");
}

public Test()
{
	new a[2];
	new b = -1;
	a[b] = 0;
}
