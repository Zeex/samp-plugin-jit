// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_CORE_MEMORYMARKER_H
#define _ASMJIT_CORE_MEMORYMARKER_H

// [Dependencies - AsmJit]
#include "../core/build.h"
#include "../core/defs.h"

// [Api-Begin]
#include "../core/apibegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_MemoryManagement
//! @{

// ============================================================================
// [AsmJit::MemoryMarker]
// ============================================================================

//! @brief Virtual memory marker interface.
struct MemoryMarker
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API MemoryMarker() ASMJIT_NOTHROW;
  ASMJIT_API virtual ~MemoryMarker() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual void mark(const void* ptr, size_t size) ASMJIT_NOTHROW = 0;

  ASMJIT_NO_COPY(MemoryMarker)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "../core/apiend.h"

// [Guard]
#endif // _ASMJIT_CORE_MEMORYMARKER_H
