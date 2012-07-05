# Reguire minimum version of CMake
CMake_Minimum_Required(VERSION 2.6)

# AsmJit project - Need to use both C and C++
Project(AsmJit C CXX)

# If ASMJIT_DIR is not specified, assume that we are building it from here
If(NOT ASMJIT_DIR)
  Set(ASMJIT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
EndIf()

# AsmJit.
Set(ASMJIT_ROOT_SOURCES
)

Set(ASMJIT_ROOT_HEADERS
  AsmJit/AsmJit.h
  AsmJit/Core.h
  AsmJit/Config.h
  AsmJit/X86.h
)

Source_Group("AsmJit" FILES
  ${ASMJIT_ROOT_SOURCES}
  ${ASMJIT_ROOT_HEADERS}
)

# AsmJit/Core.
Set(ASMJIT_CORE_SOURCES
  AsmJit/Core/Assembler.cpp
  AsmJit/Core/Assert.cpp
  AsmJit/Core/Buffer.cpp
  AsmJit/Core/Compiler.cpp
  AsmJit/Core/CompilerContext.cpp
  AsmJit/Core/CompilerFunc.cpp
  AsmJit/Core/CompilerItem.cpp
  AsmJit/Core/Context.cpp
  AsmJit/Core/CpuInfo.cpp
  AsmJit/Core/Defs.cpp
  AsmJit/Core/Func.cpp
  AsmJit/Core/Logger.cpp
  AsmJit/Core/MemoryManager.cpp
  AsmJit/Core/MemoryMarker.cpp
  AsmJit/Core/Operand.cpp
  AsmJit/Core/PodVector.cpp
  AsmJit/Core/StringBuilder.cpp
  AsmJit/Core/StringUtil.cpp
  AsmJit/Core/VirtualMemory.cpp
  AsmJit/Core/ZoneMemory.cpp
)

Set(ASMJIT_CORE_HEADERS
  AsmJit/Core/ApiBegin.h
  AsmJit/Core/ApiEnd.h
  AsmJit/Core/Assembler.h
  AsmJit/Core/Assert.h
  AsmJit/Core/Build.h
  AsmJit/Core/Buffer.h
  AsmJit/Core/Compiler.h
  AsmJit/Core/CompilerContext.h
  AsmJit/Core/CompilerFunc.h
  AsmJit/Core/CompilerItem.h
  AsmJit/Core/Context.h
  AsmJit/Core/CpuInfo.h
  AsmJit/Core/Defs.h
  AsmJit/Core/Func.h
  AsmJit/Core/IntUtil.h
  AsmJit/Core/Lock.h
  AsmJit/Core/Logger.h
  AsmJit/Core/MemoryManager.h
  AsmJit/Core/MemoryMarker.h
  AsmJit/Core/Operand.h
  AsmJit/Core/PodVector.h
  AsmJit/Core/StringBuilder.h
  AsmJit/Core/StringUtil.h
  AsmJit/Core/VirtualMemory.h
  AsmJit/Core/ZoneMemory.h
)

Source_Group("AsmJit/Core" FILES
  ${ASMJIT_CORE_SOURCES}
  ${ASMJIT_CORE_HEADERS}
)

# AsmJit/X86.
Set(ASMJIT_X86_SOURCES
  AsmJit/X86/X86Assembler.cpp
  AsmJit/X86/X86Compiler.cpp
  AsmJit/X86/X86CompilerContext.cpp
  AsmJit/X86/X86CompilerFunc.cpp
  AsmJit/X86/X86CompilerItem.cpp
  AsmJit/X86/X86CpuInfo.cpp
  AsmJit/X86/X86Defs.cpp
  AsmJit/X86/X86Func.cpp
  AsmJit/X86/X86Operand.cpp
  AsmJit/X86/X86Util.cpp
)

Set(ASMJIT_X86_HEADERS
  AsmJit/X86/X86Assembler.h
  AsmJit/X86/X86Compiler.h
  AsmJit/X86/X86CompilerContext.h
  AsmJit/X86/X86CompilerFunc.h
  AsmJit/X86/X86CompilerItem.h
  AsmJit/X86/X86CpuInfo.h
  AsmJit/X86/X86Defs.h
  AsmJit/X86/X86Func.h
  AsmJit/X86/X86Operand.h
  AsmJit/X86/X86Util.h
)

Source_Group("AsmJit/X86" FILES
  ${ASMJIT_X86_SOURCES}
  ${ASMJIT_X86_HEADERS}
)

# AsmJit/All.
Set(ASMJIT_ALL_SOURCES
  ${ASMJIT_ROOT_SOURCES}
  ${ASMJIT_CORE_SOURCES}
  ${ASMJIT_HOST_SOURCES}
  ${ASMJIT_X86_SOURCES}
)

Set(ASMJIT_ALL_HEADERS
  ${ASMJIT_ROOT_HEADERS}
  ${ASMJIT_CORE_HEADERS}
  ${ASMJIT_HOST_HEADERS}
  ${ASMJIT_X86_HEADERS}
)

# Include AsmJit to be able to use #include <AsmJit/...>
Include_Directories(${ASMJIT_DIR})

# pthread library is needed for non-windows OSes.
If(NOT WIN32)
  Link_Libraries(pthread)
EndIf()

# Build-Type.
If(${CMAKE_BUILD_TYPE})
  If(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    Add_Definitions(-DASMJIT_DEBUG)
  Else()
    Add_Definitions(-DASMJIT_NO_DEBUG)
  EndIf()
EndIf()

If(ASMJIT_BUILD_STATIC_LIBRARY OR AMSJIT_BUILD_SHARED_LIBRARY)
  Set(ASMJIT_BUILD_LIBRARY TRUE)
EndIf()

# Build AsmJit library?
If(ASMJIT_BUILD_LIBRARY)
  If(ASMJIT_BUILD_STATIC_LIBRARY)
    Add_Definitions(-DASMJIT_STATIC)
    Add_Library(AsmJit STATIC ${ASMJIT_ALL_SOURCES} ${ASMJIT_ALL_HEADERS})
  ElseIf(ASMJIT_BUILD_SHARED_LIBRARY)
    Add_Library(AsmJit SHARED ${ASMJIT_ALL_SOURCES} ${ASMJIT_ALL_HEADERS})
  EndIf()

  # Install library and header files?
  If(ASMJIT_INSTALL)
    Install(TARGETS AsmJit DESTINATION lib${LIB_SUFFIX})
    ForEach(i ${ASMJIT_ALL_HEADERS})
      Get_Filename_Component(path ${i} PATH)
      Install(FILES ${i} DESTINATION "include/${path}")
    EndForEach(i)
  EndIf()
EndIf()

# Build AsmJit test executables?
If(ASMJIT_BUILD_TEST)
  Set(ASMJIT_TEST_FILES
    TestCpu
    TestDummy
    TestMem
    TestOpCode
    TestSizeOf
    TestX86
  )

  ForEach(file ${ASMJIT_TEST_FILES})
    Add_Executable(${file} Test/${file}.cpp)
    Target_Link_Libraries(${file} AsmJit)
  EndForEach(file)
EndIf()
