// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../core/build.h"
#include "../core/memorymarker.h"

// [Api-Begin]
#include "../core/apibegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::MemoryMarker]
// ============================================================================

MemoryMarker::MemoryMarker() ASMJIT_NOTHROW {}
MemoryMarker::~MemoryMarker() ASMJIT_NOTHROW {}

} // AsmJit namespace

// [Api-End]
#include "../core/apiend.h"
