// [AsmJit]
// Complete JIT Assembler for C++ Language.
//
// [License]
// Zlib - See COPYING file in this package.

// [Guard]
#ifndef _ASMJIT_CORE_COMPILERFUNC_H
#define _ASMJIT_CORE_COMPILERFUNC_H

// [Dependencies - AsmJit]
#include "../Core/Compiler.h"
#include "../Core/CompilerItem.h"

// [Api-Begin]
#include "../Core/ApiBegin.h"

namespace AsmJit {

// ============================================================================
// [AsmJit::CompilerFuncDecl]
// ============================================================================

//! @brief Compiler function declaration item.
//!
//! Functions are base blocks for generating assembler output. Each generated
//! assembler stream needs standard entry and leave sequences thats compatible
//! with the operating system conventions (ABI).
//!
//! Function class can be used to generate function prolog) and epilog sequences
//! that are compatible with the demanded calling convention and to allocate and
//! manage variables that can be allocated/spilled during compilation time.
//!
//! @note To create a function use @c Compiler::newFunc() method, do not 
//! create any form of compiler function items using new operator.
//!
//! @sa @ref CompilerState, @ref CompilerVar.
struct CompilerFuncDecl : public CompilerItem
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @c CompilerFuncDecl instance.
  //!
  //! @note Always use @c AsmJit::Compiler::newFunc() to create @c Function
  //! instance.
  ASMJIT_API CompilerFuncDecl(Compiler* compiler) ASMJIT_NOTHROW;
  //! @brief Destroy the @c CompilerFuncDecl instance.
  ASMJIT_API virtual ~CompilerFuncDecl() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get function entry label.
  //!
  //! Entry label can be used to call this function from another code that's
  //! being generated.
  inline const Label& getEntryLabel() const ASMJIT_NOTHROW
  { return _entryLabel; }

  //! @brief Get function exit label.
  //!
  //! Use exit label to jump to function epilog.
  inline const Label& getExitLabel() const ASMJIT_NOTHROW
  { return _exitLabel; }

  //! @brief Get function entry target.
  inline CompilerTarget* getEntryTarget() const ASMJIT_NOTHROW
  { return _entryTarget; }

  //! @brief Get function exit target.
  inline CompilerTarget* getExitTarget() const ASMJIT_NOTHROW
  { return _exitTarget; }

  //! @brief Get function end item.
  inline CompilerFuncEnd* getEnd() const ASMJIT_NOTHROW
  { return _end; }

  //! @brief Get function declaration.
  inline FuncDecl* getDecl() const ASMJIT_NOTHROW
  { return _decl; }

  //! @brief Get function arguments as variables.
  inline CompilerVar** getVars() const ASMJIT_NOTHROW
  { return _vars; }

  //! @brief Get function argument at @a index.
  inline CompilerVar* getVar(uint32_t index) const ASMJIT_NOTHROW
  {
    ASMJIT_ASSERT(index < _decl->getArgumentsCount());
    return _vars[index];
  }

  //! @brief Get function hints.
  inline uint32_t getFuncHints() const ASMJIT_NOTHROW
  { return _funcHints; }

  //! @brief Get function flags.
  inline uint32_t getFuncFlags() const ASMJIT_NOTHROW
  { return _funcFlags; }

  //! @brief Get whether the _funcFlags has @a flag
  inline bool hasFuncFlag(uint32_t flag) const ASMJIT_NOTHROW
  { return (_funcFlags & flag) != 0; }

  //! @brief Set function @a flag.
  inline void setFuncFlag(uint32_t flag) ASMJIT_NOTHROW
  { _funcFlags |= flag; }

  //! @brief Clear function @a flag.
  inline void clearFuncFlag(uint32_t flag) ASMJIT_NOTHROW
  { _funcFlags &= ~flag; }
  
  //! @brief Get whether the function is also a caller.
  inline bool isCaller() const ASMJIT_NOTHROW
  { return hasFuncFlag(kFuncFlagIsCaller); }

  //! @brief Get whether the function is finished.
  inline bool isFinished() const ASMJIT_NOTHROW
  { return hasFuncFlag(kFuncFlagIsFinished); }

  //! @brief Get whether the function is naked.
  inline bool isNaked() const ASMJIT_NOTHROW
  { return hasFuncFlag(kFuncFlagIsNaked); }

  //! @brief Get stack size needed to call other functions.
  inline int32_t getFuncCallStackSize() const ASMJIT_NOTHROW
  { return _funcCallStackSize; }

  // --------------------------------------------------------------------------
  // [Hints]
  // --------------------------------------------------------------------------

