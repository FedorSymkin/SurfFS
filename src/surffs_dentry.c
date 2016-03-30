#include "surffs_dentry.h"
#include "surffs_debug.h"

int surffs_dentry_delete(const struct dentry *dentry)
{
    sfs_enter();
    sfs_debug("surffs_dentry_delete: '%s'\n", dentry->d_name.name);
    sfs_leave();
    return 0;
}

void surffs_dentry_release(struct dentry *dentry)
{
    sfs_enter();
    sfs_debug("surffs_dentry_release: '%s'\n", dentry->d_name.name);
    sfs_leave();
}

static int reverse_path(sfs_string *path, sfs_string *reversed)
{
    int ret = 0;
    char *start;
    char *i;
    char *end;

    ret = sfs_string_clear(reversed);
    if (ret) goto out;

    end = path->data + path->textlen;
    for (i = path->data + path->textlen; ; i--)
    {
        if (*i == '/') start = i + 1;
        else if (i == path->data) start = i;
        else start = 0;

        if (start)
        {
            if (end > start)
            {
                ret = sfs_string_cat(reversed, "/");
                if (ret) goto out;

                ret = sfs_string_ncat(reversed, start, end - start);
                if (ret) goto out;
            }

            end = start - 1;
        }

        if (i == path->data) break;
    }

out:
    return ret;
}

int get_relative_dentry_path(struct dentry *dentry, sfs_string *path)
{
    int ret = 0;
    struct super_block *sb = dentry->d_sb;
    sfs_string tmp = {0};

    sfs_enter();
    sfs_trace("get_relative_dentry_path\n");

    ret = sfs_string_clear(path);
    if (ret) goto out;

    ret = sfs_string_createz(&tmp, 256);
    if (ret) goto out;

    for (; dentry && (dentry != sb->s_root); dentry = dentry->d_parent)
    {
        ret = sfs_string_cat(&tmp, dentry->d_name.name);
        if (ret) goto out;

        ret = sfs_string_cat(&tmp, "/");
        if (ret) goto out;
    }

    ret = reverse_path(&tmp, path);
    if (ret) goto out;

out:
    sfs_string_free(&tmp);
    sfs_leave();
    return ret;
}

int get_relative_path_to_surffs_root(struct dentry *dentry, sfs_string *path)
{
    int ret = 0;
    struct super_block *sb = dentry->d_sb;

    ret = sfs_string_clear(path);
    if (ret) goto out;

    for (; dentry && (dentry != sb->s_root); dentry = dentry->d_parent)
    {
        ret = sfs_string_cat(path, "../");
        if (ret) goto out;
    }

    if (path->textlen)
    {
        path->data[path->textlen - 1] = 0;
        path->textlen--;
    }

out:
    return ret;
}
