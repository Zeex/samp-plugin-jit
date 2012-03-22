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

// [Dependencies]
#include "Assembler.h"
#include "CodeGenerator.h"
#include "Defs.h"
#include "MemoryManager.h"
#include "MemoryMarker.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::CodeGenerator - Construction / Destruction]
// ============================================================================

CodeGenerator::CodeGenerator()
{
}

CodeGenerator::~CodeGenerator()
{
}

// ============================================================================
// [AsmJit::CodeGenerator - GetGlobal]
// ============================================================================

JitCodeGenerator* CodeGenerator::getGlobal()
{
  static JitCodeGenerator global;
  return &global;
}

// ============================================================================
// [AsmJit::JitCodeGenerator - Construction / Destruction]
// ============================================================================

JitCodeGenerator::JitCodeGenerator() :
  _memoryManager(NULL),
  _memoryMarker(NULL),
  _allocType(MEMORY_ALLOC_FREEABLE)
{
}

JitCodeGenerator::~JitCodeGenerator()
{
}

// ============================================================================
// [AsmJit::JitCodeGenerator - Generate]
// ============================================================================

uint32_t JitCodeGenerator::generate(void** dest, Assembler* assembler)
{
  // Disallow empty code generation.
  sysuint_t codeSize = assembler->getCodeSize();
  if (codeSize == 0)
  {
    *dest = NULL;
    return AsmJit::ERROR_NO_FUNCTION;
  }

  // Switch to global memory manager if not provided.
  MemoryManager* memmgr = getMemoryManager();

  if (memmgr == NULL)
  {
    memmgr = MemoryManager::getGlobal();
  }

  void* p = memmgr->alloc(codeSize, getAllocType());
  if (p == NULL)
  {
    *dest = NULL;
    return ERROR_NO_VIRTUAL_MEMORY;
  }

  // Relocate the code.
  sysuint_t relocatedSize = assembler->relocCode(p);

  // Return unused memory to MemoryManager.
  if (relocatedSize < codeSize)
  {
    memmgr->shrink(p, relocatedSize);
  }

  // Mark memory if MemoryMarker provided.
  if (_memoryMarker)
  {
    _memoryMarker->mark(p, relocatedSize);
  }

  // Return the code.
  *dest = p;
  return ERROR_NONE;
}

} // AsmJit namespace
