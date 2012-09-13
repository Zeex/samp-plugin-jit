// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

#define ASMJIT_EXPORTS

// [Dependencies - AsmJit]
#include "../core/assembler.h"
#include "../core/compiler.h"
#include "../core/compilerfunc.h"
#include "../core/compileritem.h"
#include "../core/intutil.h"
#include "../core/logger.h"

// [Api-Begin]
#include "../core/apibegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::CompilerFuncDecl - Construction / Destruction]
// ============================================================================

CompilerFuncDecl::CompilerFuncDecl(Compiler* compiler) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemFuncDecl),
  _entryTarget(NULL),
  _exitTarget(NULL),
  _end(NULL),
  _decl(NULL),
  _vars(NULL),
  _funcHints(0),
  _funcFlags(0),
  _funcCallStackSize(0)
{
}

CompilerFuncDecl::~CompilerFuncDecl() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerFuncDecl - Hints]
// ============================================================================

void CompilerFuncDecl::setHint(uint32_t hint, uint32_t value) ASMJIT_NOTHROW
{
  if (hint > 31)
    return;

  if (value)
    _funcHints |= IntUtil::maskFromIndex(hint);
  else
    _funcHints &= ~IntUtil::maskFromIndex(hint);
}

uint32_t CompilerFuncDecl::getHint(uint32_t hint) const ASMJIT_NOTHROW
{
  if (hint > 31)
    return 0;
  
  return (_funcHints & IntUtil::maskFromIndex(hint)) != 0;
}

// ============================================================================
// [AsmJit::CompilerFuncEnd - Construction / Destruction]
// ============================================================================

CompilerFuncEnd::CompilerFuncEnd(Compiler* compiler, CompilerFuncDecl* func) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemFuncEnd),
  _func(func)
{
}

CompilerFuncEnd::~CompilerFuncEnd() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerFuncEnd - Interface]
// ============================================================================

CompilerItem* CompilerFuncEnd::translate(CompilerContext& cc) ASMJIT_NOTHROW
{
  _isTranslated = true;
  return NULL;
}

// ============================================================================
// [AsmJit::CompilerFuncRet - Construction / Destruction]
// ============================================================================

CompilerFuncRet::CompilerFuncRet(Compiler* compiler, CompilerFuncDecl* func, const Operand* first, const Operand* second) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemFuncRet),
  _func(func)
{
  if (first != NULL)
    _ret[0] = *first;

  if (second != NULL)
    _ret[1] = *second;
}

CompilerFuncRet::~CompilerFuncRet() ASMJIT_NOTHROW
{
}

// ============================================================================
// [AsmJit::CompilerFuncRet - Misc]
// ============================================================================

bool CompilerFuncRet::mustEmitJump() const ASMJIT_NOTHROW
{
  // Iterate over next items until we found an item which emits a real instruction.
  CompilerItem* item = this->getNext();

  while (item)
  {
    switch (item->getType())
    {
      // Interesting item.
      case kCompilerItemEmbed:
      case kCompilerItemInst:
      case kCompilerItemFuncCall:
      case kCompilerItemFuncRet:
        return true;

      // Non-interesting item.
      case kCompilerItemComment:
      case kCompilerItemMark:
      case kCompilerItemAlign:
      case kCompilerItemHint:
        break;

      case kCompilerItemTarget:
        if (static_cast<CompilerTarget*>(item)->getLabel().getId() == getFunc()->getExitLabel().getId())
          return false;
        break;

      // Invalid items - these items shouldn't be here. We are inside the 
      // function, after prolog.
      case kCompilerItemFuncDecl:
        break;

      // We can't go forward from here.
      case kCompilerItemFuncEnd:
        return false;
    }

    item = item->getNext();
  }

  return false;
}

// ============================================================================
// [AsmJit::CompilerFuncCall - Construction / Destruction]
// ============================================================================

CompilerFuncCall::CompilerFuncCall(Compiler* compiler, CompilerFuncDecl* caller, const Operand* target) ASMJIT_NOTHROW :
  CompilerItem(compiler, kCompilerItemFuncCall),
  _caller(caller),
  _decl(NULL),
  _args(NULL)
{
  if (target != NULL)
    _target = *target;
}

CompilerFuncCall::~CompilerFuncCall() ASMJIT_NOTHROW
{
}

} // AsmJit namespace

// [Api-Begin]
#include "../core/apibegin.h"
