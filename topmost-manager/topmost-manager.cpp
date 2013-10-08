#include <windows.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "resource.h"

#define COUNT_WINDOWS 1
#define LIST_WINDOWS 2
#define ID_TIMER	1

int numWindows = 0;
HWND *visibleWindows;
HIMAGELIST icons = NULL;
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
				ImageList_Destroy(icons);
				icons = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_MASK | ILC_COLOR32, numWindows, 1);
			}
			if (i >= numWindows )
				return FALSE;
			else if (isRealWindow(hwnd)) {
				HICON curIcon = (HICON)SendMessage(hwnd,WM_GETICON,ICON_BIG,0);
				if (!curIcon) {
					curIcon = (HICON)GetClassLong(hwnd,GCL_HICON);
				}
				if (curIcon) {
					ImageList_AddIcon(icons,curIcon);
				} else {
					curIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_YELLOWQUESTION));
					ImageList_AddIcon(icons,curIcon);
				}
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

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case IDC_WINDOWLIST:
			if (((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
				{
				if (IsDlgButtonChecked(hwnd,IDC_LIVEUPDATE) ) {
					SetTimer(hwnd,ID_TIMER,1000,NULL);
				}
				int index = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
				if (index >= 0) {
					EnableWindow(GetDlgItem(hwnd, IDC_FLAGBOX),TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_BRINGTOTOP),TRUE);
					if (topmostStatus[index]) {
						CheckDlgButton(hwnd,IDC_FLAGBOX,BST_CHECKED);
					} else {
						CheckDlgButton(hwnd,IDC_FLAGBOX,BST_UNCHECKED);
					}
				} else {
					EnableWindow(GetDlgItem(hwnd, IDC_FLAGBOX),FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_BRINGTOTOP),FALSE);
				}
			    break;
			 }
			}
			break;
		default:
			break;
		}

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_EXIT:
			EndDialog(hwnd,0);
			break;

		case IDC_REFRESH:
			{
				DWORD       dwExtent;
				HDC         hDCListBox;
				HWND		hWndListBox;
				HFONT       hFontOld, hFontNew;
				TEXTMETRIC tm;
				LVITEM curItem;
				int longestIndex = 0;
				int curIndex = 0;
				int itemCount = -1;
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

				

				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_SETHORIZONTALEXTENT,0,0);
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,WM_HSCROLL,SB_TOP,0);
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_DELETESTRING,0,0);
				//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_RESETCONTENT,0,0);

				SendDlgItemMessage(hwnd,IDC_WINDOWLIST,WM_SETREDRAW,FALSE,0);

				EnumWindows(EnumWindowsProc,COUNT_WINDOWS);
				EnumWindows(EnumWindowsProc,LIST_WINDOWS);

				curIndex = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
				SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_DELETEALLITEMS,0,0);
				SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_SETIMAGELIST,LVSIL_NORMAL,(LPARAM)icons);

				for (i=0; i<numWindows; i++) {
					curItem.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
					curItem.stateMask = 0;
					curItem.iSubItem = 0;
					curItem.iItem = i;
					curItem.iImage = i;
					curItem.state = 0;
					if (strlen(windowNames[i]) == 0)
						curItem.pszText = "[no title]";
						//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_ADDSTRING,0,(LPARAM)"[no title]");
					else
						curItem.pszText = windowNames[i];
						//SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LB_ADDSTRING,0,(LPARAM)windowNames[i]);
					itemCount = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_INSERTITEM,0,(LPARAM)&curItem);
				}
				itemCount = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_GETITEMCOUNT,0,0);


				if (curIndex >= numWindows) {
					curIndex = numWindows - 1;
				}
				if (curIndex >= 0) {
					curItem.stateMask = LVIS_SELECTED;
					curItem.state = LVIS_SELECTED;
					SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_SETITEMSTATE,curIndex,(LPARAM)&curItem);
				}
				SendDlgItemMessage(hwnd,IDC_WINDOWLIST,WM_SETREDRAW,TRUE,0);
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
			i = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
			if (i < 0) break;
			if (IsDlgButtonChecked(hwnd,IDC_FLAGBOX)) {
				SetWindowPos(visibleWindows[i],HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			} else {
				SetWindowPos(visibleWindows[i],HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			}
			break;

		case IDC_BRINGTOTOP:
			i = SendDlgItemMessage(hwnd,IDC_WINDOWLIST,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
			if (i < 0) break;
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
	LPSTR lpCmdLine, 
	int nShowCmd 
	) {

		icons = ImageList_Create(20,20,0,1,1);
		DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,MainDialogProc);

		return 0;
}

