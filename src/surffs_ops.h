#ifndef _SURFFS_OPS_H_
#define _SURFFS_OPS_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/dcache.h>

extern const struct super_operations surffs_sb_ops;
extern const struct file_operations surffs_file_dir_ops;
extern const struct file_operations surffs_file_reg_ops;
extern const struct inode_operations surffs_inode_dir_ops;
extern const struct inode_operations surffs_inode_reg_ops;
extern const struct dentry_operations surffs_dentry_operations;
extern const struct inode_operations surffs_symlink_inode_operations;

#endif
