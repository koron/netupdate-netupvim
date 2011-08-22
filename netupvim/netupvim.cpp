/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 03-Jun-2003.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

/* netupvim.cpp : �A�v���P�[�V�����p�̃G���g�� �|�C���g�̒�` */

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
	    output_info("�_�E�����[�h�Ƀv���L�V %s ���g�p���܂��B", proxy);
	    break;
	case FRESHEN_MSG_START:
	    output_info("�l�b�g���[�N�A�b�v�f�[�g���J�n���܂��B");
	    output_info("");
	    break;
	case FRESHEN_MSG_NOUPDATE:
	    output_info("�X�V���ꂽ�t�@�C���͂���܂���ł����B");
	    break;
	case FRESHEN_MSG_FLESHOUT:
	    if (param1 >= 0)
		output_info("�X�V���������܂����B");
	    else
		output_info("�X�V�͍s�Ȃ��܂���ł���");
	    output_info("�u����v�{�^���������ďI�����Ă��������B");
	    break;
	case FRESHEN_MSG_BADINFO:
	    output_info("�X�V�����擾�o���܂���ł����B");
	    MessageBox(g_hwnd,
		    "Vim�̍X�V���𐳂����擾�o���܂���ł����B\r\n"
		    "�Ǘ��� <koron@tka.att.ne.jp> �Ƀ��[���ŘA������邩�A\r\n"
		    "���艮 http://www.kaoriya.net/ �փA�N�Z�X���Ă��������B"
		    , "Vim�̍X�V���擾�Ɏ��s", MB_OK | MB_ICONERROR);
	    break;
	case FRESHEN_MSG_GOODINFO:
	    output_info("�X�V���̎擾���������܂����B");
	    break;
	case FRESHEN_MSG_SCHEDULE:
	    filenum = param1;
	    totalsize = (int)param2;
	    output_info("%d �t�@�C�� ���v %d �o�C�g���_�E�����[�h���܂��B",
		    filenum, totalsize);
	    output_info("");
	    break;
	case FRESHEN_MSG_CONFIRM:
	    {
		char buf[1024];
		char proxymsg[512];

		proxymsg[0] = '\0';
		if (proxyflag)
		    _snprintf(proxymsg, sizeof(proxymsg), " �v���L�V %s",
			    proxy);
		_snprintf(buf, sizeof(buf),
			"�t�@�C���̃_�E�����[�h\r\n"
			"(%d �t�@�C�� ���v %d �o�C�g%s)\r\n"
			"���J�n���܂���?"
			, filenum, totalsize, proxymsg);
		confirm = MessageBox(g_hwnd, buf, "�_�E�����[�h�m�F",
			MB_ICONQUESTION | MB_YESNO);
	    }
	    return confirm == IDNO ? 0 : 1;
	case FRESHEN_MSG_CANCEL:
	    switch (param1)
	    {
		case FRESHEN_CANCEL_NOFILE:
		    output_info("�X�V���ꂽ�t�@�C���͂���܂���ł����B");
		    break;
		case FRESHEN_CANCEL_CONFIRM:
		case FRESHEN_CANCEL_DOWNLOAD:
		    output_info("�_�E�����[�h�𒆎~���܂����B");
		    break;
		default:
		    output_info(str);
		    break;
	    }
	    break;
	case FRESHEN_MSG_DLBEGIN:
	    output_info("");
	    output_info("URL %s", param2);
	    output_info("%d �o�C�g���_�E�����[�h���Ă��܂��B", param1);
	    break;
	case FRESHEN_MSG_DLEND:
	    output_info("�_�E�����[�h(%d �o�C�g)���������܂����B", param1);
	    break;
	case FRESHEN_MSG_DECOMPRESS:
	    output_info("�_�E�����[�h�����t�@�C�����𓀂��Ă��܂��B");
	    break;
	case FRESHEN_MSG_BADMD5:
	    output_info("MD5�l����v���܂���ł����B"
		    "�_�E�����[�h�Ɏ��s�����悤�ł��B");
	    break;
	case FRESHEN_MSG_GOODMD5:
	    output_info("MD5�l����v���܂����B�_�E�����[�h�ɐ������܂����B");
	    break;
	case FRESHEN_MSG_DLALLEND:
	    output_info("");
	    if (param1 > 0)
	    {
		output_info("%d �̃t�@�C�����_�E�����[�h���I���܂����B",
			param1);
	    }
	    break;
	case FRESHEN_MSG_UPDATEINFO:
	    output_info("�X�V�����`�F�b�N���܂��B");
	    break;
	case FRESHEN_MSG_DLINFO:
	    output_info("URL %s", param2);
	    output_info("����ŐV�̍X�V�����_�E�����[�h���Ă��܂�"
		    "(%d ���)�B", param1);
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
	    SetDlgItemText(hwnd, ID_NOTIFYBUTTON, "����");
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
	    "Vim�̃l�b�g���[�N�A�b�v�f�[�g���J�n���܂���?\r\n"
	    "\r\n"
	    "�A�b�v�f�[�g�ɂ͐���������ȏォ����ꍇ��\r\n"
	    "����܂��B����������Vim���g�p���Ă���ꍇ�A\r\n"
	    "�A�b�v�f�[�g���n�߂�O��Vim���I�����Ă��������B"
	    , "Vim�A�b�v�f�[�g�̊m�F", MB_YESNO | MB_ICONQUESTION);
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
