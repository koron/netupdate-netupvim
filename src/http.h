/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 13-Jan-2003.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef HTTP_H
#define HTTP_H

#include "cdecl.h"

#define HTTP_PORT 80
#define HTTP_GETERROR -1
#define HTTP_GETERROR_TERMINATED -2

typedef int (*HTTP_GET_PROGRESS)(void*, int);
typedef struct _http_t http_t;

C_DECL_BEGIN();
http_t* http_open();
void http_close(http_t* http);

/* ÉGÉâÅ[éûÇ…ÇÕïâÇÃêîÇï‘Ç∑ */
int http_get(http_t* http, char* filename, char* url);

void http_set_cache(http_t* http, int state);
void http_set_proxy(http_t* http, const char* proxy, const char* auth);
void http_set_useragent(http_t* http, const char* useragent);
void http_set_proc_getprogress(http_t* http, HTTP_GET_PROGRESS proc, void* value);
C_DECL_END();

#endif /* HTTP_H */
