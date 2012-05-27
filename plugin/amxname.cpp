// Copyright (c) 2011-2012, Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met: 
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution. 
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <algorithm>
#include <cstring>
#include <ctime>
#include <exception>
#include <iterator>
#include <list>
#include <map>
#ifdef _WIN32
	#include <memory>
#else
	#include <tr1/memory>
#endif
#include <string>
#include <vector>
#include <amx/amx.h>
#include <amx/amxaux.h>
#ifdef _WIN32
	#include <windows.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#if !defined stat
		#define stat _stat
	#endif
#else
	#include <dirent.h>
	#include <fnmatch.h>
	#include <sys/stat.h>
#endif
#include "amxname.h"

static time_t GetMtime(const std::string filename) {
	struct stat attrib;
	stat(filename.c_str(), &attrib);
	return attrib.st_mtime;
}

class AmxFile {
public:
	static void FreeAmx(AMX *amx);

	explicit AmxFile(const std::string &name);

	bool IsLoaded() const {
		return amxPtr_.get() != 0;
	}

	const AMX *GetAmx() const {
		return amxPtr_.get();
	}

	const std::string &GetName() const {
		return name_;
	}

	std::time_t GetLastWriteTime() const {
		return last_write_;
	}

private:
	std::tr1::shared_ptr<AMX> amxPtr_;
	std::string name_;
	std::time_t last_write_;
};

AmxFile::AmxFile(const std::string &name)
	: name_(name)
	, last_write_(GetMtime(name))
	, amxPtr_(new AMX, FreeAmx)
{
	if (aux_LoadProgram(amxPtr_.get(), const_cast<char*>(name.c_str()), 0) != AMX_ERR_NONE) {
		amxPtr_.reset();
	}
}

void AmxFile::FreeAmx(AMX *amx) {
	aux_FreeProgram(amx);
	delete amx;
}

typedef std::map<std::string, AmxFile> StringToAmxFileMap;
static StringToAmxFileMap scripts;

typedef std::map<AMX*, std::string> AmxToStringMap;
static AmxToStringMap cachedNames;

template<typename OutputIterator>
static void GetFilesInDirectory(const std::string &dir,
								const std::string &pattern,
								OutputIterator result) {
#if defined _WIN32
	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile = FindFirstFile((dir + "\\" + pattern).c_str(), &findFileData);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		do {
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				*result++ = dir + "\\" + findFileData.cFileName;
			}
		} while (FindNextFile(hFindFile, &findFileData) != 0);
		FindClose(hFindFile);
	}
#else
	DIR *dp;
	if ((dp = opendir(dir.c_str())) != 0) {
		struct dirent *dirp;
		while ((dirp = readdir(dp)) != 0) {
			if (!fnmatch(pattern.c_str(), dirp->d_name,
							FNM_CASEFOLD | FNM_NOESCAPE | FNM_PERIOD)) {
				*result++ = dir + "/" + dirp->d_name;
			}
		}
		closedir(dp);
	}
#endif
}

std::string GetAmxName(AMX_HEADER *amxhdr) {
	std::string result;

	typedef std::list<std::string> StringList;

	StringList files;
	GetFilesInDirectory("gamemodes", "*.amx", std::back_inserter(files));
	GetFilesInDirectory("filterscripts", "*.amx", std::back_inserter(files));

	for (StringList::const_iterator it = files.begin(); it != files.end(); ++it) {
		const std::string &file = *it;
		StringToAmxFileMap::iterator script_it = scripts.find(file);
		if (script_it == scripts.end() ||
				script_it->second.GetLastWriteTime() < GetMtime(file)) {
			if (script_it != scripts.end()) {
				scripts.erase(script_it);
			}
			AmxFile script(file);
			if (script.IsLoaded()) {
				scripts.insert(std::make_pair(file, script));
			}
		}
	}

	for (StringToAmxFileMap::const_iterator it = scripts.begin(); it != scripts.end(); ++it) {
		void *amxhdr2 = it->second.GetAmx()->base;
		if (std::memcmp(amxhdr, amxhdr2, sizeof(AMX_HEADER)) == 0) {
			result = it->first;
			break;
		}
	}

	return result;
}

std::string GetAmxName(AMX *amx) {
	std::string result;

	AmxToStringMap::iterator it = cachedNames.find(amx);
	if (it != cachedNames.end()) {
		result = it->second;
	} else {
		result = GetAmxName(reinterpret_cast<AMX_HEADER*>(amx->base));
		if (!result.empty()) {
			cachedNames.insert(std::make_pair(amx, result));
		}
	}

	return result;
}
