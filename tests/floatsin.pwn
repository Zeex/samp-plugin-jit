#include "float_extras"
#include "test"

main() {
	// Radians
	TEST_TRUE(floatsin(0.0) == 0.0);
	TEST_TRUE(FloatEqual(floatsin(PI / 2), 1.0));
	TEST_TRUE(FloatEqual(floatsin(-PI / 2), -1.0));
	TEST_TRUE(FloatEqual(floatsin(PI), 0.0));
	TEST_TRUE(FloatEqual(floatsin(-PI), 0.0));
	TEST_TRUE(FloatEqual(floatsin(3 * PI / 2), -1.0));
	TEST_TRUE(FloatEqual(floatsin(-3 * PI / 2), 1.0));
	TEST_TRUE(FloatEqual(floatsin(2 * PI), 0.0));
	TEST_TRUE(FloatEqual(floatsin(-2 * PI), 0.0));

	// Degrees
	TEST_TRUE(floatsin(0.0, degrees) == 0.0);
	TEST_TRUE(FloatEqual(floatsin(90.0, degrees), 1.0));
	TEST_TRUE(FloatEqual(floatsin(-90.0, degrees), -1.0));
	TEST_TRUE(FloatEqual(floatsin(180.0, degrees), 0.0));
	TEST_TRUE(FloatEqual(floatsin(-180.0, degrees), 0.0));
	TEST_TRUE(FloatEqual(floatsin(270.0, degrees), -1.0));
	TEST_TRUE(FloatEqual(floatsin(-270.0, degrees), 1.0));
	TEST_TRUE(FloatEqual(floatsin(360.0, degrees), 0.0));
	TEST_TRUE(FloatEqual(floatsin(-360.0, degrees), 0.0));

	// Grades
	TEST_TRUE(floatsin(0.0, grades) == 0.0);
	TEST_TRUE(FloatEqual(floatsin(100.0, grades), 1.0));
	TEST_TRUE(FloatEqual(floatsin(-100.0, grades), -1.0));
	TEST_TRUE(FloatEqual(floatsin(200.0, grades), 0.0));
	TEST_TRUE(FloatEqual(floatsin(-200.0, grades), 0.0));
	TEST_TRUE(FloatEqual(floatsin(300.0, grades), -1.0));
	TEST_TRUE(FloatEqual(floatsin(-300.0, grades), 1.0));
	TEST_TRUE(FloatEqual(floatsin(400.0, grades), 0.0));
	TEST_TRUE(FloatEqual(floatsin(-400.0, grades), 0.0));

	TestExit();
}
