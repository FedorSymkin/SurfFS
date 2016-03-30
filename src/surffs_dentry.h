#ifndef _SURFFS_DENTRY_H_
#define _SURFFS_DENTRY_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include "surffs_helpers.h"

int surffs_dentry_delete(const struct dentry *dentry);
void surffs_dentry_release(struct dentry *dentry);
int get_relative_dentry_path(struct dentry *dentry, sfs_string *path);
int get_relative_path_to_surffs_root(struct dentry *dentry, sfs_string *path);

#endif
