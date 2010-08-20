/* Copyright 2010 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/
#include "stdafx.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <boost/filesystem.hpp>
#include <string>
#include <cstring>
#include <cstdlib>

namespace fs = boost::filesystem;

void _ImplFindPersonalDirectory(std::string& dir) {
	char myGames[MAX_PATH];
	// gotta love WinAPI
	SHGetFolderPathAndSubDirA(
		NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, "My Games", myGames
	);
	dir = myGames;
	dir += "\\Goblin Camp";
}

void _ImplFindExecutableDirectory(fs::path& exec, fs::path& execDir, fs::path& dataDir) {
	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	
	char exe[MAX_PATH];
	size_t converted;
	wcstombs_s(&converted, exe, MAX_PATH, argv[0], MAX_PATH);
	
	exec    = fs::path(std::string(exe));
	execDir = exec.parent_path();
	dataDir = execDir;
}
