// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define _ASMJIT_BEING_COMPILED

// [Dependencies - AsmJit]
#include "../Core/Assert.h"

// [Api-Begin]
#include "../Core/ApiBegin.h"

// helpers
namespace AsmJit {

// ============================================================================
// [AsmJit::Assert]
// ============================================================================

void assertionFailure(const char* file, int line, const char* exp)
{
  fprintf(stderr,
    "*** ASSERTION FAILURE at %s (line %d)\n"
    "*** %s\n", file, line, exp);

  exit(1);
}

} // AsmJit namespace

// [Api-End]
#include "../Core/ApiEnd.h"
