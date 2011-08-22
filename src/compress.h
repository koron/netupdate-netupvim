/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 08-Oct-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef COMPRESS_H
#define COMPRESS_H

#include "cdecl.h"

C_DECL_BEGIN();
char* compressname_bz2(char* file);
/* サイズが返る。失敗すると負数 */
int decompress_bz2(char* file);
C_DECL_END();

#endif /* COMPRESS_H */
