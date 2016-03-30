#include "surffs_sb.h"
#include "surffs_debug.h"
#include "surffs_inode.h"
#include "surffs_ops.h"
#include <linux/parser.h>
#include "surffs_parser.h"
#include "surffs_internet.h"
#include "surffs_dentry.h"

struct string_hash_node
{
    struct hlist_node hashlist;
    sfs_string key;
    sfs_string value;
};


inline struct surffs_sb_info *SURFFS_SB(struct super_block *sb)
{
    return sb->s_fs_info;
}


enum {
    Opt_ip,
    Opt_err
};

static const match_table_t tokens = {
    {Opt_ip, "ip=%s"},
    {Opt_err, NULL}
};

static int surffs_parse_mount_options(char *data, struct surffs_sb_info *fsi)
{
    int ret = 0;
    substring_t args[MAX_OPT_ARGS];
    int token;
    char *p;
    char *tmp = 0;

    sfs_enter();
    sfs_debug("surffs_parse_mount_options, data = %s\n", data);

    while ((p = strsep(&data, ",")) != NULL) {
        if (!*p)
            continue;

        sfs_trace("parsing...\n");

        token = match_token(p, tokens, args);

        sfs_trace("token = %d\n", token);

        switch (token) {
        case Opt_ip:
            tmp = match_strdup(&args[0]);
            if (!tmp)
            {
                sfs_debug("error match_strdup\n");
                ret = -EINVAL;
                goto out;
            }
            sfs_string_cat(&fsi->root_web_address->ip, tmp);
            kfree(tmp);
            break;
        }
    }

    sfs_debug("ok. ip = %s\n", fsi->root_web_address->ip.data);

out:
    sfs_leave();
    return ret;
}

static void free_discovred_paths(struct surffs_sb_info *fsi)
{
    int bkt;
    struct string_hash_node *node;
    struct hlist_node *tmp;

    sfs_debug("free_discovred_paths\n");

    hash_for_each_safe(fsi->discovred_paths, bkt, tmp, node, hashlist)
    {
        sfs_string_free(&node->key);
        sfs_string_free(&node->value);
        hash_del(&node->hashlist);
        kfree(node);
    }
}

static void surffs_free_super_private(struct surffs_sb_info *fsi)
{
    sfs_enter();
    sfs_debug("surffs_free_super_private\n");

    if (fsi->raw_mount_data)
        kfree(fsi->raw_mount_data);

    if (fsi->root_web_address)
        SURFFS_WEB_ADDRESS_free(fsi->root_web_address);

    free_discovred_paths(fsi);

    sfs_leave();
}

static int surffs_fill_super(struct super_block *sb, void *data, const char *mountdata, int silent)
{
    int ret = 0;
    struct surffs_sb_info *fsi = 0;
    struct inode *root_inode = 0;
    sfs_string protocol = {0}; // dont forget free!

    sfs_enter();
    sfs_debug("surffs_fill_super\n");

    save_mount_options(sb, data);

    sb->s_d_op = &surffs_dentry_operations;

    fsi = kzalloc(sizeof(struct surffs_sb_info), GFP_KERNEL);
    sb->s_fs_info = fsi;
    if (!fsi) {ret = -ENOMEM; goto out;}

    fsi->raw_mount_data = kstrdup(mountdata, GFP_KERNEL);
    if (!fsi->raw_mount_data) {ret = -ENOMEM; goto out;}

    ret = SURFFS_WEB_ADDRESS_alloc(&fsi->root_web_address);
    if (ret) goto out;

    hash_init(fsi->discovred_paths);

    ret = sfs_string_createz(&protocol, 16);
    if (ret) goto out;

    ret = extract_url_params(mountdata,
                                     &protocol, //TODO: check for https
                                     &fsi->root_web_address->host,
                                     &fsi->root_web_address->path,
                                     PREFER_HOST);
    if (ret) goto out;
    if (!is_valid_protocol(protocol.data))
    {
        sfs_error("error mount surffs for '%s': protocol '%s' not supported\n",
                  mountdata, protocol.data);
        ret = -EINVAL;
        goto out;
    }

    ret = surffs_parse_mount_options(data, fsi);
    if (ret) goto out;

    if (!fsi->root_web_address->ip.textlen)
    {
        sfs_error("error mount surffs: no ip specified. To mount surffs \"-o ip=...\" is required\n");
        ret = -EINVAL;
        goto out;
    }

    //sb->s_maxbytes		= MAX_LFS_FILESIZE;
    //sb->s_blocksize		= PAGE_CACHE_SIZE;
    //sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
    sb->s_magic           = SURFFS_MAGIC_NUMBER;
    sb->s_op              = &surffs_sb_ops;
    sb->s_time_gran		= 1;

    ret = surffs_create_inode(sb, NULL,
                              S_IFDIR | SURFFS_FILES_ACCESS_MODE,
                              SURFFS_ROOT_INO,
                              INODE_DIR,
                              0,
                              &root_inode);
    if (ret) goto out;

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {ret = -ENOMEM; goto out;}

    ret = sfs_string_set(&SURFFS_INODE(root_inode)->webPath,
                         fsi->root_web_address->path.data);
    if (ret) goto out;

    ret = add_discovered_path(sb, SURFFS_INODE(root_inode)->webPath.data, "/");
    if (ret) goto out;

out:
    sfs_string_free(&protocol);
    if (ret)
    {
        if (root_inode) iput(root_inode);
        if (fsi)
        {
            surffs_free_super_private(fsi);
            kfree(fsi);
        }
    }
    sfs_leave();
    return ret;
}



