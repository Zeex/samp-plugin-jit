# =============================================================================
# [AsmJit - CMakeLists.txt]
# =============================================================================

CMake_Minimum_Required(VERSION 2.8.7)

# =============================================================================
# [AsmJit - Configuration]
# =============================================================================

# Whether to build static library (default FALSE).
# Set(ASMJIT_STATIC FALSE)

# Whether to build samples (default FALSE).
# Set(ASMJIT_BUILD_SAMPLES FALSE)

# =============================================================================
# [AsmJit - Build]
# =============================================================================

# Create AsmJit project. In case that you called Project() before the AsmJit
# CMakeLists.txt the project won't be created (just included).
Set(ASMJIT_PROJECT_STR "Project")

If(NOT CMAKE_PROJECT_NAME)
  Project(asmjit C CXX)
Else()
  # Do not create a project if this CMakeLists.txt is included by a different
  # project. This allows easy static library build including debugger support.
  Set(ASMJIT_PROJECT_STR "Included")
EndIf()

If(ASMJIT_STATIC)
  Set(ASMJIT_PROJECT_STR "${ASMJIT_PROJECT_STR}|Static")
Else()
  Set(ASMJIT_PROJECT_STR "${ASMJIT_PROJECT_STR}|Shared")
EndIf()

Message("")
Message("== ====================================================")
Message("== [AsmJit ${ASMJIT_PROJECT_STR}]")
Message("== ====================================================")
Message("")

# =============================================================================
# [AsmJit - Directory]
# =============================================================================

If(NOT ASMJIT_DIR)
  Set(ASMJIT_DIR ${CMAKE_CURRENT_LIST_DIR})
  Message("-- Initializing ASMJIT_DIR=${ASMJIT_DIR}")
Else()
  Message("-- Using Custom ASMJIT_DIR=${ASMJIT_DIR}")
EndIf()

Set(ASMJIT_SRC_DIR "${ASMJIT_DIR}/src")

# =============================================================================
# [AsmJit - Dependencies]
# =============================================================================

Set(ASMJIT_DEPS "")

# pthread library is needed for non-windows OSes.
If(NOT WIN32)
  List(APPEND ASMJIT_DEPS pthread)
EndIf()

# =============================================================================
# [AsmJit - Include]
# =============================================================================

Include_Directories(${ASMJIT_SRC_DIR})

# =============================================================================
# [AsmJit - Macros]
# =============================================================================

Macro(AsmJitAddSrc dst_var base_path)
  Set(dst_src "")
  Set(dst_hdr "")
  Set(dst_test "")
  
  Set(dst_path "${ASMJIT_SRC_DIR}/${base_path}")

  ForEach(file ${ARGN})
    Set(file_p "${dst_path}/${file}")

    If(file MATCHES "\\.c|\\.cc|\\.cpp|\\.cxx|\\.m|\\.mm")
      Set(file_cflags "")

      # Not ready for now...
      If(file MATCHES "_test\\.")
        List(APPEND dst_test ${file_p})
      Else()
        List(APPEND dst_src ${file_p})
      EndIf()

      If(optimize)
        Set_Source_Files_Properties(${file_p} PROPERTIES COMPILE_FLAGS ${file_cflags})
      EndIf()
    EndIf()

    If(file MATCHES "\\.h|\\.hh|\\.hpp|\\.hxx|\\.inc")
      List(APPEND dst_hdr ${file_p})
    EndIf()
  EndForEach()

  List(APPEND "${dst_var}" ${dst_src} ${dst_hdr})
  List(APPEND "${dst_var}_TEST" ${dst_test})
  Source_Group(${base_path} FILES ${dst_src} ${dst_hdr} ${dst_test})
EndMacro()

# =============================================================================
# [AsmJit - Sources]
# =============================================================================

Set(ASMJIT_SRC "")
Set(ASMJIT_SRC_TEST "")

AsmJitAddSrc(ASMJIT_SRC asmjit
  asmjit.h
  config.h
  core.h
  x86.h
)

AsmJitAddSrc(ASMJIT_SRC asmjit/core
  apibegin.h
  apiend.h
  build.h

  assembler.cpp
  assembler.h
  assert.cpp
  assert.h
  buffer.cpp
  buffer.h
  compiler.cpp
  compiler.h
  compilercontext.cpp
  compilercontext.h
  compilerfunc.cpp
  compilerfunc.h
  compileritem.cpp
  compileritem.h
  context.cpp
  context.h
  cpuinfo.cpp
  cpuinfo.h
  defs.cpp
  defs.h
  func.cpp
  func.h
  intutil.h
  lock.h
  logger.cpp
  logger.h
  memorymanager.cpp
  memorymanager.h
  memorymarker.cpp
  memorymarker.h
  operand.cpp
  operand.h
  podvector.h
  stringbuilder.cpp
  stringbuilder.h
  stringutil.cpp
  stringutil.h
  virtualmemory.cpp
  virtualmemory.h
  zonememory.cpp
  zonememory.h
)

AsmJitAddSrc(ASMJIT_SRC asmjit/x86
  x86assembler.cpp
  x86assembler.h
  x86compiler.cpp
  x86compiler.h
  x86compilercontext.cpp
  x86compilercontext.h
  x86compilerfunc.cpp
  x86compilerfunc.h
  x86compileritem.cpp
  x86compileritem.h
  x86cpuinfo.cpp
  x86cpuinfo.h
  x86defs.cpp
  x86defs.h
  x86func.cpp
  x86func.h
  x86operand.cpp
  x86operand.h
  x86util.cpp
  x86util.h
)

# Build-Type.
If(${CMAKE_BUILD_TYPE})
  If(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    Add_Definitions(-DASMJIT_DEBUG)
  Else()
    Add_Definitions(-DASMJIT_NO_DEBUG)
  EndIf()
EndIf()

# =============================================================================
# [Setup - Headers]
# =============================================================================

If(NOT ASMJIT_STATIC)
  ForEach(i ${ASMJIT_SRC})
    Get_Filename_Component(path ${i} PATH)
    Get_Filename_Component(name ${i} NAME)
    String(REGEX REPLACE "^${ASMJIT_SRC_DIR}/" "" targetpath "${path}")
    If(${name} MATCHES ".h$")
      If(NOT "${name}" MATCHES "_p.h$")
        Install(FILES ${i} DESTINATION "include/${targetpath}")
      EndIf()
    EndIf()
  EndForEach()
EndIf()

# =============================================================================
# [Setup - Lib - asmjit]
# =============================================================================

If(NOT ASMJIT_STATIC)
  Add_Library(asmjit SHARED ${ASMJIT_SRC})
  Target_Link_Libraries(asmjit ${ASMJIT_DEPS})
  Install(TARGETS asmjit DESTINATION lib)
Else()
  Add_Library(asmjit STATIC ${ASMJIT_SRC})
  Target_Link_Libraries(asmjit ${ASMJIT_DEPS})
EndIf()

# =============================================================================
# [Setup - App - test]
# =============================================================================

# Build AsmJit test executables?
If(ASMJIT_BUILD_SAMPLES)
  Set(ASMJIT_SRC_SAMPLES
    testcpu
    testdummy
    testmem
    testopcode
    testsizeOf
    testx86
  )

  ForEach(file ${ASMJIT_SRC_SAMPLES})
    Add_Executable(${file} src/app/test/${file}.cpp)
    Target_Link_Libraries(${file} asmjit ${ASMJIT_DEPS})
  EndForEach(file)
EndIf()
