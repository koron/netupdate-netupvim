/* vi:set ts=8 sts=4 sw=4 tw=0: */
/*
 * md5lib.h - md5 lib import header.
 *
 * Maintainer:  Muraoka Taro <koron@tka.att.ne.jp>
 * Last Change: 01-Oct-2001.
 */

#ifndef MD5_LIB
#define MD5_LIB

#if !defined(C_DECL_BEGIN) && !defined(C_DECL_END)
# ifdef __cplusplus
#  define C_DECL_BEGIN() extern "C" {
#  define C_DECL_END() }
# else
#  define C_DECL_BEGIN()
#  define C_DECL_END()
# endif
#endif

/* MD5 context. */
typedef struct {
  unsigned int state[4];
  unsigned int count[2];
  unsigned char buffer[64];
} MD5_CTX;

C_DECL_BEGIN();
void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);
#ifndef MD5_LEAN_AND_MEAN
void MD5toa(unsigned char *digest, char *md5a);
void toMD5(char *str, char *md5);
void ftoMD5(FILE *fp, char *md5);
void msgMD5(FILE *fs, FILE *tc, char *md5a);
#endif
C_DECL_END();

#endif MD5_LIB
