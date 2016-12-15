#include "test"
#include <jit>

public Test();

public OnGameModeInit()
{
	CallLocalFunction("Test", "");
}

main()
{
	TestExit();
}

public Test()
{
	TEST_TRUE(IsJITPresent());
	TEST_TRUE(IsJITASMJumpCapable());
}


