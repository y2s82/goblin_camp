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

#include <vector>
#include <string>
#include <cstdlib>

void GCCommandLine(std::vector<std::string>& args) {
	int argc = 0;
	wchar_t **argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
	char buffer[4096];
	args.resize(argc);
	
	for (int i = 0; i < argc; ++i) {
		size_t converted;
		wcstombs_s(&converted, buffer, sizeof(buffer), argvW[i], _TRUNCATE);
		buffer[converted] = '\0';
		
		args[i] = buffer;
	}
}
