#include <jit>
#include "test"

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
