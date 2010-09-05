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

#include <vector>
#include <string>
#include <boost/foreach.hpp>

int GCMain(std::vector<std::string>&);
void InstallExceptionHandler();
void GCCommandLine(std::vector<std::string>&);

#ifdef DEBUG
int main(int argc, char **argv) {
	InstallExceptionHandler();
	
	std::vector<std::string> args(argc);
	for (int i = 0; i < argc; ++i) {
		args[i] = argv[i];
	}
	return GCMain(args);
}
#endif

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	InstallExceptionHandler();
	
	std::vector<std::string> args;
	GCCommandLine(args);
	return GCMain(args);
}
