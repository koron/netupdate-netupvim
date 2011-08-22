/* vim:set ts=8 sw=4 sts=4 tw=0: */
/*
 * Last Change: 18-Nov-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <md5lib.h>
#pragma comment(lib, "md5")

#include "etclib.h"

    int
is_directory(const char* path)
{
    DWORD dw = GetFileAttributes(path);
    return (dw != -1 && (dw & FILE_ATTRIBUTE_DIRECTORY)) ? 1 : 0;
}

    void
md5_file(char* md5, char* filepath)
{
    /* md5には33バイト以上必要。'\0'が付加される。 */
    FILE *fp;

    if (!(fp = fopen(filepath, "rb")))
	*md5 = '\0';
    else
    {
	ftoMD5(fp, md5);
	fclose(fp);
    }
}

    char*
mkdir_p(char* path)
{
    int loop = 1;
    char *dup, *p;

    if (!(dup = p = strdup(path)))
	return NULL;
    while (loop)
    {
	if (*p == '\0')
	    loop = 0;
	if ((p > dup && p[-1] != ':' && p[-1] != '/')
		&& (*p == '/' || *p == '\\' || *p == '\0'))
	{
	    *p = '\0';
	    if (!is_directory(dup))
	    {
		if (!CreateDirectory(dup, NULL))
		{
		    free(dup);
		    return NULL;
		}
	    }
	    *p = '/';
	}
	++p;
    }
    free(dup);
    return path;
}

    char*
strdupv(const char* str, ...)
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

    char*
strndup(const char* src, int len)
{
    char *p = NULL;

    if (src && len >= 0 && (p = malloc(len + 1)))
    {
	memcpy(p, src, len);
	p[len] = '\0';
    }
    return p;
}
