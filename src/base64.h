/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 18-Nov-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef BASE64_H
#define BASE64_H

#ifdef __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN
#endif

EXTERN char* base64_encode(char* in);

#endif /* BASE64_H */
