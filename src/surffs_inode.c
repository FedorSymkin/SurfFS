#include <linux/aio.h>
#include <linux/uio.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <asm/uaccess.h>
#include "surffs_inode.h"
#include "surffs_debug.h"
#include "surffs_ops.h"
#include "surffs_helpers.h"
#include "surffs.h"
#include "surffs_webpages.h"
#include "surffs_sb.h"
#include "surffs_dentry.h"


inline struct SURFFS_INODE_PRIVATE* SURFFS_INODE(struct inode *inode)
{
    if (!inode->i_private) {sfs_error("Null pointer to inode private!");}
    return inode->i_private;
}

static void free_inode_private(struct SURFFS_INODE_PRIVATE* prvt)
{
    sfs_string_free(&prvt->webPath);
    sfs_string_free(&prvt->linkto);
}

int surffs_create_inode(struct super_block* sb, struct inode *dir,
                                  umode_t mode, unsigned long ino,
                                  enum SURFFS_INODE_TYPE type, int file_size,
                                  struct inode **output_inode)
{
    int ret = 0;
    struct inode* inode = 0;
    struct SURFFS_INODE_PRIVATE* prvt = 0;

    sfs_enter();
    sfs_debug("surffs_create_inode, mode = %u\n", (unsigned int)mode);

    if (dir)
        {sfs_debug("dir ino = %lu\n", dir->i_ino);}


    inode = new_inode(sb);
    if (!inode) {ret = -ENOMEM; goto out;}

    inode->i_ino = ino;
    inode_init_owner(inode, dir, mode);
    inode->i_version = 1;
    inode->i_generation = get_seconds();
    inode->i_generation |= 1;
    inode->i_size = file_size;
    //inode->i_mapping->a_ops = &empty_aops;

    switch (mode & S_IFMT)
    {
        case S_IFDIR:
            inode->i_op = &surffs_inode_dir_ops;
            inode->i_fop = &surffs_file_dir_ops;
        break;

        case S_IFLNK:
            inode->i_op = &surffs_symlink_inode_operations;
        break;

        case S_IFREG:
            inode->i_op = &surffs_inode_reg_ops;
            inode->i_fop = &surffs_file_reg_ops;
        break;
    }

    inode->i_blocks = 0;
    inode->i_ctime = inode->i_atime = inode->i_mtime = CURRENT_TIME_SEC;
    if (dir) dir->i_mtime = dir->i_ctime = CURRENT_TIME;

    prvt = kzalloc(sizeof(struct SURFFS_INODE_PRIVATE), GFP_KERNEL);
    if (!prvt) {ret = -ENOMEM; goto out;}

    prvt->type = type;
    ret = sfs_string_createz(&prvt->webPath, 64);
    if (ret) goto out;

    ret = sfs_string_createz(&prvt->linkto, 64);
    if (ret) goto out;

    inode->i_private = prvt;

    insert_inode_hash(inode);
    *output_inode = inode;

    sfs_debug("create inode OK: %ld\n", inode->i_ino);

out:
    if (ret)
    {
        if (prvt)
        {
            free_inode_private(prvt);
            kfree(prvt);
        }
        if (inode) iput(inode);
    }
    sfs_leave();
    return ret;
}

int surffs_delete_inode(struct inode *inode)
{
    sfs_enter();
    sfs_debug("delete inode ino %ld\n", inode->i_ino);
    if (inode->i_private)
    {
        free_inode_private(SURFFS_INODE(inode));
        kfree(inode->i_private);
    }
    sfs_leave();
    return 1;
}

static int get_webpath_by_dentry_name(struct inode *dir, const char* dentry_name, char **webpath)
{
    int ret = 0;
    struct SURFFS_WEB_PAGE *webpage = 0;
    struct list_head *pos;
    struct SURFFS_HTML_LINK* link;

    sfs_enter();
    sfs_debug("get_webpath_by_dentry_name: dir inode ino = %ld; dentry name = '%s'\n",
                            dir->i_ino, dentry_name);

    *webpath = 0;

    webpage = SURFFS_INODE(dir)->webpage;
    if (!webpage) {ret = -EINVAL; goto out;}

    list_for_each(pos, &webpage->html_links)
    {
        link = list_entry(pos, struct SURFFS_HTML_LINK, html_links);
        if (strcmp(link->title.data, dentry_name) == 0)
        {
            sfs_debug("found link, path = '%s'\n", link->path.data);
            *webpath = link->path.data;
            goto out;
        }
    }

    sfs_debug("cannot find link with title '%s' on inode %ld\n",
                               dentry_name, dir->i_ino);

out:
    sfs_leave();
    return ret;
}

