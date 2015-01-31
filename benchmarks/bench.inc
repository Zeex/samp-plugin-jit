#if defined BENCH_INC
	#endinput
#endif
#define BENCH_INC

#include <a_samp>

#if __Pawn == 0x030A
	#pragma warning disable 217 // loose indentation
#else
	#pragma tabsize 0
#endif

#define BENCH_BEGIN(%0,%1) \
	{ \
		new name_[] = #%0; \
		new started_ = GetTickCount(); \
		new num_iterations_ = %1; \
		for (new i_ = 0; i_ < num_iterations_; i_++) {

#define BENCH_END() \
		} \
		printf("%s: %d ticks (%d iterations)", \
			name_, GetTickCount() - started_, num_iterations_); \
	}
