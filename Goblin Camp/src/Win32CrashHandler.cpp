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

// Jamroot needs a way to do platform-specific source files
#ifdef WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <cstring>

#include "Logger.hpp"

namespace {
	// Produces crash.dmp containing exception info and portions of process memory
	LONG CALLBACK ExceptionHandler(__in PEXCEPTION_POINTERS ExceptionInfo) {
		OutputDebugString(TEXT("[Goblin Camp] Unhandled exception occured."));
		
		HANDLE dump = CreateFile(
			TEXT("crash.dmp"), GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
		);
		
		if (dump != INVALID_HANDLE_VALUE) {
			OutputDebugString(TEXT("[Goblin Camp] Trying to write minidump."));
			
			MINIDUMP_EXCEPTION_INFORMATION dumpExcInfo = {0};
			dumpExcInfo.ThreadId          = GetCurrentThreadId();
			dumpExcInfo.ClientPointers    = IsDebuggerPresent();
			dumpExcInfo.ExceptionPointers = ExceptionInfo;
			
			if (!MiniDumpWriteDump(
				GetCurrentProcess(), GetCurrentProcessId(), dump,
				(MINIDUMP_TYPE)(
					MiniDumpWithUnloadedModules | MiniDumpWithHandleData |
					MiniDumpWithProcessThreadData | MiniDumpWithDataSegs |
					MiniDumpWithIndirectlyReferencedMemory
				), &dumpExcInfo, NULL, NULL
			)) {
				OutputDebugString(TEXT("[Goblin Camp] MiniDumpWriteDump failed."));
				char buffer[128];
				sprintf(buffer, "[Goblin Camp] Errcode = 0x%X", HRESULT_FROM_WIN32(GetLastError()));
				OutputDebugStringA(buffer);
			}
			
			CloseHandle(dump);
		} else {
			OutputDebugString(TEXT("[Goblin Camp] Could not create crash.dmp."));
		}
		
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

void InstallExceptionHandler() {
	Logger::Inst()->output << "Installing SEH handler.\n";
	Logger::Inst()->output.flush();
	SetUnhandledExceptionFilter(ExceptionHandler);
}

#endif // WINDOWS
