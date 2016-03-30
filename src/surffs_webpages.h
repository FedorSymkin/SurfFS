#ifndef _SURFFS_WEBPAGES_H_
#define _SURFFS_WEBPAGES_H_

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include "surffs_helpers.h"

enum SURFFS_WEB_STATUS
{
    STATUS_OK = 0,
    STATUS_HTTP_ERROR,
    STATUS_NEED_GET
};
#define STATUS_OK_STR           "ok"
#define STATUS_HTTP_ERROR_STR   "error"
#define STATUS_NEED_GET_STR     "unknown"


struct SURFFS_HTML_LINK
{
    struct list_head html_links;

    sfs_string title;
    sfs_string protocol;
    sfs_string host;
    sfs_string path;

    sfs_string full_url;
};
int  SURFFS_HTML_LINK_alloc(struct SURFFS_HTML_LINK **link);
void SURFFS_HTML_LINK_free(struct SURFFS_HTML_LINK *link);
int  SURFFS_HTML_LINK_print(struct SURFFS_HTML_LINK *link, sfs_string *str);


struct SURFFS_WEB_ADDRESS
{
    sfs_string ip;
    sfs_string host;
    sfs_string path;
};
int  SURFFS_WEB_ADDRESS_alloc(struct SURFFS_WEB_ADDRESS **addr);
void SURFFS_WEB_ADDRESS_free(struct SURFFS_WEB_ADDRESS *addr);
int  SURFFS_WEB_ADDRESS_print(struct SURFFS_WEB_ADDRESS *addr, sfs_string *str);

struct SURFFS_WEB_PAGE
{
    struct list_head webpages;

    struct SURFFS_WEB_ADDRESS address;
    enum SURFFS_WEB_STATUS status;

    sfs_string http_resp;
    char *http_payload;
    struct list_head html_links;

    sfs_string log;
    sfs_string full_url;
    sfs_string status_str;
};
int  SURFFS_WEB_PAGE_alloc(struct SURFFS_WEB_PAGE **p);
void SURFFS_WEB_PAGE_free(struct SURFFS_WEB_PAGE *p);


int get_webpage(struct SURFFS_WEB_ADDRESS address, struct SURFFS_WEB_PAGE **page);
void free_webpages(void);

#endif
