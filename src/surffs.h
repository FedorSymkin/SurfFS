#ifndef _SURFFS_H_
#define _SURFFS_H_

#include <linux/kernel.h>
#include <linux/fs.h>

#define SURFFS_MAGIC_NUMBER 0x0508FF50
#define SURFFS_FILES_ACCESS_MODE 00444
#define SURFFS_ROOT_INO 1
#define SURFFS_HTTP_PORT 80
#define SURFFS_HTTP_CHUNK_SIZE 4096
#define SURFFS_SOCKET_TOUT_SEC  2
#define SURFFS_SOCKET_TOUT_USEC 0
#define SURFFS_VERSION "0.1 beta"

#endif



