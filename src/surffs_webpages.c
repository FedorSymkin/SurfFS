#include <linux/string.h>
#include <linux/list.h>
#include "surffs_webpages.h"
#include "surffs_debug.h"
#include "surffs_internet.h"
#include <linux/slab.h>
#include "surffs_helpers.h"

LIST_HEAD(webpages_list);

static int cmp_web_address(  struct SURFFS_WEB_ADDRESS address1,
                             struct SURFFS_WEB_ADDRESS address2)
{
    if (strcmp(address1.path.data, address2.path.data)) return 0;
    if (strcmp(address1.host.data, address2.host.data)) return 0;
    if (strcmp(address1.ip.data, address2.ip.data)) return 0;
    return 1;
}

static struct SURFFS_WEB_PAGE* find_webpage(struct SURFFS_WEB_ADDRESS address)
{
    struct list_head *pos;
    struct SURFFS_WEB_PAGE* ret = 0;
    struct SURFFS_WEB_PAGE* p;

    sfs_enter();
    sfs_trace("find_webpage\n");

    list_for_each(pos, &webpages_list)
    {
        p = list_entry(pos, struct SURFFS_WEB_PAGE, webpages);
        if (cmp_web_address(p->address, address))
        {
            sfs_trace("found webpage!\n");
            ret = p;
            goto out;
        }
    }

    sfs_trace("no pages found\n");

out:
    sfs_leave();
    return ret;
}

static void add_webpage(struct SURFFS_WEB_PAGE* page)
{
    sfs_enter();
    sfs_debug("add_webpage");
    list_add(&page->webpages, &webpages_list);
    sfs_leave();
}



int get_webpage(struct SURFFS_WEB_ADDRESS address, struct SURFFS_WEB_PAGE **page)
{
    struct SURFFS_WEB_PAGE *p = 0;
    int ret = 0;

    sfs_enter();
    sfs_debug("get_webpage: %s%s (%s)\n", address.host.data, address.path.data, address.ip.data);

    p = find_webpage(address);
    if (p)
    {
        *page = p;
        goto out;
    }
    else
    {
        ret = SURFFS_WEB_PAGE_alloc(&p); if (ret) goto out;
        ret = obtain_webpage(address, p); if (ret) goto out;
        add_webpage(p);
        *page = p;
        goto out;
    }

out:
    if (ret && p) kfree(p);
    sfs_leave();
    return ret;
}

int SURFFS_HTML_LINK_alloc(struct SURFFS_HTML_LINK **link)
{
    int ret = 0;
    struct SURFFS_HTML_LINK *newlink = 0;

    sfs_enter();
    sfs_trace("SURFFS_HTML_LINK_alloc\n");

    newlink = kzalloc(sizeof(struct SURFFS_HTML_LINK), GFP_KERNEL);
    if (!newlink) {ret = -ENOMEM; goto out;}

    ret = sfs_string_createz(&newlink->title, 64); if (ret) goto out;
    ret = sfs_string_createz(&newlink->protocol, 16); if (ret) goto out;
    ret = sfs_string_createz(&newlink->host, 64); if (ret) goto out;
    ret = sfs_string_createz(&newlink->path, 64); if (ret) goto out;
    ret = sfs_string_createz(&newlink->full_url, 64); if (ret) goto out;

    *link = newlink;

out:
    sfs_leave();
    return ret;
}

void SURFFS_HTML_LINK_free(struct SURFFS_HTML_LINK *link)
{
    sfs_enter();
    sfs_trace("SURFFS_HTML_LINK_free: '%s'\n", link->full_url.data);

    if (!link) goto out;
    sfs_string_free(&link->title);
    sfs_string_free(&link->protocol);
    sfs_string_free(&link->host);
    sfs_string_free(&link->path);
    sfs_string_free(&link->full_url);
    kfree(link);

out:
    sfs_leave();
}

int SURFFS_HTML_LINK_print(struct SURFFS_HTML_LINK *link, sfs_string *str)
{
    int ret = 0;

    sfs_enter();
    sfs_trace("SURFFS_HTML_LINK_print\n");

    ret = sfs_string_cat(str, "SURFFS_HTML_LINK: ");
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "title = '%s' ", link->title.data);
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "full_url = '%s' ", link->full_url.data);
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "protocol = '%s' ", link->protocol.data);
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "host = '%s' ", link->host.data);
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "path = '%s' ", link->path.data);
    if (ret) goto out;

out:
    sfs_leave();
    return ret;
}

