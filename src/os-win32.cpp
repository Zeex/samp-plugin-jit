// Copyright (c) 2012 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "os.h"

#include <string>
#include <vector>

#include <Windows.h>

const char os::kDirSepChar = '\\';

#undef GetModulePath

std::string os::GetModulePath(void *address, std::size_t maxLength) {
	std::vector<char> name(maxLength + 1);
	if (address != 0) {
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(address, &mbi, sizeof(mbi));
		GetModuleFileName((HMODULE)mbi.AllocationBase, &name[0], maxLength);
	}
	return std::string(&name[0]);
}

void os::ListDirectoryFiles(const std::string &directory, const std::string &pattern,
		bool (*callback)(const char *, void *), void *userData) 
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile = FindFirstFile((directory + kDirSepChar + pattern).c_str(), &findFileData);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		do {
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				callback(findFileData.cFileName, userData);
			}
		} while (FindNextFile(hFindFile, &findFileData) != 0);
		FindClose(hFindFile);
	}
}
