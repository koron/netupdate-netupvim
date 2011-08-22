/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 04-Apr-2002.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "xmlparse.h"
#ifdef _WIN32
# include <crtdbg.h>
# pragma comment(lib, "expat_mt")
#endif

#include "wordbuf.h"
#include "upinfo.h"
#include "etclib.h"

    wordstack_t*
wordstack_push(wordstack_t* prev)
{
    wordstack_t *stack = (wordstack_t*)malloc(sizeof(wordstack_t));

    if (stack)
    {
	stack->prev = prev;
	stack->buf = wordbuf_open();
    }
    return stack;
}

    wordstack_t*
wordstack_pop(wordstack_t* stack)
{
    if (!stack)
	return stack;
    else
    {
	wordstack_t* prev = stack->prev;

	wordbuf_close(stack->buf);
	free(stack);
	return prev;
    }
}

/*****************************************************************************/

    void
updatefile_dump(updatefile_t *updatefile, FILE *fp)
{
    fprintf(fp, "updatefile_t(%p)\n", updatefile);
    fprintf(fp, "  compress_type=%d\n", updatefile->compress_type);
    fprintf(fp, "  path=%s\n", updatefile->path);
    fprintf(fp, "  verify_type=%d, verify_data=%s\n",
	    updatefile->verify_type, updatefile->verify_data);
    fprintf(fp, "  size=%d\n", updatefile->size);
    fprintf(fp, "  prev=%p\n", updatefile->prev);
}

    updatefile_t*
updatefile_open()
{
    updatefile_t* updatefile = (updatefile_t*)calloc(1, sizeof(updatefile_t));

    if (updatefile)
    {
	updatefile->compress_type = UPDATEFILE_COMPRESSTYPE_NONE;
	updatefile->verify_type = UPDATEFILE_VERIFYTYPE_DEFAULT;
	updatefile->size = -1;
    }
    return updatefile;
}

    updatefile_t*
updatefile_close(updatefile_t* updatefile)
{
    updatefile_t* prev = NULL;

    if (updatefile)
    {
	prev = updatefile->prev;
	free(updatefile->path);
	free(updatefile);
    }
    return prev;
}

    void
info_dump(info_t* info, FILE* fp)
{
    fprintf(fp, "info_t(%p)\n", info);
    fprintf(fp, "  depth=%d, stack=%p, mode=%d\n",
	    info->depth, info->stack, info->mode);
    fprintf(fp, "  name=%s\n", info->name);
    fprintf(fp, "  url=%s\n", info->url);
    fprintf(fp, "  baseurl=%s\n", info->baseurl);
    fprintf(fp, "  curfile=%p\n", info->curfile);
    if (info->curfile)
	updatefile_dump(info->curfile, fp);
    fprintf(fp, "  filelist=%p\n", info->filelist);
    if (info->filelist)
    {
	updatefile_t *p;

	for (p = info->filelist; p; p = p->prev)
	    updatefile_dump(p, fp);
    }
}

    info_t*
info_open()
{
    info_t *info = (info_t*)calloc(1, sizeof(info_t));

    return info;
}

    void
info_reset(info_t* info)
{
    /* infoの初期化/中身を空っぽにする */
    if (!info)
	return;

    while (info->stack)
	info->stack = wordstack_pop(info->stack);
    free(info->name);
    free(info->url);
    free(info->baseurl);

    updatefile_close(info->curfile);
    while (info->filelist)
	info->filelist = updatefile_close(info->filelist);

    memset(info, 0, sizeof(info_t));
}

    void
info_close(info_t* info)
{
    if (info)
    {
	info_reset(info);

	free(info);
    }
}

/*****************************************************************************/

/*
 * 先行する"."と"/"と"\\"を読み飛ばす。セキュリティ上の措置。
 */
    static char*
verify_path(char* path)
{
    while (strchr("./\\", *path))
	++path;
    return path;
}

/*
 * 空白文字をまとめて一文字の空白文字(0x20)にし、
 * 文字列の先頭と末尾の空白文字を削除する。
 *	:s/\s\+/ /g
 *	:s/^ \| $//
 */
    static int