typedef struct
{
    const char *filename;
    int type;
} surffs_special_file_desc;

const surffs_special_file_desc special_files[] =
{
    {.filename="page.html",     .type=INODE_FILE_PAGE},
    {.filename="url",           .type=INODE_FILE_URL},
    {.filename="status",        .type=INODE_FILE_STATUS},
    {.filename="loading.log",   .type=INODE_FILE_LOG},
    {.filename=0,               .type=0}
};

#define SPECIAL_FILES_COUNT ( (sizeof(special_files) / sizeof(surffs_special_file_desc)) - 1 )

static int get_file_size(enum SURFFS_INODE_TYPE type,
                         struct SURFFS_WEB_PAGE *webpage)
{
    switch (type)
    {
        case INODE_FILE_URL:
            return webpage->full_url.textlen;

        case INODE_FILE_STATUS:
            return webpage->status_str.textlen;

        case INODE_FILE_PAGE:
            return (webpage->http_payload) ?
                webpage->http_resp.textlen - (webpage->http_payload - webpage->http_resp.data)
                :0;

        case INODE_FILE_LOG:
            return webpage->log.textlen;

        default: return 0;
    }
}

static struct dentry *surffs_lookup_special_file(struct inode *dir,
                                                 struct dentry *dentry,
                                                 surffs_special_file_desc desc)
{
    int ret = 0;
    struct inode *inode;
    struct super_block *sb = dentry->d_sb;
    struct SURFFS_WEB_PAGE *webpage;

    sfs_enter();
    sfs_info("surffs_lookup_special_file - %s\n", desc.filename);

    webpage = SURFFS_INODE(dir)->webpage;
    ret = surffs_create_inode(sb, dir, SURFFS_FILES_ACCESS_MODE | S_IFREG,
                              iunique(sb, SURFFS_ROOT_INO),
                              desc.type,
                              get_file_size(desc.type, webpage),
                              &inode);
    if (ret) goto out;
    SURFFS_INODE(inode)->webpage = webpage;

    sfs_debug("lookup result: inode ino %ld\n", inode->i_ino);

out:
    sfs_leave();
    return ret ? ERR_PTR(ret) : d_splice_alias(inode, dentry);
}

static struct dentry *surffs_lookup_subdir(struct inode *dir, struct dentry *dentry)
{
    int ret = 0;
    struct inode *inode = 0;
    struct super_block *sb = dentry->d_sb;
    char* webpath = 0;
    sfs_string dentry_path = {0};
    char *discovered_path = 0;

    sfs_enter();
    sfs_info("surffs_lookup_dir - '%s'\n", dentry->d_name.name);

    ret = get_webpath_by_dentry_name(dir, dentry->d_name.name, &webpath);
    if (ret) goto out;
    if (!webpath) goto out;

    ret = sfs_string_createz(&dentry_path, 256);
    if (ret) goto out;

    ret = get_relative_dentry_path(dentry, &dentry_path);
    if (ret) goto out;

    discovered_path = find_discovered_path(sb, webpath);

    if (discovered_path)
    {
        sfs_debug("webpath '%s' was already discovered at dentry '%s' => "
                  "make symlink instead of directrory\n"
                  ,webpath, discovered_path);

        ret = surffs_create_inode(sb, dir, SURFFS_FILES_ACCESS_MODE | S_IFLNK,
                                    iunique(sb, SURFFS_ROOT_INO), INODE_LINK,
                                    0,
                                    &inode);
        if (ret) goto out;

        ret = sfs_string_clear(&SURFFS_INODE(inode)->linkto);
        if (ret) goto out;


        ret = get_relative_path_to_surffs_root(dentry->d_parent,
                                               &SURFFS_INODE(inode)->linkto);
        if (ret) goto out;


        ret = sfs_string_cat(&SURFFS_INODE(inode)->linkto, discovered_path);
        if (ret) goto out;
    }
    else
    {
        ret = surffs_create_inode(sb, dir, SURFFS_FILES_ACCESS_MODE | S_IFDIR,
                                    iunique(sb, SURFFS_ROOT_INO), INODE_DIR,
                                    0,
                                    &inode);
        if (ret) goto out;

        ret = sfs_string_set(&SURFFS_INODE(inode)->webPath, webpath);
        if (ret) goto out;

        ret = add_discovered_path(sb, webpath, dentry_path.data);
        if (ret) goto out;
    }

