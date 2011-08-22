/* vim:set ts=8 sw=4 sts=4 tw=0: */
/*
 * Last Change: 09-Nov-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#define DLLPATH "freshen.dll"

#ifdef Freshen
# pragma message("Freshen was defined\n")
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>
#include "freshen.h"

#define Freshen			dll_Freshen
#define FreshenLastErrorString	dll_FreshenLastErrorString
#define FreshenSetMessageProc	dll_FreshenSetMessageProc
#define FreshenSetProgressProc	dll_FreshenSetProgressProc

int (*Freshen)(unsigned int flag, char* infofile, char* default_url);
char* (*FreshenLastErrorString)();
void (*FreshenSetMessageProc)(FRESHEN_MSGPROC proc, void* data);
void (*FreshenSetProgressProc)(FRESHEN_PROGRESS proc, void* data);

    int
message(void* d, unsigned int id, int param1, void* param2, char* str)
{
    _RPT4(_CRT_WARN, "Message ID=%08X P1=%d P2=%p :%s\n",
	    id, param1, param2, str);
    return 1;
}

    int
progress(void* d, FRESHEN_PROGRESS_T* p)
{
    _RPT2(_CRT_WARN, "%s::progress(%p) ", __FILE__, d);
    _RPT4(_CRT_WARN, "%1d/%1d:%7d/%7d\n",
	    p->hour, p->hour_last, p->minute, p->minute_last);
    return 1;
}

    int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    HMODULE hdll = NULL;

    /* ライブラリのロード */
    hdll = LoadLibrary(DLLPATH);
    _RPT2(_CRT_WARN, "LoadLibrary(\"%s\")=%08X\n", DLLPATH, hdll);
    if (!hdll)
	return -1;

    /* 関数ポインタを取得する */
    *(FARPROC*)&Freshen = GetProcAddress(hdll, "Freshen");
    *(FARPROC*)&FreshenLastErrorString = GetProcAddress(hdll,
	    "FreshenLastErrorString");
    *(FARPROC*)&FreshenSetMessageProc = GetProcAddress(hdll,
	    "FreshenSetMessageProc");
    *(FARPROC*)&FreshenSetProgressProc = GetProcAddress(hdll,
	    "FreshenSetProgressProc");
    _RPT2(_CRT_WARN, "GetProcAddress(%s)=%p\n", "Freshen", Freshen);
    _RPT2(_CRT_WARN, "GetProcAddress(%s)=%p\n", "FreshenLastErrorString",
	    FreshenLastErrorString);
    _RPT2(_CRT_WARN, "GetProcAddress(%s)=%p\n", "FreshenSetMessageProc",
	    FreshenSetMessageProc);
    _RPT2(_CRT_WARN, "GetProcAddress(%s)=%p\n", "FreshenSetProgressProc",
	    FreshenSetProgressProc);

    _RPT0(_CRT_WARN, "--------\n");
    if (FreshenSetMessageProc)
	FreshenSetMessageProc(message, 0);
    if (FreshenSetProgressProc)
	FreshenSetProgressProc(progress, 0);
    if (Freshen)
	Freshen(0, "testdir/filelist.xml", NULL);

    FreeLibrary(hdll);
    return 0;
}
