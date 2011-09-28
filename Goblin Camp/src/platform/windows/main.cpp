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

#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <crtdbg.h>
#include <windows.h>

#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

int GCMain(std::vector<std::string>&);
void GCInstallExceptionHandler();
void GCCommandLine(std::vector<std::string>&);

#if !defined(DEBUG) || defined(GC_REDIRECT_STREAMS)
#	define GC_MAIN_FUNCTION()  WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#	define GC_GET_ARGUMENTS(A) GCCommandLine(A)
#else
#	undef main
#	define GC_MAIN_FUNCTION()  main(int argc, char **argv)
#	define GC_GET_ARGUMENTS(A) std::copy(argv, argv + argc, std::back_inserter(A))
#endif

int GC_MAIN_FUNCTION() {
	GCInstallExceptionHandler();
	
	#ifdef GC_REDIRECT_STREAMS
		HANDLE newStdOut, newStdErr;
		newStdOut = CreateFile("./goblin-camp.stdout", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		newStdErr = CreateFile("./goblin-camp.stderr", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		
		if (newStdOut != INVALID_HANDLE_VALUE && newStdErr != INVALID_HANDLE_VALUE) {
			SetStdHandle(STD_OUTPUT_HANDLE, newStdOut);
			SetStdHandle(STD_ERROR_HANDLE,  newStdErr);
			
			*stdout = *_fdopen(_open_osfhandle((intptr_t)newStdOut, _O_TEXT), "w");
			setvbuf(stdout, NULL, _IONBF, 0);
			
			*stderr = *_fdopen(_open_osfhandle((intptr_t)newStdErr, _O_TEXT), "w");
			setvbuf(stderr, NULL, _IONBF, 0);
		}
	#endif
	
	std::vector<std::string> args;
	GC_GET_ARGUMENTS(args);
	
	#ifdef CHK_MEMORY_LEAKS
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
		//_CrtSetBreakAlloc(32921);
	#endif
	
	int ret = GCMain(args);
	
	#ifdef GC_REDIRECT_STREAMS
		CloseHandle(newStdOut);
		CloseHandle(newStdErr);
	#endif
	
	return ret;
}
