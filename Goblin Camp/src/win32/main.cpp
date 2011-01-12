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

#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

int GCMain(std::vector<std::string>&);
void InstallExceptionHandler();
void GCCommandLine(std::vector<std::string>&);

#ifndef DEBUG
#	define GC_MAIN_FUNCTION()  WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#	define GC_GET_ARGUMENTS(A) GCCommandLine(A)
#else
#	define GC_MAIN_FUNCTION()  main(int argc, char **argv)
#	define GC_GET_ARGUMENTS(A) std::copy(argv, argv + argc, std::back_inserter(A))
#endif

int GC_MAIN_FUNCTION() {
	InstallExceptionHandler();
	std::vector<std::string> args;
	
	GC_GET_ARGUMENTS(args);
	return GCMain(args);
}
