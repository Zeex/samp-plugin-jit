// Copyright (C) 2011-2012 Sergey Zolotarev
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstring>
#include <ctime>
#include <exception>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
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

static time_t GetMtime(const std::string &filename) {
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
	std::shared_ptr<AMX> amxPtr_;
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

static std::unordered_map<std::string, AmxFile> scripts;
static std::unordered_map<AMX*, std::string> cachedNames;

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

	std::list<std::string> files;
	GetFilesInDirectory("gamemodes", "*.amx", std::back_inserter(files));
	GetFilesInDirectory("filterscripts", "*.amx", std::back_inserter(files));

	std::for_each(files.begin(), files.end(), [](const std::string &file) {
		auto script_it = scripts.find(file);
		if (script_it == scripts.end() ||
				script_it->second.GetLastWriteTime() < GetMtime(file)) {
			if (script_it != scripts.end()) {
				scripts.erase(script_it);
			}
			AmxFile script_it(file);
			if (script_it.IsLoaded()) {
				scripts.insert(std::make_pair(file, script_it));
			}
		}
	});

	for (auto iterator = scripts.begin(); iterator != scripts.end(); ++iterator) {
		void *amxhdr2 = iterator->second.GetAmx()->base;
		if (std::memcmp(amxhdr, amxhdr2, sizeof(AMX_HEADER)) == 0) {
			result = iterator->first;
			break;
		}
	}

	return result;
}

std::string GetAmxName(AMX *amx) {
	std::string result;

	auto it = cachedNames.find(amx);
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