int SURFFS_WEB_ADDRESS_alloc(struct SURFFS_WEB_ADDRESS **addr)
{
    int ret = 0;
    struct SURFFS_WEB_ADDRESS *newaddr = 0;

    sfs_enter();
    sfs_trace("SURFFS_WEB_ADDRESS_alloc\n");

    newaddr = kzalloc(sizeof(struct SURFFS_WEB_ADDRESS), GFP_KERNEL);
    if (!newaddr) {ret = -ENOMEM; goto out;}

    ret = sfs_string_createz(&newaddr->ip, 16); if (ret) goto out;
    ret = sfs_string_createz(&newaddr->host, 64); if (ret) goto out;
    ret = sfs_string_createz(&newaddr->path, 64); if (ret) goto out;

    *addr = newaddr;

out:
    sfs_leave();
    return ret;
}

void SURFFS_WEB_ADDRESS_free(struct SURFFS_WEB_ADDRESS *addr)
{
    sfs_enter();
    sfs_trace("SURFFS_WEB_ADDRESS_free\n");

    if (!addr) goto out;
    sfs_string_free(&addr->ip);
    sfs_string_free(&addr->host);
    sfs_string_free(&addr->path);
    kfree(addr);

out:
    sfs_leave();
}

int SURFFS_WEB_ADDRESS_print(struct SURFFS_WEB_ADDRESS *addr, sfs_string *str)
{
    int ret = 0;

    sfs_enter();
    sfs_trace("SURFFS_WEB_ADDRESS_print\n");

    ret = sfs_string_cat(str, "SURFFS_WEB_ADDRESS: ");
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "ip = '%s' ", addr->ip.data);
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "host = '%s' ", addr->host.data);
    if (ret) goto out;

    ret = sfs_string_cat_param(str, "path = '%s' ", addr->path.data);
    if (ret) goto out;

out:
    sfs_leave();
    return ret;
}

int SURFFS_WEB_PAGE_alloc(struct SURFFS_WEB_PAGE **page)
{
    int ret = 0;
    struct SURFFS_WEB_PAGE *p;

    sfs_enter();
    sfs_trace("SURFFS_WEB_PAGE_alloc\n");

    p = kzalloc(sizeof(struct SURFFS_WEB_PAGE), GFP_KERNEL);
    if (!p) {ret = -ENOMEM; goto out;}

    p->status = STATUS_NEED_GET;
    ret = sfs_string_createz(&p->address.ip, 16); if (ret) goto out;
    ret = sfs_string_createz(&p->address.host, 64); if (ret) goto out;
    ret = sfs_string_createz(&p->address.path, 64); if (ret) goto out;
    ret = sfs_string_createz(&p->http_resp, 4096); if (ret) goto out;
    ret = sfs_string_createz(&p->log, 512); if (ret) goto out;
    ret = sfs_string_createz(&p->full_url, 64); if (ret) goto out;
    ret = sfs_string_createz(&p->status_str, 16); if (ret) goto out;

    INIT_LIST_HEAD(&p->html_links);

    ret = sfs_string_set(&p->status_str, STATUS_NEED_GET_STR);

    *page = p;
out:
    sfs_leave();
    return ret;
}

void SURFFS_WEB_PAGE_free(struct SURFFS_WEB_PAGE *p)
{  
    struct list_head *pos;
    struct list_head *tmp;
    struct SURFFS_HTML_LINK* link;

    sfs_enter();
    sfs_trace("SURFFS_WEB_PAGE_free: '%s'\n", p->full_url.data);

    list_for_each_safe(pos, tmp, &p->html_links)
    {
        link = list_entry(pos, struct SURFFS_HTML_LINK, html_links);
        list_del(pos);
        SURFFS_HTML_LINK_free(link);
    }

    sfs_string_free(&p->address.ip);
    sfs_string_free(&p->address.host);
    sfs_string_free(&p->address.path);
    sfs_string_free(&p->http_resp);
    sfs_string_free(&p->log);
    sfs_string_free(&p->full_url);
    sfs_string_free(&p->status_str);
    kfree(p);

    sfs_leave();
}

void free_webpages(void)
{
    struct list_head *pos;
    struct list_head *tmp;
    struct SURFFS_WEB_PAGE* page;

    sfs_enter();
    sfs_info("free_webpages\n");



    list_for_each_safe(pos, tmp, &webpages_list)
    {
        page = list_entry(pos, struct SURFFS_WEB_PAGE, webpages);
        list_del(pos);
        SURFFS_WEB_PAGE_free(page);
    }

    sfs_leave();
}