xml_chomp(char* p)
{
    int read = 0, write = 0;
    int last = -1;
    int ch;

    if (!p)
	return -1;
    /* Remove lead spaces and replace successive spaces with a space
     * character */
    do
    {
	ch = p[read++];
	if (!isspace(ch))
	{
	    last = -1;
	    p[write++] = ch;
	}
	else if (write > 0 && p[write - 1] != ' ')
	{
	    last = write;
	    p[write++] = ' ';
	}
    }
    while (ch);
    --write; /* Don't count trailing NUL character for length */
    /* Remove trail spaces */
    if (last >= 0)
    {
	p[last] = '\0';
	write = last;
    }

    return write;
}

/*****************************************************************************/

    static void
charData(void *userData, const char *name, const int len)
{
    int i;
    info_t *info = userData;

    if (info->stack && info->stack->buf)
	for (i = 0; i < len; ++i)
	    wordbuf_add(info->stack->buf, name[i]);
}

    static void
startElement(void *userData, const char *name, const char **atts)
{
    wordstack_t *stack;
    info_t *info = userData;

    info->depth += 1;
    if (stack = wordstack_push(info->stack))
	info->stack = stack;

    if (!stricmp(name, "file") && !info->curfile)
    {
	info->curfile = updatefile_open();
	if (atts)
	    while (*atts)
	    {
		if (!stricmp(*atts, "compress") && *(atts + 1))
		{
		    ++atts;
		    if (!stricmp(*atts, "bz2"))
			info->curfile->compress_type =
			    UPDATEFILE_COMPRESSTYPE_BZ2;
		}
		++atts;
	    }
    }
}

    static void
endElement(void *userData, const char *name)
{
    int len;
    info_t *info = userData;
    char *value;

    info->depth -= 1;
    if (info->stack && info->stack->buf
	    && (len = xml_chomp(value = wordbuf_get(info->stack->buf))) > 0)
    {
	if (!stricmp(name, "uri") && !info->url)
	    info->url = strdup(value);
	else if (!stricmp(name, "name"))
	    info->name = strdup(value);
	else if (info->curfile)
	{
	    if (!stricmp(name, "path"))
		info->curfile->path = strdup(verify_path(value));
	    else if (!stricmp(name, "verify"))
		strcpy(&info->curfile->verify_data[0], value);
	    else if (!stricmp(name, "size"))
		info->curfile->size = atoi(value);
	}
    }
    if (!stricmp(name, "file") && info->curfile)
    {
	info->curfile->prev = info->filelist;
	info->filelist = info->curfile;
	info->curfile = NULL;
    }
    info->stack = wordstack_pop(info->stack);
}

    info_t*
readinfo_fh(info_t* info, FILE* fp)
{
    char	buf[BUFSIZ];
    int		done;
    XML_Parser	parser;

    if (!info)
	return NULL;
    /* infoの初期化/中身を空っぽにする */
    info_reset(info);

    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, info);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, charData);
    do
    {
	size_t len = fread(buf, 1, sizeof(buf), fp);

	done = len < sizeof(buf);
	if (!XML_Parse(parser, buf, len, done))
	{
	    fprintf(stderr,
		    "%s at line %d\n",
		    XML_ErrorString(XML_GetErrorCode(parser)),
		    XML_GetCurrentLineNumber(parser));
#ifdef _WIN32
	    _RPT2(_CRT_WARN, "%s at line %d\n",
		    XML_ErrorString(XML_GetErrorCode(parser)),
		    XML_GetCurrentLineNumber(parser));
#endif
	    return NULL;
	}
    }
    while (!done);

    XML_ParserFree(parser);

    if (info->url && !info->baseurl)
    {
	char *tail = strrchr(info->url, '/');

	if (tail)
	    info->baseurl = strndup(info->url, tail - info->url + 1);
    }

    return info;
}

/*
 * Infoファイルを読み込むラッピング関数
 */
    info_t*
readinfo(info_t* info, char* filename)
{
    FILE *fp;
    info_t *result = NULL;

    if (filename && info && (fp = fopen(filename, "rt")))
    {
	result = readinfo_fh(info, fp);
	fclose(fp);
    }
    return result;
}

/*****************************************************************************/

#if !defined(_USRDLL) && !defined(_LIB)
    int
main(int argc, char** argv)
{
    info_t *info;

    if (!(info = info_open()))
	return -1;
    if (readinfo_fh(info, stdin))
    {
	info_dump(info, stdout);
	info_close(info);
    }

    return 0;
}
#endif /* !defined(_USRDLL) && !defined(_LIB) */