    sfs_debug("lookup result: inode ino %ld, webpath = '%s'\n",
              inode->i_ino, webpath);


out:
    sfs_string_free(&dentry_path);
    sfs_leave();
    return ret ? ERR_PTR(ret) : d_splice_alias(inode, dentry);
}

static int obtain_inode_webpage(struct inode* inode)
{
    int ret = 0;
    struct SURFFS_WEB_PAGE* webpage;
    struct SURFFS_WEB_ADDRESS webaddr;

    sfs_enter();
    sfs_debug("obtain_inode_webpage, inode ino %ld, webpath = '%s'\n",
              inode->i_ino, SURFFS_INODE(inode)->webPath.data);

    if (!SURFFS_INODE(inode)->webPath.textlen)
    {
        sfs_error("obtain_inode_webpage error: no webpath at dir inode\n");
        ret = -EINVAL;
        goto out;
    }

    webaddr.ip = SURFFS_SB(inode->i_sb)->root_web_address->ip;
    webaddr.host = SURFFS_SB(inode->i_sb)->root_web_address->host;
    webaddr.path = SURFFS_INODE(inode)->webPath;

    ret = get_webpage(webaddr, &webpage);
    if (ret) goto out;

    SURFFS_INODE(inode)->webpage = webpage;
    sfs_debug("set webpage to inode %ld: '%s%s'\n",
              inode->i_ino,
              webpage->address.host.data,
              webpage->address.path.data);

out:
    sfs_leave();
    return ret;
}

struct dentry *surffs_lookup(struct inode *dir, struct dentry *dentry,
                   unsigned int flags)
{
    const surffs_special_file_desc *i;
    int ret = 0;

    if (!SURFFS_INODE(dir)->webpage)
    {
        ret = obtain_inode_webpage(dir);
        if (ret) return ERR_PTR(ret);
    }

    for (i = special_files; i->filename; i++)
        if (strcmp(dentry->d_name.name, i->filename) == 0)
            return surffs_lookup_special_file(dir, dentry, *i);

    return surffs_lookup_subdir(dir, dentry);
}

static int emit_special_files(struct file *file, struct dir_context *ctx, loff_t expected_start_pos)
{
    int ret = 0;
    struct super_block *sb = file->f_path.dentry->d_sb;
    const surffs_special_file_desc *i;
    int n;
    loff_t expected_pos;

    sfs_enter();
    sfs_debug("emit_special_files, expected start pos = %d\n", (int)expected_start_pos);

    if (ctx->pos < expected_start_pos)
    {
        sfs_error("emit_special_files position error\n");
        ret = -EINVAL;
        goto out;
    }

    for (i = special_files, n = 0; i->filename; i++, n++)
    {
        expected_pos = expected_start_pos + (loff_t)n;
        sfs_debug("expected_pos: %d, fact pos: %d\n",
                  (int)expected_pos, (int)ctx->pos);

        if (expected_pos == ctx->pos)
        {
            sfs_debug("emit '%s'\n", i->filename);
            ret = ctx->actor(ctx,
                            i->filename,
                            strlen(i->filename),
                            ctx->pos,
                            iunique(sb, SURFFS_ROOT_INO),
                            DT_REG);
            ctx->pos++;
            if (ret) goto out;
        }
    }

out:
    sfs_leave();
    return ret;
}

static int emit_dirs(struct file *file, struct dir_context *ctx, loff_t expected_start_pos)
{
    int ret = 0;
    struct super_block *sb = file->f_path.dentry->d_sb;
    struct SURFFS_WEB_PAGE *webpage = 0;
    struct list_head *pos;
    struct SURFFS_HTML_LINK* link;
    loff_t expected_pos;
    int i;

    sfs_enter();
    sfs_debug("emit_dirs, expected start pos = %d\n", (int)expected_start_pos);

    if (ctx->pos < expected_start_pos)
    {
        sfs_error("emit_dirs position error\n");
        ret = -EINVAL;
        goto out;
    }

    webpage = SURFFS_INODE(file->f_inode)->webpage;
    if (!webpage) {ret = -EINVAL; goto out;}

    sfs_debug("webpage for dentry '%s': '%s%s'\n",
              file->f_path.dentry->d_name.name,
              webpage->address.host.data,
              webpage->address.path.data);

    i = 0;
    list_for_each(pos, &webpage->html_links)
    {
        expected_pos = expected_start_pos + (loff_t)i;
        sfs_debug("expected_pos: %d, fact pos: %d\n",
                  (int)expected_pos, (int)ctx->pos);

        if (expected_pos == ctx->pos)
        {
            link = list_entry(pos, struct SURFFS_HTML_LINK, html_links);

            sfs_debug("emit '%s'\n", link->title.data);

            ret = ctx->actor(ctx,
                            link->title.data,
                            link->title.textlen,
                            ctx->pos,
                            iunique(sb, SURFFS_ROOT_INO),
                            DT_DIR);

            ctx->pos++;
            if (ret) goto out;
        }
        i++;
    }

out:
    sfs_leave();
    return ret;
}

