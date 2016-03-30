#include "surffs_helpers.h"
#include "surffs_debug.h"
#include <linux/slab.h>

static inline void zero_sfs_str(sfs_string *str)
{
    str->data = 0;
    str->memlen = 0;
    str->textlen = 0;
}

int sfs_string_create(sfs_string *str, const char *data)
{
    size_t src_strlen;
    src_strlen = strlen(data);
    if (!src_strlen) {zero_sfs_str(str); return -EINVAL;}

    str->data = kstrdup(data, GFP_KERNEL);
    if (str->data)
    {
        str->memlen = src_strlen + 1;
        str->textlen = src_strlen;
        return 0;
    }
    else {zero_sfs_str(str); return -ENOMEM;}
}

int sfs_string_createz(sfs_string *str, size_t alloc_len)
{
    if (!alloc_len) {zero_sfs_str(str); return -EINVAL;}

    str->data = kmalloc(alloc_len, GFP_KERNEL);
    if (str->data)
    {
        str->data[0] = 0;
        str->memlen = alloc_len;
        str->textlen = 0;
        return 0;
    } else {zero_sfs_str(str); return -ENOMEM;}
}

void sfs_string_bind(sfs_string *str, char *data)
{
    str->data = data;
    str->textlen = strlen(data);
    str->memlen = str->textlen + 1;
}

int sfs_string_expandmem(sfs_string *str, size_t new_alloc_len)
{
    if (new_alloc_len <= str->memlen) return -EINVAL;

    str->data = krealloc(str->data, new_alloc_len ,GFP_KERNEL);
    if (str->data)
    {
        str->memlen = new_alloc_len;
        return 0;
    } else return -EINVAL;
}

int sfs_string_cat_param(sfs_string *str, const char* fmt, char* param)
{
    int ret = 0;
    size_t newmemlen;

    size_t src_strlen = strlen(fmt) + strlen(param);
    size_t new_chars_max = str->memlen - str->textlen - 1;

    if (src_strlen > new_chars_max)
    {
        newmemlen = str->memlen + (src_strlen - new_chars_max);
        ret = sfs_string_expandmem(str, newmemlen);
        if (ret) return ret;
        str->memlen = newmemlen;
    }

    str->textlen += sprintf(str->data + str->textlen, fmt, param);

    return 0;
}

int sfs_string_ncat(sfs_string *str, const char* data, size_t len)
{
    int ret = 0;
    size_t newmemlen;

    size_t src_strlen = len;
    size_t new_chars_max = str->memlen - str->textlen - 1;

    if (src_strlen > new_chars_max)
    {
        newmemlen = str->memlen + (src_strlen - new_chars_max);
        ret = sfs_string_expandmem(str, newmemlen);
        if (ret) return ret;
        str->memlen = newmemlen;
    }

    strncat(str->data, data, len);
    str->textlen += src_strlen;

    return 0;
}

int sfs_string_cat(sfs_string *str, const char* data)
{
    int ret = 0;
    size_t newmemlen;

    size_t src_strlen = strlen(data);
    size_t new_chars_max = str->memlen - str->textlen - 1;

    if (src_strlen > new_chars_max)
    {
        newmemlen = str->memlen + (src_strlen - new_chars_max);
        ret = sfs_string_expandmem(str, newmemlen);
        if (ret) return ret;
        str->memlen = newmemlen;
    }

    strcat(str->data, data);
    str->textlen += src_strlen;

    return 0;
}

int sfs_string_insert_begin(sfs_string *str, const char* data)
{
    int ret = 0;
    sfs_string tmp = {0};

    ret = sfs_string_create(&tmp, str->data);
    if (ret) goto out;

    ret = sfs_string_set(str, data);
    if (ret) goto out;

    ret = sfs_string_cat(str, tmp.data);
    if (ret) goto out;

out:
    sfs_string_free(&tmp);
    return ret;
}

int sfs_string_clear(sfs_string *str)
{
    if (!str->memlen) return -EINVAL;

    str->data[0] = 0;
    str->textlen = 0;

    return 0;
}

void sfs_string_free(sfs_string *str)
{
    if (str->data) kfree(str->data);
    str->memlen = 0;
    str->textlen = 0;
}

int sfs_string_set(sfs_string *str, const char* data)
{
    int ret = 0;

    ret = sfs_string_clear(str); if (ret) return ret;
    ret = sfs_string_cat(str, data); if (ret) return ret;

    return 0;
}