  //! @brief Set function hint.
  ASMJIT_API virtual void setHint(uint32_t hint, uint32_t value) ASMJIT_NOTHROW;
  //! @brief Get function hint.
  ASMJIT_API virtual uint32_t getHint(uint32_t hint) const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Prototype]
  // --------------------------------------------------------------------------

  virtual void setPrototype(
    uint32_t convention, 
    uint32_t returnType,
    const uint32_t* arguments, 
    uint32_t argumentsCount) ASMJIT_NOTHROW = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Function entry label.
  Label _entryLabel;
  //! @brief Function exit label.
  Label _exitLabel;
  
  //! @brief Function entry target.
  CompilerTarget* _entryTarget;
  //! @brief Function exit target.
  CompilerTarget* _exitTarget;

  //! @brief Function end item.
  CompilerFuncEnd* _end;

  //! @brief Function declaration.
  FuncDecl* _decl;
  //! @brief Function arguments as compiler variables.
  CompilerVar** _vars;

  //! @brief Function hints;
  uint32_t _funcHints;
  //! @brief Function flags.
  uint32_t _funcFlags;

  //! @brief Stack size needed to call other functions.
  int32_t _funcCallStackSize;
};

// ============================================================================
// [AsmJit::CompilerFuncEnd]
// ============================================================================

//! @brief Compiler function end item.
//!
//! This item does nothing; it's only used by @ref Compiler to mark  specific 
//! location in the code. The @c CompilerFuncEnd is similar to @c CompilerMark,
//! except that it overrides @c translate() to return @c NULL.
struct CompilerFuncEnd : public CompilerItem
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref CompilerMark instance.
  ASMJIT_API CompilerFuncEnd(Compiler* compiler, CompilerFuncDecl* func) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref CompilerMark instance.
  ASMJIT_API virtual ~CompilerFuncEnd() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------
  
  //! @brief Get related function.
  inline CompilerFuncDecl* getFunc() const ASMJIT_NOTHROW
  { return _func; }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual CompilerItem* translate(CompilerContext& cc) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Related function.
  CompilerFuncDecl* _func;

  ASMJIT_NO_COPY(CompilerFuncEnd)
};

// ============================================================================
// [AsmJit::CompilerFuncRet]
// ============================================================================

//! @brief Compiler return from function item.
struct CompilerFuncRet : public CompilerItem
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref CompilerFuncRet instance.
  ASMJIT_API CompilerFuncRet(Compiler* compiler, CompilerFuncDecl* func,
    const Operand* first, const Operand* second) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref CompilerFuncRet instance.
  ASMJIT_API virtual ~CompilerFuncRet() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @Brief Get the related function.
  inline CompilerFuncDecl* getFunc() const ASMJIT_NOTHROW
  { return _func; }

  //! @brief Get the first return operand.
  inline Operand& getFirst() ASMJIT_NOTHROW
  { return _ret[0]; }
  
  //! @overload
  inline const Operand& getFirst() const ASMJIT_NOTHROW
  { return _ret[0]; }

  //! @brief Get the second return operand.
  inline Operand& getSecond() ASMJIT_NOTHROW
  { return _ret[1]; }
 
  //! @overload
  inline const Operand& getSecond() const ASMJIT_NOTHROW
  { return _ret[1]; }

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  //! @brief Get whether jump to epilog has to be emitted.
  ASMJIT_API bool mustEmitJump() const ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Related function.
  CompilerFuncDecl* _func;
  //! @brief Return operand(s).
  Operand _ret[2];

  ASMJIT_NO_COPY(CompilerFuncRet)
};

// ============================================================================
// [AsmJit::CompilerFuncCall]
// ============================================================================

//! @brief Compiler function call item.
struct CompilerFuncCall : public CompilerItem
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create a new @ref CompilerFuncCall instance.
  ASMJIT_API CompilerFuncCall(Compiler* compiler, CompilerFuncDecl* caller, const Operand* target) ASMJIT_NOTHROW;
  //! @brief Destroy the @ref CompilerFuncCall instance.
  ASMJIT_API virtual ~CompilerFuncCall() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! @brief Get caller.
  inline CompilerFuncDecl* getCaller() const ASMJIT_NOTHROW
  { return _caller; }

  //! @brief Get function declaration.
  inline FuncDecl* getDecl() const ASMJIT_NOTHROW
  { return _decl; }

  //! @brief Get target operand.
  inline Operand& getTarget() ASMJIT_NOTHROW
  { return _target; }

  //! @overload
  inline const Operand& getTarget() const ASMJIT_NOTHROW 
  { return _target; }

  // --------------------------------------------------------------------------
  // [Prototype]
  // --------------------------------------------------------------------------

  virtual void setPrototype(uint32_t convention, uint32_t returnType, const uint32_t* arguments, uint32_t argumentsCount) ASMJIT_NOTHROW = 0;

  //! @brief Set function prototype.
  inline void setPrototype(uint32_t convention, const FuncPrototype& func) ASMJIT_NOTHROW
  { setPrototype(convention, func.getReturnType(), func.getArguments(), func.getArgumentsCount()); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Caller (the function which does the call).
  CompilerFuncDecl* _caller;
  //! @brief Function declaration.
  FuncDecl* _decl;

  //! @brief Operand (address of function, register, label, ...).
  Operand _target;
  //! @brief Return operands.
  Operand _ret[2];
  //! @brief Arguments operands.
  Operand* _args;

  ASMJIT_NO_COPY(CompilerFuncCall)
};

} // AsmJit namespace

// [Api-End]
#include "../Core/ApiEnd.h"

// [Guard]
#endif // _ASMJIT_CORE_COMPILERFUNC_H
