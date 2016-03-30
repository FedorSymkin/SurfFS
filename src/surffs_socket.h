#ifndef _SURFFS_SOCKET_H_
#define _SURFFS_SOCKET_H_

#include <linux/kernel.h>
#include "surffs_helpers.h"

int surffs_get_http(char *ip, char *host, char *path,
                    sfs_string *http_response,
                    char** http_payload_start,
                    sfs_string *log);


#endif
