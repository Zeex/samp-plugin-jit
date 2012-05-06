// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define _ASMJIT_BEING_COMPILED

// [Dependencies - AsmJit]
#include "../Core/Operand.h"

// [Api-Begin]
#include "../Core/ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::Operand]
// ============================================================================

const Operand noOperand;

// ============================================================================
// [AsmJit::Imm]
// ============================================================================

//! @brief Create signed immediate value operand.
Imm imm(sysint_t i) ASMJIT_NOTHROW
{ 
  return Imm(i, false);
}

//! @brief Create unsigned immediate value operand.
Imm uimm(sysuint_t i) ASMJIT_NOTHROW
{
  return Imm((sysint_t)i, true);
}

} // AsmJit namespace

// [Api-End]
#include "../Core/ApiEnd.h"
