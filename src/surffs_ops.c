#include "surffs_inode.h"
#include "surffs_debug.h"
#include "surffs_inode.h"
#include "surffs_dentry.h"

const struct super_operations surffs_sb_ops = {
    .statfs         = simple_statfs,
    .drop_inode     = surffs_delete_inode,
    .show_options	= generic_show_options
};

const struct file_operations surffs_file_dir_ops = {
    .llseek		= generic_file_llseek,
    .read		= generic_read_dir,
    .iterate	= surffs_readdir,
    .fsync		= noop_fsync,
};

const struct file_operations surffs_file_reg_ops = {
    .llseek		= generic_file_llseek,
    .read		= do_sync_read,
    .write		= do_sync_write,
    .aio_read	= surffs_aio_read,
    .mmap		= generic_file_mmap,
    .fsync		= noop_fsync,
    .splice_read	= generic_file_splice_read,
    .splice_write	= generic_file_splice_write,
};

const struct inode_operations surffs_inode_dir_ops = {
    .lookup		= surffs_lookup,
    .link		= simple_link,
    .unlink		= simple_unlink,
};

const struct inode_operations surffs_inode_reg_ops = {
    .setattr		= simple_setattr,
    .getattr		= simple_getattr
};

const struct dentry_operations surffs_dentry_operations = {
    .d_delete	= surffs_dentry_delete,
    .d_release	= surffs_dentry_release,
};

const struct inode_operations surffs_symlink_inode_operations = {
    .readlink    = generic_readlink,
    .follow_link = surffs_follow_link,
    .setattr     = simple_setattr,
    .getattr     = simple_getattr,
};




