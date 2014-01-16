#!/usr/bin/env python

# =============================================================================
# [generate-defs.py]
#
# The purpose of this script is to fetch all instruction names into single
# string which won't cause huge reallocation by the linker. The script is 
# included for pure optimization purposes (decrease binary size and count of
# relocation).
# =============================================================================

# =============================================================================
# [Configuration]
# =============================================================================

# Files to process.
FILES = [
  { "file": "../src/asmjit/x86/x86defs.cpp", "arch": "x86" }
]

# =============================================================================
# [Imports]
# =============================================================================

import os, re, string

# =============================================================================
# [Helpers]
# =============================================================================

def readFile(fileName):
  handle = open(fileName, "rb")
  data = handle.read()
  handle.close()
  return data

def writeFile(fileName, data):
  handle = open(fileName, "wb")
  handle.truncate()
  handle.write(data)
  handle.close()

# =============================================================================
# [Main]
# =============================================================================

def processFile(fileName, arch):
  data = readFile(fileName);
  ARCH = arch.upper();

  dIn = data
  r = re.compile(arch + r"InstInfo\[\][\s]*=[\s]*{(?P<BLOCK>[^}])*}")
  m = r.search(dIn)

  if not m:
    print "Couldn't match " + arch + "InstInfo[] in " + fileName
    exit(0)

  dIn = dIn[m.start():m.end()]
  dOut = ""

  hInstId = {}

  dInstId = []
  dInstAddr = []

  dInstStr = []
  dInstPos = 0

  r = re.compile(r'INST\((?P<kId>[A-Za-z0-9_]+)\s*,\s*\"(?P<kStr>[A-Za-z0-9_ ]*)\"')
  for m in r.finditer(dIn):
    kId = m.group("kId")
    kStr = m.group("kStr")

    if not kId in hInstId:
      dInstStr.append(kStr)
      dInstId.append(kId)
      hInstId[kId] = dInstPos

      dInstAddr.append(dInstPos)
      dInstPos += len(kStr) + 1

  dOut += "const char " + arch + "InstName[] =\n"
  for i in xrange(len(dInstStr)):
    dOut += "  \"" + dInstStr[i] + "\\0\"\n"
  dOut += "  ;\n"

  dOut += "\n"

  for i in xrange(len(dInstId)):
    dOut += "#define INDEX_" + dInstId[i] + " " + str(dInstAddr[i]) + "\n"

  mb_string = "// ${" + ARCH + "_INST_DATA:BEGIN}\n"
  me_string = "// ${" + ARCH + "_INST_DATA:END}\n"

  mb = data.index(mb_string)
  me = data.index(me_string)

  data = data[:mb + len(mb_string)] + dOut + data[me:]
  writeFile(fileName, data)

for item in FILES:
  processFile(item["file"], item["arch"])
