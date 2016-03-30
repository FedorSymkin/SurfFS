#ifndef _SURFFS_SB_H_
#define _SURFFS_SB_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/hashtable.h>
#include "surffs.h"
#include "surffs_helpers.h"
#include "surffs_webpages.h"
#include "surffs_helpers.h"




struct surffs_sb_info
{
    struct SURFFS_WEB_ADDRESS *root_web_address;
    char *raw_mount_data;

    /*
     * hash of discovred resources: key is webpath, value is linux path
     * (relative from mount root).
     * It used for creating symlinks for already discovered pages
     */
    DECLARE_HASHTABLE(discovred_paths, 12);
};

char *find_discovered_path(struct super_block *sb, char *webpath);
int add_discovered_path(struct super_block *sb, char *webpath, char *linux_path);

inline struct dentry *surffs_mount(struct file_system_type *type, int flags,
                                     char const *url, void *data);
inline void surffs_unmount(struct super_block *sb);

inline struct surffs_sb_info *SURFFS_SB(struct super_block *sb);

#endif



