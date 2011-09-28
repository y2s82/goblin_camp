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

#undef _UNICODE
#undef UNICODE

#include <windows.h>
#include <dbghelp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <list>
#include <algorithm>

#include "data/Paths.hpp"

namespace Globals {
	extern bool noDumpMode;
}

namespace {
	// Generates crash dump filename.
	void GetDumpFilename(char dumpPath[MAX_PATH], char dumpFilename[MAX_PATH]) {
		char date[20]; // DD-MM-YYYY-HH-MM-SS
		struct tm *timeStruct;
		__int64 timestamp;
		
		_time64(&timestamp);
		timeStruct = _gmtime64(&timestamp);
		
		SHGetFolderPathAndSubDir(
			NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL,
			SHGFP_TYPE_CURRENT, "My Games\\Goblin Camp\\crashes", dumpPath
		);
		
		strftime(date, 20, "%Y-%m-%d_%H-%M-%S", timeStruct);
		_snprintf(dumpFilename, MAX_PATH, "dump_%s.dmp", date);
		_snprintf(dumpPath, MAX_PATH, "%s\\%s", dumpPath, dumpFilename);
		
		char buffer[MAX_PATH + 200];
		_snprintf(buffer, MAX_PATH + 200, "[Goblin Camp] Dump will be written to: %s", dumpFilename);
		OutputDebugString(buffer);
	}
	
	BOOL CALLBACK DumpCallback(void*, MINIDUMP_CALLBACK_INPUT * const input, MINIDUMP_CALLBACK_OUTPUT *output);
	
	// Produces crash.dmp containing exception info and portions of process memory.
	bool CreateDump(EXCEPTION_POINTERS *exception, char dumpPath[MAX_PATH]) {
		HANDLE dump = CreateFile(
			dumpPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
		);
		
		if (dump == INVALID_HANDLE_VALUE) {
			OutputDebugString(TEXT("[Goblin Camp] Could not create crash dump."));
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
			MiniDumpWithDataSegs | MiniDumpWithHandleData |
			MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules | MiniDumpIgnoreInaccessibleMemory |
			MiniDumpWithPrivateReadWriteMemory | MiniDumpWithProcessThreadData
		);
		
		bool result = !!MiniDumpWriteDump(
			GetCurrentProcess(), GetCurrentProcessId(),
			dump, type, ((exception != NULL) ? &dumpExcInfo : NULL), NULL, &dumpCallback
		);
		
		if (!result) {
			char buffer[256];
			_snprintf(buffer, 256, "[Goblin Camp] MiniDumpWriteDump failed: 0x%X", HRESULT_FROM_WIN32(GetLastError()));
			OutputDebugString(buffer);
		}
		
		FlushFileBuffers(dump);
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
				{
					wchar_t filename[_MAX_FNAME];
					_wsplitpath_s(input->Module.FullPath, NULL, 0, NULL, 0, filename, _MAX_FNAME, NULL, 0);
					
					if (wcsicmp(filename, L"goblin-camp") != 0 && wcsicmp(filename, L"ntdll") != 0) {
						output->ModuleWriteFlags = ModuleWriteModule;
					}
				}
				return TRUE;
			default: return FALSE;
		}
	}
	
	LONG ExecuteCrashReporter(char dumpFilename[MAX_PATH]) {
		std::string crashExe = (Paths::Get(Paths::ExecutableDir) / "goblin-camp-crash.exe").string();
		
		PROCESS_INFORMATION procInfo;
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);
		
		std::string cmdLine = "\"" + crashExe + "\" " + dumpFilename;
		char cmdLineBuffer[2048];
		cmdLine.copy(cmdLineBuffer, 2048);
		cmdLineBuffer[cmdLine.size()] = '\0';
		
		// We don't really care whether this succeeds at all. We're very happy if it does, though.
		if (CreateProcess(
			NULL, cmdLineBuffer, NULL, NULL, FALSE,
			DETACHED_PROCESS, NULL, NULL, &startupInfo, &procInfo
		)) {
			CloseHandle(procInfo.hProcess);
			CloseHandle(procInfo.hThread);
		}
		
		// Don't let the system keep the process running, or the crash reporter
		// may not be able to access some files.
		return EXCEPTION_EXECUTE_HANDLER;
	}
	
	// Debug builds:
	//   - dumps are created only when there is no debugger attached, otherwise the exception is simply propagated upwards
	//   - external crash handler is never called (as it's meant for end-users, developers are assumed to know the details of their work environment)
	//
	// Release builds:
	//   - dumps are always created, and exceptions are never propagated upwards (process is always terminated immediately after the exception has been caught)
	//   - external crash handler is called, if it's possible
	//
	// It's also possible to disable the dumping with -nodumps CLI argument in debug builds.
	#ifdef DEBUG
	#	define GC_CREATE_DUMP(E, P) do { if (!IsDebuggerPresent() && !Globals::noDumpMode) CreateDump((E), (P)); } while (0)
	#	define GC_REPORT_CRASH()    EXCEPTION_CONTINUE_SEARCH
	#else
	#	define GC_CREATE_DUMP(E, P) CreateDump((E), (P))
	#	define GC_REPORT_CRASH()    ExecuteCrashReporter(dumpFilename)
	#endif
	
	LONG CALLBACK ExceptionHandler(EXCEPTION_POINTERS *exception) {
		OutputDebugString(TEXT("[Goblin Camp] Unhandled exception occured."));
		
		char dumpPath[MAX_PATH], dumpFilename[MAX_PATH];
		GetDumpFilename(dumpPath, dumpFilename);
		
		GC_CREATE_DUMP(exception, dumpPath);
		return GC_REPORT_CRASH();
	}
}

void GCInstallExceptionHandler() {
	SetUnhandledExceptionFilter(ExceptionHandler);
}
