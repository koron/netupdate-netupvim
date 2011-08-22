/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 10-Feb-2002.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef RCFILE_H
#define RCFILE_H

#define RC_MAXLINE_LEN 512
typedef struct _rcfile_t rcfile_t;

#ifdef __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN
#endif

EXTERN rcfile_t*	rcfile_open(const char* filename);
EXTERN void		rcfile_close(rcfile_t* rcfile);
EXTERN const char*	rcfile_get(rcfile_t* rcfile, const char* key);

#endif /* RCFILE_H */