int surffs_readdir(struct file *file, struct dir_context *ctx)
{
    int ret = 0;

    sfs_enter();
    sfs_info("surffs_readdir: '%s'\n", file->f_path.dentry->d_name.name);

    if (!SURFFS_INODE(file->f_inode)->webpage)
    {
        ret = obtain_inode_webpage(file->f_inode);
        if (ret) goto out;
    }

    sfs_debug("pos before = %d\n", (int)ctx->pos);

    if (!dir_emit_dots(file, ctx)) {ret = -EINVAL; goto out;}

    ret = emit_special_files(file, ctx, 2);
    if (ret) goto out;

    ret = emit_dirs(file, ctx, 2 + SPECIAL_FILES_COUNT);
    if (ret) goto out;

    sfs_debug("pos after = %d\n", (int)ctx->pos);

out:
    sfs_leave();
    return ret;
}

static int define_reading_source(struct SURFFS_WEB_PAGE *webpage,
                                 enum SURFFS_INODE_TYPE filetype,
                                 void **source, size_t *source_len)
{
    if (!webpage) return -EINVAL;

    switch (filetype)
    {
        case INODE_FILE_URL:
            *source = webpage->full_url.data;
            *source_len = webpage->full_url.textlen;
        break;

        case INODE_FILE_LOG:
            *source = webpage->log.data;
            *source_len = webpage->log.textlen;
        break;

        case INODE_FILE_PAGE:
            if (webpage->http_payload)
            {
                *source = webpage->http_payload;
                *source_len = strlen(webpage->http_payload);
            }
            else
            {
                *source = 0;
                *source_len = 0;
            }
        break;

        case INODE_FILE_STATUS:
            *source = webpage->status_str.data;
            *source_len = webpage->status_str.textlen;
        break;

        default:
        sfs_error("reading error: invalid inode type: %d\n", filetype);
        return -EINVAL;
    }

    return 0;
}

ssize_t surffs_aio_read(struct kiocb *iocb, const struct iovec *vec,
                        unsigned long segs, loff_t loff)
{
    int ret = 0;
    void *source;
    size_t source_len;
    size_t requested_pos = (size_t)iocb->ki_pos;
    size_t requested_len = vec->iov_len;
    void *start, *end;
    size_t read_len;
    size_t uncopyed_bytes;
    struct inode* inode = iocb->ki_filp->f_inode;

    sfs_enter();
    sfs_debug("surffs_aio_read, requested_len = %lu, pos before reading = %lu\n",
           (unsigned long)requested_len, (unsigned long)iocb->ki_pos);

    ret = define_reading_source(SURFFS_INODE(inode)->webpage,
                                SURFFS_INODE(inode)->type,
                                &source, &source_len);
    if (ret) goto out;
    if (!source) return 0;

    if (requested_pos > source_len) {ret = -EINVAL; goto out;}

    start = source + requested_pos;
    end = (requested_pos + requested_len) < source_len ?
            start + requested_len : source + source_len;

    read_len = end - start;

    if (read_len)
    {
        uncopyed_bytes = copy_to_user(vec->iov_base, start, read_len);
        read_len -= uncopyed_bytes;
    }

    iocb->ki_pos += (loff_t)read_len;

    sfs_debug("my_aio_read OK, read_len = %lu. iocb->ki_pos after reading = %lu\n",
                (unsigned long)read_len, (unsigned long)iocb->ki_pos);

out:
    sfs_leave();
    return ret ? ret : read_len;
}

void *surffs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
    int ret = 0;
    struct inode *inode;

    sfs_enter();
    sfs_debug("surffs_follow_link for dentry '%s'\n", dentry->d_name.name);

    inode = dentry->d_inode;
    if (!inode)
    {
        sfs_error("surffs_follow_link: no inode\n");
        ret = -EINVAL;
        goto out;
    }

    sfs_debug("link = '%s'\n", SURFFS_INODE(inode)->linkto.data);
    nd_set_link(nd, SURFFS_INODE(inode)->linkto.data);

out:
    sfs_leave();
    return ret ? ERR_PTR(ret) : NULL;
}

