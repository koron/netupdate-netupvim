/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * Last Change: 08-Oct-2001.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#ifndef UPINFO_H
#define UPINFO_H

#include "cdecl.h"

#ifndef WORDBUF_H
typedef void wordbuf;
#endif
typedef struct _wordstack_t
{
    wordbuf* buf;
    struct _wordstack_t* prev;
} wordstack_t;

#define UPDATEFILE_COMPRESSTYPE_NONE 0
#define UPDATEFILE_COMPRESSTYPE_BZ2 1
#define UPDATEFILE_VERIFYTYPE_MD5 0
#define UPDATEFILE_VERIFYTYPE_DEFAULT UPDATEFILE_VERIFYTYPE_MD5

typedef struct _updatefile_t {
    unsigned int flag;

    int compress_type;
    char* path;
    int verify_type;
    char verify_data[64];
    int size;

    struct _updatefile_t* prev;
} updatefile_t;

typedef struct _info_t {
    int depth;
    wordstack_t *stack;
    int mode;
    updatefile_t* curfile;

    char* name;
    char* url;
    char* baseurl;
    updatefile_t* filelist;
} info_t;

C_DECL_BEGIN();
wordstack_t* wordstack_push(wordstack_t* prev);
wordstack_t* wordstack_pop(wordstack_t* stack);

void updatefile_dump(updatefile_t *updatefile, FILE *fp);

info_t* info_open();
void info_close(info_t* info);
void info_reset(info_t* info);
void info_dump(info_t* info, FILE* fp);
info_t* readinfo_fh(info_t* info, FILE* fp);
info_t* readinfo(info_t* info, char* filename);
C_DECL_END();

#endif /* UPINFO_H */
