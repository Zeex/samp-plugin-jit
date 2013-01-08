#include <a_samp>

forward Test();

public OnGameModeInit() {
	CallLocalFunction("Test", "");
	printf("CallLocalFunction returned");
}

main() {}

public Test()
{
	new a[2];
	new b = -1;
	a[b] = 0;

	print("Compile this script with array bounds checks turned on");
}
