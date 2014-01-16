// [AsmJit/Contrib]
// Remote Context.
//
// [License]
// Zlib - See COPYING file in this package.// [Guard]#ifndef _ASMJIT_CONTRIB_REMOTECONTEXT_H#define _ASMJIT_CONTRIB_REMOTECONTEXT_H

#include <asmjit/core.h>
#if defined(ASMJIT_WINDOWS)

namespace AsmJit {// ============================================================================// [AsmJit::RemoteContext]// ============================================================================//! @brief RemoteContext can be used to inject code into different process.struct RemoteContext : public Context{  // --------------------------------------------------------------------------  // [Construction / Destruction]  // --------------------------------------------------------------------------   //! @brief Create a @c RemoteCodeGenerator instance for @a hProcess.  RemoteContext(HANDLE hProcess);  //! @brief Destroy the @c RemoteCodeGenerator instance.  virtual ~RemoteContext();  // --------------------------------------------------------------------------  // [Accessors]  // --------------------------------------------------------------------------  //! @brief Get the remote process handle.  inline HANDLE getProcess() const
  { return _hProcess; }

  //! @brief Get the virtual memory manager.  inline VirtualMemoryManager* getMemoryManager()
  { return &_memoryManager; }  // --------------------------------------------------------------------------  // [Interface]  // -------------------------------------------------------------------------- 
  virtual uint32_t generate(void** dest, Assembler* assembler);  // --------------------------------------------------------------------------  // [Members]  // --------------------------------------------------------------------------  //! @brief Process.  HANDLE _hProcess;  //! @brief Virtual memory manager.  VirtualMemoryManager _memoryManager;  ASMJIT_NO_COPY(RemoteContext)};} // AsmJit namespace// [Guard]#endif // ASMJIT_WINDOWS#endif // _ASMJIT_CONTRIB_REMOTECONTEXT_H