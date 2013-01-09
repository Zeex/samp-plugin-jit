#include <a_samp>
#include <test>

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

	print("Compile this script with array bounds checks turned on");
}
