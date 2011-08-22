/* vim:set ts=8 sw=4 sts=4 tw=0: */
/*
 * Last Change: 09-Nov-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef DLL_FRESHEN_H
#define DLL_FRESHEN_H

#define FRESHEN_FLAG_BACKUP (1 << 0)

/* メッセージID */
#define FRESHEN_MSG_NULL	0x00000000
#define FRESHEN_MSG_START	0x00000001
#define FRESHEN_MSG_CHDIR	0x00000002
#define FRESHEN_MSG_NOUPDATE	0x00000003
#define FRESHEN_MSG_FLESHOUT	0x00000004
#define FRESHEN_MSG_BADINFO	0x00000005
#define FRESHEN_MSG_GOODINFO	0x00000006
#define FRESHEN_MSG_SCHEDULE	0x00000007
#define FRESHEN_MSG_CONFIRM	0x00000008
#define FRESHEN_MSG_CANCEL	0x00000009
#define FRESHEN_MSG_DLBEGIN	0x0000000a
#define FRESHEN_MSG_DLEND	0x0000000b
#define FRESHEN_MSG_DECOMPRESS	0x0000000c
#define FRESHEN_MSG_BADMD5	0x0000000e
#define FRESHEN_MSG_GOODMD5	0x0000000f
#define FRESHEN_MSG_DLALLEND	0x00000010
#define FRESHEN_MSG_UPDATEINFO	0x00000011
#define FRESHEN_MSG_DLINFO	0x00000012
#define FRESHEN_MSG_USEPROXY	0x00000013

#define FRESHEN_CANCEL_NOFILE	0
#define FRESHEN_CANCEL_CONFIRM	1
#define FRESHEN_CANCEL_DOWNLOAD	2

#ifdef __cplusplus
# define C_DECL_BEGIN() extern "C" {
# define C_DECL_END() }
#else
# define C_DECL_BEGIN()
# define C_DECL_END()
#endif

typedef struct freshen_md5check_t
{
    char *name;
    char *md5;
    char *md5_req;
}
FRESHEN_MD5CHECK_T;

typedef struct freshen_progress_t
{
    int hour;
    int hour_last;
    int minute;
    int minute_last;
}
FRESHEN_PROGRESS_T;

/* userdata, msgid, param1, param2, msgstr */
typedef int (*FRESHEN_MSGPROC)(void*, unsigned int, int, void*, char*);
typedef int (*FRESHEN_PROGRESS)(void*, FRESHEN_PROGRESS_T*);

C_DECL_BEGIN();
int Freshen(unsigned int flag, char* infofile, char* default_url);
char* FreshenLastErrorString();
void FreshenSetMessageProc(FRESHEN_MSGPROC proc, void* data);
void FreshenSetProgressProc(FRESHEN_PROGRESS proc, void* data);
C_DECL_END();

#endif /* DLL_FRESHEN_H */
