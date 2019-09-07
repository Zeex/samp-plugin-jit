// OUTPUT: All tests passed

#include "test"

func1()
{
	TEST_TRUE(numargs() == 0);
	return 101;
}

func2(...)
{
	TEST_TRUE(numargs() == 7);
	return 107;
}

func3(a, b, c)
{
	#pragma unused b, c
	TEST_TRUE(numargs() == 3);
	return a;
}

public Test1();
public Test2();
public Test3();

main()
{
	CallLocalFunction("Test1", "");
	CallLocalFunction("Test2", "");
	CallLocalFunction("Test3", "");
	TestExit();
}

public Test1()
{
	new ret = 0;
	// Push function parameters.
	#emit PUSH.C     0
	// Get the function address.
	#emit CONST.pri  func1
	// Move it to alt.
	#emit MOVE.alt
	// Create the return address.
	#emit LCTRL      6
	#emit ADD.C      32
	#emit LCTRL      8
	#emit PUSH.pri
	#emit MOVE.pri
	#emit SCTRL      6
	#emit STOR.S.pri ret
	TEST_TRUE(ret == 101);
	ret = 0;
	// Push function parameters.
	#emit PUSH.C     0
	// Get the function address.
	#emit CONST.pri  func1
	// Convert it to a real address.
	#emit LCTRL      8
	// Move it to alt.
	#emit MOVE.alt
	// Create the return address.
	#emit LCTRL      6
	#emit ADD.C      32
	#emit LCTRL      8
	#emit PUSH.pri
	#emit MOVE.pri
	#emit SCTRL      8
	#emit STOR.S.pri ret
	TEST_TRUE(ret == 101);
}

public Test2()
{
	new ret = 0;
	// Push function parameters.
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	// Get the function address.
	#emit CONST.pri  func2
	// Move it to alt.
	#emit MOVE.alt
	// Create the return address.
	#emit LCTRL      6
	#emit ADD.C      32
	#emit LCTRL      8
	#emit PUSH.pri
	#emit MOVE.pri
	#emit SCTRL      6
	#emit STOR.S.pri ret
	TEST_TRUE(ret == 107);
	ret = 0;
	// Push function parameters.
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	#emit PUSH.C     28
	// Get the function address.
	#emit CONST.pri  func2
	// Convert it to a real address.
	#emit LCTRL      8
	// Move it to alt.
	#emit MOVE.alt
	// Create the return address.
	#emit LCTRL      6
	#emit ADD.C      32
	#emit LCTRL      8
	#emit PUSH.pri
	#emit MOVE.pri
	#emit SCTRL      8
	#emit STOR.S.pri ret
	TEST_TRUE(ret == 107);
}

public Test3()
{
	new ret = 0;
	// Push function parameters.
	#emit PUSH.C     90
	#emit PUSH.C     80
	#emit PUSH.C     70
	#emit PUSH.C     12
	// Get the function address.
	#emit CONST.pri  func3
	// Move it to alt.
	#emit MOVE.alt
	// Create the return address.
	#emit LCTRL      6
	#emit ADD.C      32
	#emit LCTRL      8
	#emit PUSH.pri
	#emit MOVE.pri
	#emit SCTRL      6
	#emit STOR.S.pri ret
	TEST_TRUE(ret == 70);
	ret = 0;
	// Push function parameters.
	#emit PUSH.C     90
	#emit PUSH.C     80
	#emit PUSH.C     70
	#emit PUSH.C     12
	// Get the function address.
	#emit CONST.pri  func3
	// Convert it to a real address.
	#emit LCTRL      8
	// Move it to alt.
	#emit MOVE.alt
	// Create the return address.
	#emit LCTRL      6
	#emit ADD.C      32
	#emit LCTRL      8
	#emit PUSH.pri
	#emit MOVE.pri
	#emit SCTRL      8
	#emit STOR.S.pri ret
	TEST_TRUE(ret == 70);
}
