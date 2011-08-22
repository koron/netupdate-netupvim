/* vim:set ts=8 sw=4 sts=4 tw=0: */
/*
 * Last Change: 14-Nov-2003.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

const char USERAGENT[] = ("Freshen/1.4 (netupdate 1.4"
#ifdef _WIN32
	"; Windows"
#endif
	")");
const char FRESHEN_RCFILE[] = "freshen.cfg";
const char CMD_FLESHOUT[] = "fleshout.exe";
const int INFO_TRIALTIME = 6;
const char REGKEY_PROXY_BASE[] = (
	"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
const char REG_HTTPPROXY_KEYWORD[] = "http=";

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "freshen.h"
#include "upinfo.h"
#include "http.h"
#include "etclib.h"
#include "compress.h"
#include "rcfile.h"

#include <winsock2.h>
#pragma comment(lib, "ws2_32")

/* メッセージ通知コールバック */
static FRESHEN_MSGPROC message_proc = NULL;
static void* message_data = NULL;
/* プログレス通知コールバック */
static FRESHEN_PROGRESS progress_proc = NULL;
static void* progress_data = NULL;
static FRESHEN_PROGRESS_T progress;

static FILE *fplog = NULL;	/* ログファイル保存ポインタ */
static http_t *http = NULL;	/* ダウンロード用HTTPオブジェクト */

/*****************************************************************************/
/*
 * メッセージシステム
 */

    static int
message_send(unsigned int msgid, int param1, void* param2, char* fmt, ...)
{
    char buf[4096];
    va_list args;

    va_start(args, fmt);
    _vsnprintf(buf, sizeof(buf), fmt, args);
    if (fplog)
	fprintf(fplog, "%s\n", buf);
    return message_proc
	? message_proc(message_data, msgid, param1, param2, buf) : 1;
}

/*****************************************************************************/
/*
 * プログレスシステム
 */

    static int
progress_notify()
{
    return progress_proc ?  progress_proc(progress_data, &progress) : 1;
}

    static
progress_finish()
{
    progress.hour= 1;
    progress.hour_last = 1;
    progress.minute= 1;
    progress.minute_last = 1;
    progress_notify();
}

    static void
progress_reset()
{
    memset(&progress, 0, sizeof(progress));
    progress.hour_last = 8;
    progress.minute_last = 1;
    progress_notify();
}

    static void
progress_set_minutelast(int last)
{
    progress.minute = 0;
    progress.minute_last = last;
}

    static int
progress_inc_hour()
{
    ++progress.hour;
    progress_set_minutelast(1);
    return progress_notify();
}

    static int
progress_httpgetproc(void* p, int size)
{
    progress.minute = size;
    return progress_notify();
}

/*****************************************************************************/

/*
 * 使用するproxyを返す。戻り値がNULLでなければPROXYを使用する。戻り値は
 * free()する必要がある。NULLならばproxyは使用しない。
 */
    char*
obtain_proxy_setting()
{
    HKEY hkBase;
    DWORD dwType, dwLen;
    DWORD dwEnabled;
    char* proxy;

    _RPT0(_CRT_WARN, "obtain_proxy_setting()\n");

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_PROXY_BASE, 0, KEY_READ,
		&hkBase) != ERROR_SUCCESS)
	return NULL; /* レジストリを参照できません */

    /* PROXYの有無効フラグをチェック */
    dwLen = sizeof(dwEnabled);
    if (RegQueryValueEx(hkBase, "ProxyEnable", 0, &dwType,
		(LPBYTE)&dwEnabled, &dwLen) != ERROR_SUCCESS
	    || dwType != REG_DWORD || dwEnabled == 0)
    {
	_RPT0(_CRT_WARN, "  ProxyEnable: 0\n");
	RegCloseKey(hkBase);
	return NULL; /* レジストリによればPROXYは無効 */
    }

    /* レジストリからIP:PORTの情報の文字列長を取得 */
    RegQueryValueEx(hkBase, "ProxyServer", 0, &dwType, NULL, &dwLen);
    if (dwType != REG_SZ || dwLen < 1)
    {
	RegCloseKey(hkBase);
	return NULL; /* PROXYのIP:ポートが指定されていない */
    }

    proxy = malloc(dwLen);
    if (RegQueryValueEx(hkBase, "ProxyServer", 0, &dwType, (LPBYTE)proxy,
		&dwLen) != ERROR_SUCCESS)
    {
	free(proxy);
	RegCloseKey(hkBase);
	return NULL; /* 何らかの理由でレジストリの値を取得できなかった */
    }

    /* プロトコル別のプロキシ設定を解釈 */
    if (proxy && strchr(proxy, '='))
    {
	char *proxy_http = strstr(proxy, REG_HTTPPROXY_KEYWORD);

	if (proxy_http)
	{
	    char *tail, *new_proxy;

	    /* http用の設定を探し出して保存 */
	    proxy_http += strlen(REG_HTTPPROXY_KEYWORD);
	    for (tail = proxy_http; *tail && *tail != ';'; ++tail)
		;
	    new_proxy = strndup(proxy_http, tail - proxy_http);
	    if (new_proxy)
	    {
		free(proxy);
		proxy = new_proxy;
	    }
	}
	else
	{
	    /* httpのプロキシ設定が存在しない場合 */
	    free(proxy);
	    proxy = NULL;
	}
    }

    _RPT1(_CRT_WARN, "  ProxyServer: %s\n", proxy);
    return proxy;
}

    void
