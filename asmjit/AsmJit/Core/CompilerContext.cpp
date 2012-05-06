// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define _ASMJIT_BEING_COMPILED

// [Dependencies - AsmJit]
#include "../Core/CompilerContext.h"

// [Api-Begin]
#include "../Core/ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::CompilerContext - Construction / Destruction]
// ============================================================================

CompilerContext::CompilerContext(Compiler* compiler) ASMJIT_NOTHROW :
  _zoneMemory(8192 - sizeof(ZoneChunk) - 32),
  _compiler(compiler),
  _func(NULL),
  _start(NULL),
  _stop(NULL),
  _extraBlock(NULL),
  _state(NULL),
  _active(NULL),
  _currentOffset(0),
  _isUnreachable(0)
{
}

CompilerContext::~CompilerContext() ASMJIT_NOTHROW
{
}

} // AsmJit namespace

// [Api-End]
#include "../Core/ApiEnd.h"
