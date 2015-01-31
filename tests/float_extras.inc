#include <float>

#if defined FLOAT_EXTRAS
	#endinput
#endif
#define FLOAT_EXTRAS

const Float:PI = 3.141592653589793238462643383;

const Float:POS_INF = Float:0x7F800000;
const Float:NEG_INF = Float:0xFF800000;
const Float:QNAN = Float:0x7F800001;
const Float:SNAN = Float:0x3F800001;

stock bool:FloatEqual(Float:a, Float:b, Float:epsilon = 0.000001) {
	return -epsilon < a - b <= epsilon;
}