info_set_url(info_t* info, char* url)
{
    if (info)
    {
	if (info->url)
	    free(info->url);
	info->url = url ? strdup(url) : NULL;
    }
}

    static info_t*
update_info(info_t* info, char* path, char* default_url)
{
    /*
     * infoファイルを読込み、そのinfoファイルが最新のものであるかネットワー
     * ク経由で確認し、最新であることを保証する。保証が不可能な場合戻り値は
     * NULLになる。
     */
    info_t *result = NULL, *new_info = NULL;
    char *tmpfile = NULL;
    int time;
    int try_default = 0;

    message_send(FRESHEN_MSG_UPDATEINFO, 0, path,
	    "Update infofile \"%s\"", path);
    _RPT2(_CRT_WARN, "update_info(%p, \"%s\")\n", info, path);
    if (!(tmpfile = _tempnam(".", "freshen")))
	goto ERROR_FINISH;
    _RPT1(_CRT_WARN, "tmp=\"%s\"\n", tmpfile);
    if (!readinfo(info, path))
    {
	if (default_url)
	{
	    info_set_url(info, default_url);
	    try_default = 1;
	}
	else
	    goto ERROR_FINISH;
    }
    progress_inc_hour();

    /*
     * 再帰的にinfoファイルはINFO_TRIALTIME回までネットから読み込んでみる。
     * MD5で内容を検証し、既存のものと差異が無くなるまでトライする。
     */
    for (time = INFO_TRIALTIME - 1; time > 0; --time)
    {
	if (!info->url)
	{
	    _RPT1(_CRT_WARN, "NULL URI was detected (%d)\n", time);
	    time = -1;
	    break;
	}
	_RPT2(_CRT_WARN, "Try to get %s (%d)\n", info->url, time);
	message_send(FRESHEN_MSG_DLINFO, INFO_TRIALTIME - time, info->url,
		"Try to download %s (trial %d)",
		info->url, INFO_TRIALTIME - time);
	if (http_get(http, tmpfile, info->url) < 0)
	{
	    time = -1;
	    break;
	}
	else
	{
	    char md5_tmp[33], md5_cur[33];

	    md5_file(md5_cur, path);
	    md5_file(md5_tmp, tmpfile);
	    _RPT1(_CRT_WARN, "Current   MD5:%s\n", md5_cur);
	    _RPT1(_CRT_WARN, "Temporary MD5:%s\n", md5_tmp);
	    if (!strcmp(md5_cur, md5_tmp))
		break;
	    if (!readinfo(info, tmpfile))
	    {
		if (try_default || !default_url)
		{
		    _RPT0(_CRT_WARN, "Something wrong with new info file\n");
		    time = -1;
		    break;
		}
		else
		{
		    info_set_url(info, default_url);
		    try_default = 1;
		}
	    }
	    /* 正しく読み込めるファイルならば情報ファイルを更新する */
	    CopyFile(tmpfile, path, FALSE);
	}
    }
    progress_inc_hour();
    if (time <= 0)
	goto ERROR_FINISH;

    result = info;
ERROR_FINISH:
    if (tmpfile)
    {
	DeleteFile(tmpfile);
	free(tmpfile);
    }
    if (new_info)
	info_close(new_info);
    return result;
}

    static char*
