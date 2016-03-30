#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "surffs_sb.h"
#include "surffs_debug.h"

static struct file_system_type surf_fs_type = {
    .owner		= THIS_MODULE,
    .name		= "surffs",
    .mount		= surffs_mount,
    .kill_sb	= surffs_unmount,
    .fs_flags	= 0
};

static int __init surffs_init(void)
{
    int ret = 0;

    printk(KERN_INFO"surffs version %s\n", SURFFS_VERSION);

    sfs_enter();
    sfs_info("surffs_init=======================\n");

    ret = register_filesystem(&surf_fs_type);
    if (ret)
    {
        sfs_error("error register surffs filesystem, error code %d\n", ret);
        goto out;
    }

out:
    sfs_leave();
    return 0;
}

static void __exit surffs_exit(void)
{
    int ret = 0;

    sfs_enter();
    sfs_info("surffs_exit\n");

    free_webpages();

    ret = unregister_filesystem(&surf_fs_type);
    if (ret)
    {
        sfs_error("error unregister surffs filesystem, error code %d\n", ret);
        goto out;
    }

out:
    sfs_leave();
}

module_init(surffs_init);
module_exit(surffs_exit);
MODULE_LICENSE("GPL");

