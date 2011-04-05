#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>
#include <fstream>

#define GC_SKIP_RC
#include "_patcher.rch"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE lpBuff,
								  LONG cb, PLONG pcb)
{
	HANDLE hFile = (HANDLE)dwCookie;
	if (ReadFile(hFile, lpBuff, cb, (DWORD *)pcb, NULL)) 
	{
		return 0;
	}
	return -1;
}

BOOL FillRichEditFromFile(HWND hwnd, LPCTSTR pszFile)
{
	BOOL fSuccess = FALSE;
	HANDLE hFile = CreateFile(pszFile, GENERIC_READ, 
		FILE_SHARE_READ, 0, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile != INVALID_HANDLE_VALUE) 
	{
		EDITSTREAM es = { 0 };
		es.pfnCallback = EditStreamCallback;
		es.dwCookie = (DWORD_PTR)hFile;
		if (SendMessage(hwnd, EM_STREAMIN, SF_RTF, (LPARAM)&es) 
			&& es.dwError == 0) 
		{
				fSuccess = TRUE;
		}
		CloseHandle(hFile);
	}
	return fSuccess;
}

HINSTANCE app;
std::string notes;

INT_PTR CALLBACK DlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam) {
	HWND pbar = GetDlgItem(dialog, IDC_PROGRESS);
	HWND edit = GetDlgItem(dialog, IDC_PATCH_NOTES);
	
	CHARRANGE none = { -1, 0 };
	SETTEXTEX ste = { ST_DEFAULT, CP_ACP };
	switch (msg) {
		case WM_INITDIALOG:
			//FillRichEditFromFile(edit, "0141-notes.rtf");
			//SendMessage(edit, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM)(notes.c_str()));
			SendMessage(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(4, 4));
			SendMessage(edit, EM_SETSEL, -1, -1);
			SendMessage(edit, WM_VSCROLL, SB_TOP, 0);
			
			SetWindowLongPtr(pbar, GWL_STYLE, GetWindowLongPtr(pbar, GWL_STYLE) | PBS_MARQUEE);
			SendMessage(pbar, PBM_SETMARQUEE, 1, 0);
		break;
		case WM_COMMAND:
			if (wParam == IDOK || wParam == IDCANCEL) {
				EndDialog(dialog, wParam);
			} else if (wParam == IDC_GCPATH_BUTTON) {
				BROWSEINFO bi;
				PIDLIST_ABSOLUTE pid;
				char fn[MAX_PATH];
				
				ZeroMemory(&bi, sizeof(bi));
				bi.hwndOwner = dialog;
				bi.lpszTitle = "Select Goblin Camp directory to upgrade.";
				bi.ulFlags   = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
				
				pid = SHBrowseForFolder(&bi);
				if (SHGetPathFromIDList(pid, fn)) {
					SetWindowText(GetDlgItem(dialog, IDC_GCPATH_EDIT), fn);
				}
				
				CoTaskMemFree(pid);
				/*IMalloc *m = 0;
				if (SUCCEEDED(SHGetMalloc(&m))) {
					m->Free(pid);
					m->Release();
				}*/
				//MessageBox(dialog, fn, "foo", MB_OK);
			}
		break;
		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->code == NM_CLICK) {
				ShellExecute(NULL, "open", "http://www.goblincamp.com", NULL, NULL, SW_SHOW);
			}
		break;
		default: return FALSE;
	}
	return TRUE;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPTSTR cmd, BOOL show) {
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC  = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_LINK_CLASS;
	
	CoInitialize(NULL);
	
	if (!LoadLibrary("msftedit.dll")) {
		MessageBox(NULL, "msftedit", "msftedit", MB_OK);
		return 0;
	}
	
	app = instance;
	
	//std::ifstream ifs("0141-notes.rtf");
	//notes = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	
	if (DialogBox(instance, MAKEINTRESOURCE(IDD_PATCHER_MAIN), NULL, &DlgProc) == -1) {
		char *msg;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, (LPTSTR)&msg, 128, NULL);
		MessageBox(NULL, msg, "error", MB_OK);
	}
	
	CoUninitialize();
	
	return 0;
}
