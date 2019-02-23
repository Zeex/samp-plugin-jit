#include "test"
#include <jit>

public Test();

main()
{
	CallLocalFunction("Test", "");
}

public Test()
{
	TEST_TRUE(IsJITPresent());
	TEST_TRUE(IsJITASMJumpCapable());
}


