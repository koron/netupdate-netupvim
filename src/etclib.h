/* vim:set ts=8 sw=4 sts=4 tw=0: */
/*
 * Last Change: 18-Nov-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef ETCLIB_H
#define ETCLIB_H

#include "cdecl.h"

/* md5ÇÕchar[33]à»è„ïKóv */
C_DECL_BEGIN();
int is_directory(const char* path);
void md5_file(char* md5, char* filepath);
char* mkdir_p(char* path);
char* strdupv(const char* str, ...);
char* strndup(const char* src, int len);
C_DECL_END();

#endif /* ETCLIB_H */
