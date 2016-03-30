#ifndef _SURFFS_INODE_H_
#define _SURFFS_INODE_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include "surffs_webpages.h"
#include "surffs_helpers.h"

enum SURFFS_INODE_TYPE
{
    INODE_UNKNOWN = 0,
    INODE_FILE_URL,
    INODE_FILE_STATUS,
    INODE_FILE_PAGE,
    INODE_FILE_LOG,
    INODE_LINK,
    INODE_DIR
};

struct SURFFS_INODE_PRIVATE
{
    sfs_string webPath;
    struct SURFFS_WEB_PAGE *webpage;
    enum SURFFS_INODE_TYPE type;
    sfs_string linkto;
};

int surffs_create_inode(struct super_block* sb, struct inode *dir,
                                  umode_t mode, unsigned long ino,
                                  enum SURFFS_INODE_TYPE type,
                                  int file_size,
                                  struct inode **output_inode);

int surffs_delete_inode(struct inode *inode);

struct dentry *surffs_lookup(struct inode *dir, struct dentry *dentry,
                   unsigned int flags);

int surffs_readdir(struct file *file, struct dir_context *ctx);

ssize_t surffs_aio_read(struct kiocb *iocb, const struct iovec *vec, unsigned long segs, loff_t loff);

void *surffs_follow_link(struct dentry *dentry, struct nameidata *nd);

inline struct SURFFS_INODE_PRIVATE* SURFFS_INODE(struct inode *inode);

#endif
