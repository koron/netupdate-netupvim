/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 09-Oct-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void flush_dir_stub(char* dir);

static int flag_backup = 0;
static int flag_reheasal = 0;
static int flag_verbose = 0;
static int flag_delete = 0;

/*
 * Return value char** must be free().
 */
    char**
parse_string_arg(const char *str, int *argc)
{
    char **argv, *argstr, *pw;
    const char *p;
    int parse_mode;
    int len = 1;
    int argcount = 1; /* Include terminater NULL pointer */

    for (p = str, parse_mode = 0; *p; ++p, ++len)
    {
	switch (parse_mode)
	{
	    case 0:
		if (!isspace(*p))
		{
		    ++argcount;
		    parse_mode = *p != '"' ? 1 : 2;
		}
		break;
	    case 1:
		if (isspace(*p))
		    parse_mode = 0;
		else if (*p == '\\' && *(p + 1) == ' ')
		    ++p;
		break;
	    case 2:
		if (*p == '"')
		    parse_mode = 0;
		else if (*p == '\\' && *(p + 1) == '"')
		    ++p;
		break;
	}
    }

    if (argc)
	*argc = argcount - 1;
    argv = (char**)malloc(sizeof(char*) * argcount + len);
    argv[argcount - 1] = (char*)0;
    argstr = (char*)(argv + argcount);

    /* It is a problem there is parser twice */
    parse_mode = 0;
    argcount = 0;
    pw = argstr;
    for (p = str; *p; ++p)
    {
	switch (parse_mode)
	{
	    case 0:
		if (!isspace(*p))
		{
		    if (*p != '"')
		    {
			if (*p == '\\' && *(p + 1) == ' ')
			    ++p;
			argv[argcount++] = pw;
			*pw++ = *p;
			parse_mode = 1;
		    }
		    else
		    {
			argv[argcount++] = pw;
			parse_mode = 2;
		    }
		}
		break;
	    case 1:
		if (isspace(*p))
		{
		    *pw++ = '\0';
		    parse_mode = 0;
		}
		else
		{
		    if (*p == '\\' && *(p + 1) == ' ')
			++p;
		    *pw++ = *p;
		}
		break;
	    case 2:
		if (*p == '"')
		{
		    *pw++ = '\0';
		    parse_mode = 0;
		}
		else
		{
		    if (*p == '\\' && *(p + 1) == '"')
			++p;
		    *pw++ = *p;
		}
		break;
	}
    }
    *pw = '\0';
    return argv;
}

    char*
strdupv(char* str, ...)
{
    va_list args;
    char *buf, *tmp;
    int len;

    len = strlen(str) + 1;
    va_start(args, str);
    while (tmp = va_arg(args, char*))
	len += strlen(tmp);
    va_end(args);

    buf = (char*)malloc(len);
    strcpy(buf, str);
    va_start(args, str);
    while (tmp = va_arg(args, char*))
	strcat(buf, tmp);
    va_end(args);

    return buf;
}

    void
update_file(char* curfile, char* newfile)
{
    if (flag_reheasal)
	return;

    if (flag_delete)
	DeleteFile(newfile);
    else
    {
	if (flag_backup)
	{
	    char* oldfile = strdupv(curfile, "~", NULL);

	    DeleteFile(oldfile);
	    MoveFile(curfile, oldfile);
	    free(oldfile);
	}
	else
	    DeleteFile(curfile);
	MoveFile(newfile, curfile);
    }
}

    void
flush_dir(char* dir)
{
    char* dir2;

    switch (dir[strlen(dir) - 1])
    {
	case '\\':
	case '/':
	    flush_dir_stub(dir);
	    break;
	default:
	    if (dir2 = strdupv(dir, "/", NULL))
	    {
		flush_dir_stub(dir2);
		free(dir2);
	    }
	    break;
    }
}

    void
flush_dir_stub(char* dir)
{
    char *dir_pat;

    if (dir_pat = strdupv(dir, "*", NULL))
    {
	HANDLE hFind;
	WIN32_FIND_DATA find;

	hFind = FindFirstFile(dir_pat, &find);
	while (hFind != INVALID_HANDLE_VALUE)
	{
	    char *src = find.cFileName;
	    char *dest = strdup(src);
	    int len = strlen(dest);

	    /*
	     * 1. ディレクトリが相手の場合、削除してみる
	     * 2. ファイル名が','で終わっている時は削除してみる
	     */
	    if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
		if (strcmp(src, ".") && strcmp(src, ".."))
		{
		    char* destfull = strdupv(dir, dest, NULL);

#ifndef _WINDOWS
		    if (flag_verbose)
			printf("DIR: %s\n", destfull);
#endif
		    flush_dir(destfull);

		    free(destfull);
		}
	    }
	    else if (len > 1 && dest[len - 1] == ',')
	    {
		char *srcfull, *destfull;

		dest[len - 1] = '\0';
		srcfull = strdupv(dir, src, NULL);
		destfull = strdupv(dir, dest, NULL);
#ifndef _WINDOWS
		if (flag_verbose)
		    printf("SRC:%s DEST:%s\n", srcfull, destfull);
#endif
		update_file(destfull, srcfull);

		free(srcfull);
		free(destfull);
	    }
	    else if (len > 3 && !strcmp(&dest[len - 3], ",bz2"))
	    {
		if (flag_delete && !flag_reheasal)
		{
		    char *destful = strdupv(dir, dest, NULL);

		    DeleteFile(destful);
		    free(destful);
		}
	    }

	    free(dest);
	    if (!FindNextFile(hFind, &find))
	    {
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	    }
	}
	free(dir_pat);
    }
}

    int
main(int argc, char** argv)
{
    char **args;
    int cnt = 0;

    for (args = argv + 1; *args; ++args)
    {
	if ((*args)[0] != '-')
	{
	    ++cnt;
	    continue;
	}
	else if (!strcmp(*args, "-b") || !strcmp(*args, "--backup"))
	    flag_backup = 1;
	else if (!strcmp(*args, "-d") || !strcmp(*args, "--delete"))
	    flag_delete = 1;
	else if (!strcmp(*args, "-r") || !strcmp(*args, "--rehearsel"))
	    flag_reheasal= 1;
	else if (!strcmp(*args, "-v") || !strcmp(*args, "--verbose"))
	    flag_verbose = 1;
    }

    if (cnt > 0)
    {
	for (args = argv + 1; *args; ++args)
	    if ((*args)[0] != '-')
		flush_dir(*args);
    }
    else
	flush_dir(".");

    return 0;
}

    int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    char **argv;
    char *cmdline;
    char module[4096];
    int result = -1;

    GetModuleFileName(NULL, module, sizeof(module) / sizeof(TCHAR));
    if (cmdline = strlen(lpCmdLine)
	    ? strdupv("\"", module, "\" ", lpCmdLine, NULL)
	    : strdupv("\"", module, "\"", NULL))
    {
	if (argv = parse_string_arg(cmdline, &argc))
	{
	    Sleep(200);
	    result = main(argc, argv);
	    free(argv);
	}
	free(cmdline);
    }
    return result;
}
