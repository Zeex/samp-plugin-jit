// AsmJit - Complete JIT Assembler for C++ Language.

// Copyright (c) 2008-2012, Petr Kobalicek <kobalicek.petr@gmail.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// [Guard]
#ifndef _ASMJIT_MEMORYMARKER_H
#define _ASMJIT_MEMORYMARKER_H

// [Dependencies]
#include "Build.h"
#include "Defs.h"

// [Api-Begin]
#include "ApiBegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_MemoryManagement
//! @{

// ============================================================================
// [AsmJit::MemoryMarker]
// ============================================================================

//! @brief Virtual memory marker interface.
struct ASMJIT_API MemoryMarker
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  MemoryMarker() ASMJIT_NOTHROW;
  virtual ~MemoryMarker() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual void mark(const void* ptr, sysuint_t size) ASMJIT_NOTHROW = 0;

private:
  ASMJIT_DISABLE_COPY(MemoryMarker)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

// [Guard]
#endif // _ASMJIT_MEMORYMARKER_H