static int surffs_test_super(struct super_block *sb, void *data)
{
    struct surffs_sb_info *fsi = SURFFS_SB(sb);
    if (!fsi)
      return 0;

    if (!fsi->raw_mount_data)
      return 0;

    return (strcmp(fsi->raw_mount_data, (char*)data) != 0);
}

struct dentry *surffs_mount(struct file_system_type *fs_type,
    int flags, const char *url, void *data)
{
    int ret = 0;
    struct super_block *s = 0;

    sfs_enter();
    sfs_info("surffs_mount\n");

    s = sget(fs_type, surffs_test_super, set_anon_super, flags, (void*)url);
    if (IS_ERR(s)) {ret = PTR_ERR(s); goto out;}

    if (!s->s_fs_info)
    {
        sfs_info("New superblok was created\n");

        ret = surffs_fill_super(s, data, url, flags & MS_SILENT ? 1 : 0);
        if (ret) goto out;

        s->s_flags |= MS_ACTIVE;
    }
    else
    {
        sfs_info("Existing superblok used\n");
    }

out:
    if (ret)
    {
        if (s && (!IS_ERR(s))) deactivate_locked_super(s);
    }
    sfs_leave();
    return ret ? ERR_PTR(ret) : dget(s->s_root);
}

void surffs_unmount(struct super_block *sb)
{
    struct surffs_sb_info *fsi;

    sfs_enter();
    sfs_info("surffs_unmount\n");

    fsi = SURFFS_SB(sb);
    if (fsi)
    {
        surffs_free_super_private(fsi);
        kfree(fsi);
    }

    sfs_debug("kill_anon_super...\n");
    kill_anon_super(sb);

    sfs_debug("surffs_unmount OK\n");
    sfs_leave();
}



char *find_discovered_path(struct super_block *sb, char *webpath)
{
    struct string_hash_node *node = 0;
    unsigned int hash;
    char *result = 0;

    sfs_enter();
    sfs_debug("find_discovered_path for webpath '%s'\n", webpath);

    hash = full_name_hash(webpath, strlen(webpath));
    hash_for_each_possible(SURFFS_SB(sb)->discovred_paths, node, hashlist, hash)
    {
        if (strcmp(node->key.data, webpath) == 0)
        {
            sfs_debug("found: '%s'\n", node->value.data);
            result = node->value.data;
            goto out;
        }
    }

    sfs_debug("no entry found\n");

out:
    sfs_leave();
    return result;
}

int add_discovered_path(struct super_block *sb, char *webpath, char *linux_path)
{
    int ret = 0;
    unsigned int hash;
    struct string_hash_node *node = 0;

    sfs_enter();
    sfs_debug("add_discovered_path: webpath = '%s' linux_path = '%s'\n",
              webpath, linux_path);

    node = kzalloc(sizeof(struct string_hash_node), GFP_KERNEL);
    if (!node) {ret = -ENOMEM; goto out;}

    ret = sfs_string_create(&node->key, webpath);
    if (ret) goto out;

    ret = sfs_string_create(&node->value, linux_path);
    if (ret) goto out;

    hash = full_name_hash(webpath, strlen(webpath));

    hash_add(SURFFS_SB(sb)->discovred_paths, &node->hashlist, hash);

out:
    sfs_leave();
    return ret;
}