assure_directory(char* path)
{
    char *p, *r;

    if (p = strrchr(path, '/'))
    {
	*p = '\0';
	if (!mkdir_p(path))
	    r = NULL;
	*p = '/';
    }
    else
	r = path;
    return r;
}

#define FLAG_UPDATE (1 << 0)

    static int
freshen_main(int flag, char* infofile, char* default_url)
{
    int result = 0;
    info_t* info;
    _RPT2(_CRT_WARN, "----\nfreshen_main(\"%s\", %08X)\n", infofile, flag);

    if (!(info = info_open()))
	return -1;

    fplog = fopen("freshen.log", "wt");
    if (!update_info(info, infofile, default_url))
    {
	_RPT0(_CRT_WARN, "Can't update infofile\n");
	message_send(FRESHEN_MSG_BADINFO, 0, 0, "Can't update infofile");
	result = -1;
    }
    else
    {
	int total_size;
	int loaded_size;
	int error_file;
	int load_filenum;
	int no_download;
	updatefile_t *file;
	_RPT1(_CRT_WARN, "readinfo(\"%s\") success\n", infofile);
	message_send(FRESHEN_MSG_GOODINFO, 0, 0,
		"Infofile was updated successfully");
#ifdef _DEBUG
	info_dump(info, fplog);
#endif

	progress_inc_hour();

	/* ファイルの更新チェック */
	total_size = 0;
	load_filenum = 0;
	for (file = info->filelist; file; file = file->prev)
	{
	    char md5_cur[33];
	    _RPT1(_CRT_WARN, "Update check for \"%s\"\n", file->path);

	    file->flag = 0;
	    md5_file(md5_cur, file->path);
	    if (strcmp(md5_cur, file->verify_data))
	    {
		++load_filenum;
		file->flag |= FLAG_UPDATE;
		if (file->size > 0)
		    total_size += file->size;
	    }

	    _RPT1(_CRT_WARN, "  Current MD5: %s\n", md5_cur);
	    _RPT1(_CRT_WARN, "  Recent  MD5: %s\n", file->verify_data);
	    _RPT1(_CRT_WARN, "  Size: %d\n", file->size);
	}
	_RPT1(_CRT_WARN, "Total download size is %d bytes\n", total_size);
	progress.hour_last += load_filenum;
	progress_inc_hour();
	message_send(FRESHEN_MSG_SCHEDULE, load_filenum, (void*)total_size,
		"Schedule to download %d files in %d bytes",
		load_filenum, total_size);
	no_download = 0;
	if (load_filenum == 0)
	{
	    message_send(FRESHEN_MSG_CANCEL, FRESHEN_CANCEL_NOFILE, 0,
		    "No file to update");
	    no_download = 1;
	}
	else if (!message_send(FRESHEN_MSG_CONFIRM, 0, 0,
		    "Continue to download"))
	{
	    message_send(FRESHEN_MSG_CANCEL, FRESHEN_CANCEL_CONFIRM, 0,
		    "Canceled by confirm");
	    no_download = 1;
	}

	/* 必要なファイルのダウンロード */
	loaded_size = 0;
	error_file = 0;
	for (file = info->filelist; file && !no_download; file = file->prev)
	{
	    char *dl_url;
	    char *dl_name;
	    int get_size;

	    if (!(file->flag & FLAG_UPDATE))
		continue;
	    _RPT1(_CRT_WARN, "Download for \"%s\"\n", file->path);
	    ++result;

	    /* 保存すべきディレクトリの確保 */
	    if (!assure_directory(file->path))
	    {
		++error_file;
		result = -1;
		break;
	    }

	    /* ダウンロードするURLとファイル名を生成する */
	    switch (file->compress_type)
	    {
		case UPDATEFILE_COMPRESSTYPE_BZ2:
		    dl_url = strdupv(info->baseurl, file->path, ",bz2", 0);
		    dl_name = strdupv(file->path, ",bz2", 0);
		    break;
		default:
		    dl_url = strdupv(info->baseurl, file->path, 0);
		    dl_name = strdupv(file->path, ",", 0);
		    break;
	    }
	    _RPT1(_CRT_WARN, "  URL: %s\n", dl_url);
	    _RPT1(_CRT_WARN, "  Local name: %s\n", dl_name);

	    /* 実際のダウンロードを行なう */
	    progress_set_minutelast(file->size);
	    message_send(FRESHEN_MSG_DLBEGIN, file->size, dl_url,
		    "Download %s (%d bytes)", dl_url, file->size);
	    http_set_proc_getprogress(http, progress_httpgetproc, NULL);
	    get_size = http_get(http, dl_name, dl_url);
	    http_set_proc_getprogress(http, NULL, NULL);
	    progress_inc_hour();
	    if (get_size >= 0)
	    {
		message_send(FRESHEN_MSG_DLEND, get_size, dl_url,
			"Finish to download %s (%d bytes)", dl_url, file->size);
		loaded_size += get_size;
	    }
	    else
	    {
		message_send(FRESHEN_MSG_CANCEL, FRESHEN_CANCEL_DOWNLOAD, 0,
			"Canceled while downloading %s", dl_url);
		DeleteFile(dl_name);
		++error_file;
		break;
	    }

	    /* 必要ならばファイルを解凍してしまう */
	    switch (file->compress_type)
	    {
		case UPDATEFILE_COMPRESSTYPE_BZ2:
		    message_send(FRESHEN_MSG_DECOMPRESS, 0, dl_name,
			    "Decompress (bz2) %s", dl_name);
		    decompress_bz2(dl_name);
		    DeleteFile(dl_name);
		    free(dl_name);
		    dl_name = strdupv(file->path, ",", 0);
		    break;
	    }

	    /* ロードしたファイルのMD5をチェックする */
	    {
		char md5_dl[33];
		FRESHEN_MD5CHECK_T md5check;
		unsigned int msgid;

		md5_file(md5_dl, dl_name);
		md5check.name = file->path;
		md5check.md5 = md5_dl;
		md5check.md5_req = file->verify_data;

		if (strcmp(file->verify_data, md5_dl))
		{
		    ++error_file;
		    msgid = FRESHEN_MSG_BADMD5;
		}
		else
		    msgid = FRESHEN_MSG_GOODMD5;
		_RPT1(_CRT_WARN, "  Require  MD5: %s\n", file->verify_data);
		_RPT1(_CRT_WARN, "  Download MD5: %s\n", md5_dl);
		message_send(msgid, 0, &md5check,
			"Result of check MD5 for \"%s\"", md5check.name);
	    }

	    free(dl_url);
	    free(dl_name);
	}

	if (error_file > 0 || no_download)
	    result = -1;
    }
    progress_inc_hour();
    message_send(FRESHEN_MSG_DLALLEND, result, 0,
	    "Download session is all over");

    info_close(info);
    if (fplog)
    {
	fclose(fplog);
	fplog = NULL;
    }
    return result;
}

