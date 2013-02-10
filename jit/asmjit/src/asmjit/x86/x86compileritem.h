// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_X86_X86COMPILERITEM_H
#define _ASMJIT_X86_X86COMPILERITEM_H

// [Dependencies - AsmJit]
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"

// [Api-Begin]
#include "../core/apibegin.h"

namespace AsmJit {

//! @addtogroup AsmJit_X86
//! @{

// ============================================================================
// [AsmJit::X86CompilerAlign]
// ============================================================================

//! @brief Compiler align item.
struct X86CompilerAlign : public CompilerAlign
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref CompilerAlign instance.
  ASMJIT_API X86CompilerAlign(X86Compiler* x86Compiler, uint32_t size = 0) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref CompilerAlign instance.
  ASMJIT_API virtual ~X86CompilerAlign() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get compiler as @ref X86Compiler.
  inline X86Compiler* getCompiler() const ASMJIT_NOTHROW
  { return reinterpret_cast<X86Compiler*>(_compiler); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  ASMJIT_NO_COPY(X86CompilerAlign)
};

// ============================================================================
// [AsmJit::X86CompilerHint]
// ============================================================================

//! @brief @ref X86Compiler variable hint item.
struct X86CompilerHint : public CompilerHint
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref X86CompilerHint instance.
  ASMJIT_API X86CompilerHint(X86Compiler* compiler, X86CompilerVar* var, uint32_t hintId, uint32_t hintValue) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref X86CompilerHint instance.
  ASMJIT_API virtual ~X86CompilerHint() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get variable as @ref X86CompilerVar.
  inline X86CompilerVar* getVar() const ASMJIT_NOTHROW
  { return reinterpret_cast<X86CompilerVar*>(_var); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual CompilerItem* translate(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual int getMaxSize() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ASMJIT_NO_COPY(X86CompilerHint)
};

// ============================================================================
// [AsmJit::X86CompilerTarget]
// ============================================================================

//! @brief X86Compiler target item.
struct X86CompilerTarget : public CompilerTarget
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref X86CompilerTarget instance.
  ASMJIT_API X86CompilerTarget(X86Compiler* x86Compiler, const Label& target) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref X86CompilerTarget instance.
  ASMJIT_API virtual ~X86CompilerTarget() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get compiler as @ref X86Compiler.
  inline X86Compiler* getCompiler() const ASMJIT_NOTHROW
  { return reinterpret_cast<X86Compiler*>(_compiler); }

  //! @brief Get state as @ref X86CompilerState.
  inline X86CompilerState* getState() const ASMJIT_NOTHROW
  { return reinterpret_cast<X86CompilerState*>(_state); }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual CompilerItem* translate(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ASMJIT_NO_COPY(X86CompilerTarget)
};

// ============================================================================
// [AsmJit::X86CompilerInst]
// ============================================================================

//! @brief @ref X86Compiler instruction item.
struct X86CompilerInst : public CompilerInst
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref X86CompilerInst instance.
  ASMJIT_API X86CompilerInst(X86Compiler* x86Compiler, uint32_t code, Operand* opData, uint32_t opCount) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref X86CompilerInst instance.
  ASMJIT_API virtual ~X86CompilerInst() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get compiler as @ref X86Compiler.
  inline X86Compiler* getCompiler() const ASMJIT_NOTHROW
  { return reinterpret_cast<X86Compiler*>(_compiler); }

  //! @brief Get whether the instruction is special.
  inline bool isSpecial() const ASMJIT_NOTHROW
  { return (_instFlags & kX86CompilerInstFlagIsSpecial) != 0; }

  //! @brief Get whether the instruction is FPU.
  inline bool isFpu() const ASMJIT_NOTHROW
  { return (_instFlags & kX86CompilerInstFlagIsFpu) != 0; }

  //! @brief Get whether the instruction is used with GpbLo register.
  inline bool isGpbLoUsed() const ASMJIT_NOTHROW
  { return (_instFlags & kX86CompilerInstFlagIsGpbLoUsed) != 0; }

  //! @brief Get whether the instruction is used with GpbHi register.
  inline bool isGpbHiUsed() const ASMJIT_NOTHROW
  { return (_instFlags & kX86CompilerInstFlagIsGpbHiUsed) != 0; }

  //! @brief Get memory operand.
  inline Mem* getMemOp() ASMJIT_NOTHROW
  { return _memOp; }

  //! @brief Set memory operand.
  inline void setMemOp(Mem* memOp) ASMJIT_NOTHROW
  { _memOp = memOp; }

  //! @brief Get operands array (3 operands total).
  inline VarAllocRecord* getVars() ASMJIT_NOTHROW
  { return _vars; }

  //! @brief Get operands array (3 operands total).
  inline const VarAllocRecord* getVars() const ASMJIT_NOTHROW
  { return _vars; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual CompilerItem* translate(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual int getMaxSize() const ASMJIT_NOTHROW;
  ASMJIT_API virtual bool _tryUnuseVar(CompilerVar* v) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Memory operand or NULL.
  Mem* _memOp;
  //! @brief Variables (extracted from operands).
  VarAllocRecord* _vars;

  ASMJIT_NO_COPY(X86CompilerInst)
};

// ============================================================================
// [AsmJit::X86CompilerJmpInst]
// ============================================================================

//! @brief @ref X86Compiler "jmp" instruction item.
struct X86CompilerJmpInst : public X86CompilerInst
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API X86CompilerJmpInst(X86Compiler* x86Compiler, uint32_t code, Operand* opData, uint32_t opCount) ASMJIT_NOTHROW;
  ASMJIT_API virtual ~X86CompilerJmpInst() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  inline X86CompilerJmpInst* getJumpNext() const ASMJIT_NOTHROW
  { return _jumpNext; }
  
  inline bool isTaken() const ASMJIT_NOTHROW
  { return (_instFlags & kX86CompilerInstFlagIsTaken) != 0; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual void prepare(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual CompilerItem* translate(CompilerContext& cc) ASMJIT_NOTHROW;
  ASMJIT_API virtual void emit(Assembler& a) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [DoJump]
  // --------------------------------------------------------------------------

  ASMJIT_API void doJump(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Jump]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual CompilerTarget* getJumpTarget() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Jump target.
  X86CompilerTarget* _jumpTarget;
  //! @brief Next jump to the same target in a single linked list.
  X86CompilerJmpInst *_jumpNext;
  //! @brief State associated with the jump.
  X86CompilerState* _state;

  ASMJIT_NO_COPY(X86CompilerJmpInst)
};

//! @}

} // AsmJit namespace

// [Api-End]
#include "../core/apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86COMPILERITEM_H
