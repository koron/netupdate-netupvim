/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 08-Oct-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "bzlib.h"
#include "etclib.h"

#pragma comment(lib, "bz2_mt")

    char*
compressname_bz2(char* file)
{
    int len = strlen(file);
    char* out = malloc(len + 5);

    if (out)
    {
	memcpy(out, file, len);
	if (out[len - 1] == ',')
	    --len;
	memcpy(out + len, ",bz2", 5);
    }
    return out;
}

    static int
decompress_bz2_stub(char* out, char* bz2)
{
    char buf[8192];
    int len, size = -1;
    FILE* fpout = NULL;
    BZFILE* fpbz = NULL;

    if (fpbz = BZ2_bzopen(bz2, "rb"))
	fpout = fopen(out, "wb");
    if (fpout && fpbz)
    {
	size = 0;
	while ((len = BZ2_bzread(fpbz, buf, sizeof(buf))) > 0)
	    size += fwrite(buf, 1, len, fpout);
    }
    if (fpout)
	fclose(fpout);
    if (fpbz)
	BZ2_bzclose(fpbz);
    return size;
}

    int
decompress_bz2(char* file)
{
#if 0
    int res = -1;
    char* bz2file = compressname_bz2(file);

    if (bz2file)
    {
	res = decompress_bz2_stub(file, bz2file);
	free(bz2file);
    }
    return res;
#else
    int res = -1;
    char* dest = strndup(file, strlen(file) - 3);
    _RPT1(_CRT_WARN, "decompress_bz2(\"%s\")\n", file);

    if (dest)
    {
	_RPT1(_CRT_WARN, "  dest=\"%s\"\n", dest);
	res = decompress_bz2_stub(dest, file);
	free(dest);
    }
    return res;
#endif
}

#if !defined(_USRDLL) && !defined(_LIB)
    int
main(int argc, char** argv)
{
    int ret;

    if (!argv[1])
    {
	fprintf(stderr, "%s: No filename\n", argv[0]);
	return -1;
    }
    ret = decompress_bz2(argv[1]);
    printf("decompress_bz2()=%d\n", ret);
    return 0;
}
#endif /* !defined(_USRDLL) && !defined(_LIB) */
