#ifndef _SURFFS_INTERNET_H_
#define _SURFFS_INTERNET_H_

#include <linux/kernel.h>
#include "surffs_webpages.h"


int is_valid_protocol(char *protocol);
int obtain_webpage(struct SURFFS_WEB_ADDRESS address, struct SURFFS_WEB_PAGE *page);
#endif
