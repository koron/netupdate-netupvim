/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 03-Jun-2003.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

/* netupvim.cpp : アプリケーション用のエントリ ポイントの定義 */

#include "stdafx.h"
#include <stdio.h>
#include "resource.h"
#include "freshen.h"
#pragma comment(lib, "freshen")

#define NETUPVIM_FILELIST "filelist.xml"
#define NETUPVIM_DEFAULT_URL "http://www.kaoriya.net/update/netup-vim64-w32j.xml"

static HWND g_hwnd = 0;
static int g_cancel = 0;

    void
output_info(char* info, ...)
{
    if (info)
    {
	char buf[4096];
	va_list args;
	va_start(args, info);
	_vsnprintf(buf, sizeof(buf), info, args);

	HWND hwndInfo = GetDlgItem(g_hwnd, IDC_INFOMATION);
	SendMessage(hwndInfo, EM_SETSEL, -1, 0);
	SendMessage(hwndInfo, EM_REPLACESEL, FALSE, (LPARAM)buf);
	SendMessage(hwndInfo, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
    }
}

    int
message(void* d, unsigned int id, int param1, void* param2, char* str)
{
    static int filenum = 0, totalsize = 0, proxyflag = 0;
    static char proxy[256];
    int confirm;

    switch (id)
    {
	case FRESHEN_MSG_USEPROXY:
	    proxyflag = 1;
	    strncpy(proxy, (char*)param2, sizeof(proxy));
	    output_info("ダウンロードにプロキシ %s を使用します。", proxy);
	    break;
	case FRESHEN_MSG_START:
	    output_info("ネットワークアップデートを開始します。");
	    output_info("");
	    break;
	case FRESHEN_MSG_NOUPDATE:
	    output_info("更新されたファイルはありませんでした。");
	    break;
	case FRESHEN_MSG_FLESHOUT:
	    if (param1 >= 0)
		output_info("更新を完了しました。");
	    else
		output_info("更新は行なわれませんでした");
	    output_info("「閉じる」ボタンを押して終了してください。");
	    break;
	case FRESHEN_MSG_BADINFO:
	    output_info("更新情報を取得出来ませんでした。");
	    MessageBox(g_hwnd,
		    "Vimの更新情報を正しく取得出来ませんでした。\r\n"
		    "管理者 <koron@tka.att.ne.jp> にメールで連絡を取るか、\r\n"
		    "香り屋 http://www.kaoriya.net/ へアクセスしてください。"
		    , "Vimの更新情報取得に失敗", MB_OK | MB_ICONERROR);
	    break;
	case FRESHEN_MSG_GOODINFO:
	    output_info("更新情報の取得を完了しました。");
	    break;
	case FRESHEN_MSG_SCHEDULE:
	    filenum = param1;
	    totalsize = (int)param2;
	    output_info("%d ファイル 合計 %d バイトをダウンロードします。",
		    filenum, totalsize);
	    output_info("");
	    break;
	case FRESHEN_MSG_CONFIRM:
	    {
		char buf[1024];
		char proxymsg[512];

		proxymsg[0] = '\0';
		if (proxyflag)
		    _snprintf(proxymsg, sizeof(proxymsg), " プロキシ %s",
			    proxy);
		_snprintf(buf, sizeof(buf),
			"ファイルのダウンロード\r\n"
			"(%d ファイル 合計 %d バイト%s)\r\n"
			"を開始しますか?"
			, filenum, totalsize, proxymsg);
		confirm = MessageBox(g_hwnd, buf, "ダウンロード確認",
			MB_ICONQUESTION | MB_YESNO);
	    }
	    return confirm == IDNO ? 0 : 1;
	case FRESHEN_MSG_CANCEL:
	    switch (param1)
	    {
		case FRESHEN_CANCEL_NOFILE:
		    output_info("更新されたファイルはありませんでした。");
		    break;
		case FRESHEN_CANCEL_CONFIRM:
		case FRESHEN_CANCEL_DOWNLOAD:
		    output_info("ダウンロードを中止しました。");
		    break;
		default:
		    output_info(str);
		    break;
	    }
	    break;
	case FRESHEN_MSG_DLBEGIN:
	    output_info("");
	    output_info("URL %s", param2);
	    output_info("%d バイトをダウンロードしています。", param1);
	    break;
	case FRESHEN_MSG_DLEND:
	    output_info("ダウンロード(%d バイト)を完了しました。", param1);
	    break;
	case FRESHEN_MSG_DECOMPRESS:
	    output_info("ダウンロードしたファイルを解凍しています。");
	    break;
	case FRESHEN_MSG_BADMD5:
	    output_info("MD5値が一致しませんでした。"
		    "ダウンロードに失敗したようです。");
	    break;
	case FRESHEN_MSG_GOODMD5:
	    output_info("MD5値が一致しました。ダウンロードに成功しました。");
	    break;
	case FRESHEN_MSG_DLALLEND:
	    output_info("");
	    if (param1 > 0)
	    {
		output_info("%d 個のファイルをダウンロードし終わりました。",
			param1);
	    }
	    break;
	case FRESHEN_MSG_UPDATEINFO:
	    output_info("更新情報をチェックします。");
	    break;
	case FRESHEN_MSG_DLINFO:
	    output_info("URL %s", param2);
	    output_info("から最新の更新情報をダウンロードしています"
		    "(%d 回目)。", param1);
	    break;
	case FRESHEN_MSG_CHDIR:
	    break;
	default:
	    output_info(str);
	    break;
    }
    return 1;
}

    int
progress(void* d, FRESHEN_PROGRESS_T* p)
{
    HWND hwndMinute = GetDlgItem(g_hwnd, IDC_PROGRESS_MINUTE);
    PostMessage(GetDlgItem(g_hwnd, IDC_PROGRESS_HOUR),
	    PBM_SETPOS, (p->hour * 100 / p->hour_last), 0);
    PostMessage(GetDlgItem(g_hwnd, IDC_PROGRESS_MINUTE),
	    PBM_SETPOS, (p->minute * 100 / p->minute_last), 0);
    return g_cancel ? 0 : 1;
}

    void __cdecl
do_update(void* p)
{
    FreshenSetMessageProc(message, 0);
    FreshenSetProgressProc(progress, 0);
    int res = Freshen(0, NETUPVIM_FILELIST, NETUPVIM_DEFAULT_URL);
    SendMessage(g_hwnd, WM_USER + 0, res, 0);
    _endthread();
}

    static int CALLBACK
dlgproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int flag_close = 0;
    switch (uMsg)
    {
	case WM_USER + 0:
	    SetDlgItemText(hwnd, ID_NOTIFYBUTTON, "閉じる");
	    flag_close = 1;
	    return 1;
	case WM_SYSCOMMAND:
	    switch (wParam)
	    {
		case SC_CLOSE:
		    if (!flag_close)
			g_cancel = 1;
		    else
			DestroyWindow(hwnd);
		    return 1;
	    }
	    break;
	case WM_COMMAND:
	    switch (LOWORD(wParam))
	    {
		case ID_NOTIFYBUTTON:
		    if (!flag_close)
			g_cancel = 1;
		    else
			DestroyWindow(hwnd);
		    return 1;
	    }
	    break;
	case WM_DESTROY:
	    PostQuitMessage(0);
	    return 1;
	case WM_INITDIALOG:
	    SetWindowLong(hwnd, DWL_MSGRESULT, 1);
	    return 1;
    }

    return 0;
}

    int APIENTRY
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow )
{
    InitCommonControls();
    HWND hwnd = CreateDialog(hInstance,
		MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, dlgproc);
    if (!hwnd)
	return -1;
    g_hwnd = hwnd;
    SetClassLong(g_hwnd, GCL_HICON, (LONG)LoadIcon(hInstance,
		MAKEINTRESOURCE(IDI_NETUPVIM)));

    MSG msg;
    ShowWindow(hwnd, SW_SHOW);
    int res = MessageBox(hwnd,
	    "Vimのネットワークアップデートを開始しますか?\r\n"
	    "\r\n"
	    "アップデートには数分かそれ以上かかる場合も\r\n"
	    "あります。もしも現在Vimを使用している場合、\r\n"
	    "アップデートを始める前にVimを終了してください。"
	    , "Vimアップデートの確認", MB_YESNO | MB_ICONQUESTION);
    if (res == IDNO)
	DestroyWindow(hwnd);
    else
	_beginthread(do_update, 0, 0);
    while (GetMessage(&msg, NULL, 0, 0))
    {
	if (IsDialogMessage(hwnd, &msg))
	    continue;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    return 0;
}
