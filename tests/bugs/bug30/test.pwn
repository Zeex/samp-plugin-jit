#include <a_samp>

native jit_exit(code = 0);

public Test();

public OnGameModeInit() {
	CallLocalFunction("Test", "");
	printf("CallLocalFunction returned");
}

main() {
	jit_exit();
}

public Test()
{
	new a[2];
	new b = -1;
	a[b] = 0;

	print("Compile this script with array bounds checks turned on");
}