/*****************************************************************************/

    void
FreshenSetProgressProc(FRESHEN_PROGRESS proc, void* data)
{
    progress_proc = proc;
    progress_data = data;
}

    void
FreshenSetMessageProc(FRESHEN_MSGPROC proc, void* data)
{
    message_proc = proc;
    message_data = data;
}

    char*
FreshenLastErrorString()
{
    return "Not implemented yet";
}

    int
Freshen(unsigned int flag, char* infopath, char* default_url)
{
    int result = 0;
    WSADATA wsa;
    _RPT2(_CRT_WARN, "Freshen(info=\"%s\", flag=%08X)\n", infopath, flag);

    if (infopath && WSAStartup(MAKEWORD(1, 1), &wsa) == 0)
    {
	DWORD dwLen;
	char *curdir;
	char *infofile = infopath;

	progress_reset();
	message_send(FRESHEN_MSG_START, 0, infopath,
		"Freshen Start with \"%s\"", infopath);

	/* カレントディレクトリを保存 */
	curdir = (char*)malloc(dwLen = GetCurrentDirectory(0, NULL));
	if (curdir)
	{
	    char *newdir = strdup(infopath);
	    int len = strlen(newdir);

	    GetCurrentDirectory(dwLen, curdir);
	    _RPT1(_CRT_WARN, "GetCurrentDirectory()=\"%s\"\n", curdir);

	    /* infopathのあるディレクトリにカレントディレクトリを変更する */
	    if (newdir)
	    {
		while (len > 0 && newdir[len] != '\\' && newdir[len] != '/')
		    --len;
		if (len > 0)
		{
		    newdir[len] = '\0';
		    infofile = &infopath[len + 1];
		    SetCurrentDirectory(newdir);
		    _RPT1(_CRT_WARN, "Enter directory: %s\n", newdir);
		    message_send(FRESHEN_MSG_CHDIR, 0, newdir,
			    "Enter directory \"%s\"", newdir);
		}
		free(newdir);
	    }
	}
	progress_inc_hour();
	_RPT1(_CRT_WARN, "infofile=\"%s\"\n", infofile);

	/* Freshenの肝 */
	if (http = http_open())
	{
	    char *proxy;
	    rcfile_t *rc;
	    const char *proxyhost = NULL, *proxyport = NULL;
	    const char *proxyuser = NULL, *proxypass = NULL;

	    /* 設定ファイル読み込み */
	    if (rc = rcfile_open(FRESHEN_RCFILE))
	    {
		proxyhost = rcfile_get(rc, "proxyhost");
		proxyport = rcfile_get(rc, "proxyport");
		proxyuser = rcfile_get(rc, "proxyuser");
		proxypass = rcfile_get(rc, "proxypass");
	    }

	    /* プロクシの基本(サーバ・IP)を設定 */
	    if (proxyhost)
		proxy = strdupv(proxyhost, ":",
			(proxyport ? proxyport : "8080"), NULL);
	    else
		proxy = obtain_proxy_setting();
	    if (proxy)
	    {
		char *auth = NULL;

		/* ここでPROXYの設定 */
		message_send(FRESHEN_MSG_USEPROXY, 0, proxy,
			"Will use proxy \"%s\"", proxy);
		if (proxyuser && proxypass)
		    auth = strdupv(proxyuser, ":", proxypass, NULL);
		_RPT2(_CRT_WARN, "USING PROXY: proxy=%s, auth=%s\n",
			proxy, auth);
		http_set_proxy(http, proxy, auth);
		free(auth);
		free(proxy);
	    }

	    http_set_useragent(http, USERAGENT);
	    http_set_cache(http, 0);
	    /* freshen起動 */
	    result = freshen_main(flag, infofile, default_url);

	    rcfile_close(rc);
	    http_close(http);
	    http = NULL;
	}
	progress_inc_hour();

	/* fleshoutを実行する */
	if (result == 0)
	{
	    _RPT0(_CRT_WARN, "No file was updated.\n");
	    message_send(FRESHEN_MSG_NOUPDATE, 0, 0, "No file was updated.");
	}
	else
	{
	    char *cmd, *option, *str;

	    /* オプション選択 */
	    if (result < 0)
	    {
		option = " -d";
		str = "Delete all temporary files";
	    }
	    else if (flag & FRESHEN_FLAG_BACKUP)
	    {
		option = " -b";
		str = "Make backup of old files";
	    }
	    else
	    {
		option = "";
		str = "Going to flesh out new files";
	    }

	    /* コマンドの実行 */
	    if (cmd = strdupv(CMD_FLESHOUT, option, NULL))
	    {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		_RPT1(_CRT_WARN, "Fleshout coomand: %s\n", cmd);

		message_send(FRESHEN_MSG_FLESHOUT, result, 0, str);
		memset(&si, 0, sizeof(si));
		CreateProcess(
			NULL,
			cmd,
			NULL,	/* Process security */
			NULL,	/* Thread security */
			FALSE,	/* Handle inheritance */
			CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS,
			NULL,	/* Environment */
			NULL,	/* Current directory */
			&si,
			&pi
			);
		free(cmd);
	    }
	}
	progress_inc_hour();

	/* 保存しておいたカレントディレクトリに戻す */
	if (curdir)
	{
	    SetCurrentDirectory(curdir);
	    _RPT1(_CRT_WARN, "Return direcotory: %s\n", curdir);
	    message_send(FRESHEN_MSG_CHDIR, 0, curdir,
		    "Return directory \"%s\"", curdir);
	    free(curdir);
	}
	progress_finish();
	WSACleanup();
	_RPT0(_CRT_WARN, "\n");
    }
    return result;
}

    BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
