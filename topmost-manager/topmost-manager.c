#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "resource.h"

#define COUNT_WINDOWS 1
#define LIST_WINDOWS 2
#define ID_TIMER	1

int numWindows = 0;
HWND *visibleWindows;
char **windowNames;
BOOL *topmostStatus;
BOOL list_initialized = FALSE;

BOOL isRealWindow(HWND hwnd) {
	WINDOWINFO wininfo;
	wininfo.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hwnd,&wininfo);

	if (wininfo.dwStyle & WS_VISIBLE) {
		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL isTopmost(HWND hwnd) {
	WINDOWINFO wininfo;
	wininfo.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hwnd,&wininfo);

	if (wininfo.dwExStyle & WS_EX_TOPMOST) {
		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL CALLBACK EnumWindowsProc(
	HWND hwnd,
	LPARAM lParam
	) {
		static int i=0;

		switch (lParam) {

		case COUNT_WINDOWS:
			if (isRealWindow(hwnd)) {
				numWindows++;
			}
			break;

		case LIST_WINDOWS:
			if (!list_initialized) {
				visibleWindows = (HWND *)malloc(numWindows * sizeof(HWND));
				windowNames = (char **)malloc(numWindows * sizeof(char *));
				topmostStatus = (BOOL *)malloc(numWindows * sizeof(BOOL));
				i = 0;
				list_initialized = TRUE;
			}
			if (i >= numWindows )
				return FALSE;
			else if (isRealWindow(hwnd)) {
				visibleWindows[i] = hwnd;
				windowNames[i] = (char *)malloc((GetWindowTextLength(hwnd) + 2) * sizeof(char));
				GetWindowText(hwnd,windowNames[i],GetWindowTextLength(hwnd)+1);
				topmostStatus[i] = isTopmost(hwnd);
				i++;
			}
			break;

		}
		return TRUE;
}

BOOL CALLBACK AboutDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message) {
	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwnd,0);
			break;
		default:
			break;
		}

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK MainDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	int i;
	BOOL freeMem = TRUE;

	switch(Message) {

	case WM_INITDIALOG:
		{
			HICON hIcon, hIconSm;

			hIcon = (HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,32,32,0);
			hIconSm = (HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,16,16,0);
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

			SendMessage(hwnd,WM_COMMAND,IDC_REFRESH,0);
			//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_SETCURSEL,0,0);

			CheckDlgButton(hwnd,IDC_LIVEUPDATE,BST_CHECKED);
			EnableWindow(GetDlgItem(hwnd,IDC_REFRESH),FALSE);
			SetTimer(hwnd,ID_TIMER,1000,NULL);
			break;
		}

	case WM_TIMER:
		SendMessage(hwnd,WM_COMMAND,IDC_REFRESH,0);
		break;

	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_EXIT:
			EndDialog(hwnd,0);
			break;

		/*case IDC_WINDOWLIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				int index = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_GETCURSEL,0,0);
				if (topmostStatus[index]) {
					CheckDlgButton(hwnd,IDC_FLAGBOX,BST_CHECKED);
				} else {
					CheckDlgButton(hwnd,IDC_FLAGBOX,BST_UNCHECKED);
				}
			}
			break;
*/
		case IDC_REFRESH:
			{
				DWORD       dwExtent;
				HDC         hDCListBox;
				HWND		hWndListBox;
				HFONT       hFontOld, hFontNew;
				TEXTMETRIC tm;
				int longestIndex = 0;
				int curIndex = 0;
				//EnableWindow(hwnd,FALSE);

				/* Free memory */
				free(visibleWindows);
				free(topmostStatus);
				for (i=0; i<numWindows; i++) {
					free(windowNames[i]);
				}
				free(windowNames);

				numWindows = 0;
				list_initialized = FALSE;

				//curIndex = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_GETCURSEL,0,0);

				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_SETHORIZONTALEXTENT,0,0);
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,WM_HSCROLL,SB_TOP,0);
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_DELETESTRING,0,0);
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_RESETCONTENT,0,0);

				EnumWindows(EnumWindowsProc,COUNT_WINDOWS);
				EnumWindows(EnumWindowsProc,LIST_WINDOWS);

				for (i=0; i<numWindows; i++) {
					//if (strlen(windowNames[i]) == 0)
						//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_ADDSTRING,0,(LPARAM)"[no title]");
					//else
						//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_ADDSTRING,0,(LPARAM)windowNames[i]);
				}

				/* Scrollbar stuff */

				/*for (i=0; i<numWindows; i++) {
					if (strlen(windowNames[longestIndex]) <  strlen(windowNames[i]))
						longestIndex = i;
				}

				hWndListBox = GetDlgItem(hwnd,IDC_WINDOWLIST);
				hDCListBox = GetDC(hWndListBox);
				hFontNew = (HFONT)SendMessage(hWndListBox, WM_GETFONT, 0, 0);
				hFontOld = (HFONT)SelectObject(hDCListBox, hFontNew);
				GetTextMetrics(hDCListBox, (LPTEXTMETRIC)&tm);
				dwExtent = GetTabbedTextExtent(hDCListBox, windowNames[longestIndex], strlen(windowNames[longestIndex]),0,NULL)
					+ tm.tmAveCharWidth;
				SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_SETHORIZONTALEXTENT,LOWORD(dwExtent),0);
				SelectObject(hDCListBox, hFontOld);
				ReleaseDC(hWndListBox, hDCListBox);*/

				if (curIndex >= numWindows) {
					curIndex = numWindows - 1;
				}
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_SETCURSEL,curIndex,0);
				if (topmostStatus[curIndex]) {
					CheckDlgButton(hwnd,IDC_FLAGBOX,BST_CHECKED);
				} else {
					CheckDlgButton(hwnd,IDC_FLAGBOX,BST_UNCHECKED);
				}

				//EnableWindow(hwnd,TRUE);
				break;
			}

		case IDC_FLAGBOX:
			//i = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_GETCURSEL,0,0);
			i = 0;
			if (IsDlgButtonChecked(hwnd,IDC_FLAGBOX)) {
				SetWindowPos(visibleWindows[i],HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			} else {
				SetWindowPos(visibleWindows[i],HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			}
			break;

		case IDC_BRINGTOTOP:
			//i = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_GETCURSEL,0,0);
			i = 0;
			SwitchToThisWindow(visibleWindows[i],FALSE);
			break;

		case IDC_LIVEUPDATE:
			if (IsDlgButtonChecked(hwnd,IDC_LIVEUPDATE)) {
				EnableWindow(GetDlgItem(hwnd,IDC_REFRESH),FALSE);
				SetTimer(hwnd,ID_TIMER,1000,NULL);

			} else {
				EnableWindow(GetDlgItem(hwnd,IDC_REFRESH),TRUE);
				KillTimer(hwnd,ID_TIMER);
			}
			break;

		case IDC_ABOUT:
			DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_ABOUT),hwnd,AboutDialogProc);
			break;

		default: //TODO
			break;
		}

	default:
		return FALSE;
	}

	return TRUE;
}

int WINAPI WinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	LPWSTR lpCmdLine, 
	int nShowCmd 
	) {


		DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,MainDialogProc);

		return 0;
}

