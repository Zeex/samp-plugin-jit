#include "bench"

#define ITERATIONS 100000000

main() {
	BENCH_BEGIN(floatabs, ITERATIONS)
		floatabs(-1.0);
	BENCH_END()

	BENCH_BEGIN(floatadd, ITERATIONS)
		floatadd(1.0, 1.0);
	BENCH_END()

	BENCH_BEGIN(floatcmp, ITERATIONS)
		floatcmp(1.0, 1.0);
	BENCH_END()

	BENCH_BEGIN(floatdiv, ITERATIONS)
		floatdiv(1.0, 1.0);
	BENCH_END()

	BENCH_BEGIN(floatlog, ITERATIONS)
		floatlog(1.0, 10.0);
	BENCH_END()

	BENCH_BEGIN(floatmul, ITERATIONS)
		floatmul(1.0, 1.0);
	BENCH_END()

	BENCH_BEGIN(floatsqroot, ITERATIONS)
		floatsqroot(1.0);
	BENCH_END()

	BENCH_BEGIN(floatsub, ITERATIONS)
		floatsub(1.0, 1.0);
	BENCH_END()
}
