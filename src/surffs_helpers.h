#ifndef _SURFFS_HELPERS_H_
#define _SURFFS_HELPERS_H_

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/hashtable.h>




//some string helpers.
//TODO: maybe it is already implemented somewhere in linux kernel?
typedef struct
{
    char *data;
    size_t textlen;
    size_t memlen;
} sfs_string;

int sfs_string_create(sfs_string *str, const char *data);
int sfs_string_createz(sfs_string *str, size_t alloc_len);
int sfs_string_expandmem(sfs_string *str, size_t new_alloc_len);
int sfs_string_cat(sfs_string *str, const char* data);
int sfs_string_insert_begin(sfs_string *str, const char* data);
int sfs_string_ncat(sfs_string *str, const char* data, size_t len);
int sfs_string_set(sfs_string *str, const char* data);
int sfs_string_cat_param(sfs_string *str, const char* fmt, char* param);
int sfs_string_clear(sfs_string *str);
void sfs_string_free(sfs_string *str);
void sfs_string_bind(sfs_string *str, char *data);


#endif
