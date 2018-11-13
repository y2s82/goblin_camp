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
#include <windows.h>
#include <windowsx.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include <boost/scoped_ptr.hpp>
#include <string>
#include <sstream>

#define GC_SKIP_RC
#include "goblin-camp.rch"

#include "Logger.hpp"

/**
	Crashes the program. Obviously.
	Uses the 0xC0FFEE exception code, raises noncontinuable exception.
*/
void GCDebugInduceCrash() {
	RaiseException(0xC0FFEE, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

namespace {
	struct AssertData {
		const char *msg, *expr, *func, *file;
		int line;
	};
	
	INT_PTR CALLBACK AssertDialog(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_INITDIALOG:
			{
				const AssertData& data = *((AssertData*)lParam);
				std::stringstream msg;
				
				msg << "Expression:\r\n\t" << data.expr << "\r\n\r\n";
				if (data.msg) {
					msg << "Additional info:\r\n\t" << data.msg << "\r\n\r\n";
				}
				msg << "Function:\r\n\t" << data.func << "\r\n\r\n";
				msg << "File:\r\n\t"     << data.file << "\r\n\r\n";
				msg << "Line:\r\n\t"     << data.line << "\r\n\r\n";
				
				std::string msgStr = msg.str();
				
				HWND edit = GetDlgItem(dialog, IDC_REASON);
				Edit_SetText(edit, msgStr.c_str());
				SendMessage(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(4, 4));
				//Edit_SetSel(edit, -1, -1);
			}
			break;
			case WM_COMMAND:
				switch (wParam) {
					case ID_DEBUG:
					case ID_CONTINUE:
					case ID_EXIT:
						EndDialog(dialog, wParam);
					break;
					default:
						return FALSE;
				}
			break;
			default:
				return FALSE;
		}
		
		return TRUE;
	}
}

/**
	Shows 'assertion failed' dialog with a choice to exit, break to debugger or continue.
	
	\param[in] msg      Additional information.
	\param[in] expr     Expression that failed.
	\param[in] function Function that uses assert.
	\param[in] file     File that contains the function.
	\param[in] line     Line where assert was used.
	\returns            True if 'Debug' button was pressed (or the dialog couldn't be created),
	                    false if 'Continue' button was pressed.
*/
bool GCAssert(const char* msg, const char* expr, const char* function, const char* file, int line) {
	LOG("Assertion failed: '" << expr << "' (" << function << " in " << file << " at " << line << "): " << msg);
	
	SDL_SysWMinfo info;
	HWND parent = NULL;
	
	/*SDL_VERSION(&info.version);
	if (SDL_GetWMInfo(&info) > 0 && info.window) {
		parent = info.window;
	}*/
	
	AssertData data = { msg, expr, function, file, line };
	DWORD ret = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ASSERT), parent, &AssertDialog, (LPARAM)&data);
	
	switch (ret) {
		case ID_CONTINUE:
			return false;
		case ID_EXIT:
			exit(424242);
		default:
			LOG("Assertion dialog failed, defaulting to 'break to debugger'.");
			// passthrough
		case ID_DEBUG:
			return true;
	}
}
