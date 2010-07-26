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

#undef _UNICODE
#undef UNICODE

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cstdio>

namespace {
	// Generates crash dump filename.
	void GetDumpFilename(char dumpFilename[MAX_PATH]) {
		char date[20]; // DD-MM-YYYY-HH-MM-SS
		struct tm *timeStruct;
		__int64 timestamp;
		
		_time64(&timestamp);
		timeStruct = _gmtime64(&timestamp);
		
		SHGetFolderPathAndSubDir(
			NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL,
			SHGFP_TYPE_CURRENT, "My Games\\Goblin Camp", dumpFilename
		);
		
		strftime(date, 20, "%d-%m-%Y-%H-%M-%S", timeStruct);
		_snprintf(dumpFilename, MAX_PATH, "%s\\crash-%s.dmp", dumpFilename, date);
		
		char buffer[MAX_PATH + 200];
		_snprintf(buffer, MAX_PATH + 200, "[Goblin Camp] Dump will be written to: %s", dumpFilename);
		OutputDebugString(buffer);
	}
	
	BOOL CALLBACK DumpCallback(void*, MINIDUMP_CALLBACK_INPUT * const input, MINIDUMP_CALLBACK_OUTPUT *output);
	
	// Produces crash.dmp containing exception info and portions of process memory.
	bool CreateDump(EXCEPTION_POINTERS *exception) {
		char dumpFilename[MAX_PATH];
		GetDumpFilename(dumpFilename);
		
		HANDLE dump = CreateFile(
			dumpFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
		);
		
		if (dump == INVALID_HANDLE_VALUE) {
			OutputDebugString(TEXT("[Goblin Camp] Could not create crash.dmp."));
			return false;
		}
		
		OutputDebugString(TEXT("[Goblin Camp] Trying to write minidump."));
		
		MINIDUMP_EXCEPTION_INFORMATION dumpExcInfo;
		MINIDUMP_CALLBACK_INFORMATION  dumpCallback;
		
		dumpExcInfo.ThreadId          = GetCurrentThreadId();
		dumpExcInfo.ClientPointers    = FALSE;
		dumpExcInfo.ExceptionPointers = exception;
		
		dumpCallback.CallbackRoutine  = DumpCallback;
		dumpCallback.CallbackParam    = NULL; 
		
		MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(
			MiniDumpWithPrivateReadWriteMemory | MiniDumpWithDataSegs |
			MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo |
			MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules
		);
		
		bool result = MiniDumpWriteDump(
			GetCurrentProcess(), GetCurrentProcessId(),
			dump, type, (exception != NULL) ? &dumpExcInfo : NULL, NULL, &dumpCallback
		) ? TRUE : FALSE;
		
		if (!result) {
			char buffer[256];
			_snprintf(buffer, 256, "[Goblin Camp] MiniDumpWriteDump failed: 0x%X", HRESULT_FROM_WIN32(GetLastError()));
			OutputDebugString(buffer);
		}
		
		CloseHandle(dump);
		return result;
	}
	
	BOOL CALLBACK DumpCallback(void*, MINIDUMP_CALLBACK_INPUT * const input, MINIDUMP_CALLBACK_OUTPUT *output) {
		if (input == NULL || output == NULL) return FALSE;
		
		switch (input->CallbackType) {
			case IncludeThreadCallback:
			case ThreadCallback:
			case ThreadExCallback:
			case MemoryCallback:
			case IncludeModuleCallback: return TRUE;
			case CancelCallback:        return FALSE;
			case ModuleCallback:
				if (output->ModuleWriteFlags & ModuleWriteDataSeg) {
					wchar_t filename[_MAX_FNAME];
					_wsplitpath_s(input->Module.FullPath, NULL, 0, NULL, 0, filename, _MAX_FNAME, NULL, 0);
					if (wcsicmp(filename, L"goblin-camp") != 0 && wcsicmp(filename, L"ntdll") != 0) {
						output->ModuleWriteFlags &= ~ModuleWriteDataSeg;
					}
				}
				return TRUE;
			default: return FALSE;
		}
	}
	
	LONG CALLBACK ExceptionHandler(EXCEPTION_POINTERS *exception) {
		OutputDebugString(TEXT("[Goblin Camp] Unhandled exception occured."));
		CreateDump(exception);
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

void InstallExceptionHandler() {
	SetUnhandledExceptionFilter(ExceptionHandler);
}
