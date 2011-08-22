/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * base64.c - Base64 encoder
 *
 * Last Change: 18-Nov-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

/*
 * 資料:
 *  割り当てるASCIIキャラクタは、6bit値が、0x00の時Aで、0x01の時Bで、....
 *  0x19の時Zで、0x1aの時aで、0x1bの時bで、....0x33の時zで、0x34の時0で、
 *  0x35の時1で、....0x3dの時9で、0x3eの時+で、0x3fの時/です。
 *
 *  バイトが足りない場合、ヌル(0x00)を一つパッドして入れられ計算される。変換
 *  後は、4の倍数個にするためのパッドとして、"=(イコール)"がパッドとして利用
 *  される。(変換後は、"A-Z,a-z,0-9,/"で、"="はない)
 *
 *  例えば、i(0x69) の場合、i(0x69) + NULL(0x00) - > aQ変換後は、4の倍数にす
 *  るため aQ -> aQ==
 *
 * 参考URL: http://isweb5.infoseek.co.jp/diary/sanaki/code/encode.htm
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char s_base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define B64(a) s_base64_table[(a)]

    static void
base64_encode_core(char out[4], const char* in, int len)
{
    unsigned char buf[3] = {'\0', '\0', '\0'};

    memcpy(buf, in, len);

    out[0] = len > 0 ? B64(  (buf[0] >> 2)                  & 0x3f ) : '=';
    out[1] = len > 0 ? B64( ((buf[0] << 4) | (buf[1] >> 4)) & 0x3f ) : '=';
    out[2] = len > 1 ? B64( ((buf[1] << 2) | (buf[2] >> 6)) & 0x3f ) : '=';
    out[3] = len > 2 ? B64(   buf[2]                        & 0x3f ) : '=';
}

    char*
base64_encode(char* in)
{
    int in_len, out_len, len;
    char *out, *read, *write;

    in_len = strlen(in);
    if (in_len <= 0)
	return NULL;
    /*printf("base64_encode: in_len=%d out_len=%d\n", in_len, out_len);*/

    out_len = (in_len + 2) / 3 * 4;
    out = (char*)malloc(out_len + 1);
    if (!out)
	return NULL;
    out[out_len] = '\0';

    len = in_len;
    read = in;
    write = out;
    while (len >= 3)
    {
	base64_encode_core(write, read, 3);
	len -= 3;
	read += 3;
	write += 4;
    }
    if (len > 0)
	base64_encode_core(write, read, len);

    return out;
}

#if !defined(_USRDLL) && !defined(_LIB)
/*
 * テスト用main()
 */
    int
main(int argc, char** argv)
{
    char *out;

    if (argc > 1)
    {
	out = base64_encode(argv[1]);
	printf("base64: %s -> %s\n", argv[1], out);
	free(out);
    }
    else
    {
	out = base64_encode("i");
	printf("base64: %s -> %s\n", "i", out);
	free(out);
    }
    return 0;
}
#endif
