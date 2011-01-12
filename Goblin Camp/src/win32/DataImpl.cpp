/* Copyright 2010-2011 Ilkka Halila
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

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <boost/filesystem.hpp>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

namespace fs = boost::filesystem;

void GCCommandLine(std::vector<std::string>&);

namespace PathsImpl {
	void FindPersonalDirectory(fs::path& dir) {
		char myGames[MAX_PATH];
		ZeroMemory(&myGames, sizeof(myGames));
		
		SHGetFolderPathAndSubDirA(
			NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, "My Games", myGames
		);
		
		dir = fs::path(std::string(myGames)) / "Goblin Camp";
	}
	
	void FindExecutableDirectory(fs::path& exec, fs::path& execDir, fs::path& dataDir) {
		std::vector<std::string> args;
		GCCommandLine(args);
		
		exec    = fs::path(std::string(args[0]));
		execDir = exec.parent_path();
		dataDir = execDir;
	}
}
