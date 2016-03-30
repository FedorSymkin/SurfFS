#include "surffs_internet.h"
#include <linux/string.h>
#include "surffs_debug.h"
#include <linux/slab.h>
#include <linux/netpoll.h>
#include "surffs_socket.h"
#include "surffs_helpers.h"
#include <linux/list.h>
#include "surffs_parser.h"
#include "surffs.h"

char* supported_protocols[] = {"http", 0};

int is_valid_protocol(char *protocol)
{
    char **i;

    if (!protocol[0])
        return 1;

    for (i = supported_protocols; *i; i++)
        if (strcmp(protocol, *i) == 0)
            return 1;

    return 0;
}

int check_html_link(struct SURFFS_HTML_LINK *link,
                    struct SURFFS_WEB_ADDRESS parent_addr,
                    sfs_string *log,
                    int *ok)
{
    int ret = 0;

    sfs_enter();
    sfs_debug("check_html_link\n");

    *ok = 0;

    if (!link->title.textlen)
    {
        sfs_debug("error check link '%s': empty title\n",
                  link->full_url.data);

        ret = sfs_string_cat_param(log, "error check link '%s': empty title\n",
                  link->full_url.data);
        goto out;
    }

    if (!link->full_url.textlen)
    {
        sfs_debug("error check link '%s': empty url\n",
                  link->full_url.data);

        ret = sfs_string_cat_param(log, "error check link '%s': empty url\n",
                  link->full_url.data);
        goto out;
    }

    if (!is_valid_protocol(link->protocol.data))
    {
        sfs_debug("error check link '%s': protocol '%s' not supported\n",
                  link->full_url.data, link->protocol.data);

        ret = sfs_string_cat_param(log, "error check link '%s': ",
                  link->full_url.data);
        if (ret) goto out;


        ret = sfs_string_cat_param(log, "protocol '%s' not supported\n",
                  link->protocol.data);
        goto out;
    }


    if (link->host.textlen && (strcmp(link->host.data, parent_addr.host.data) != 0))
    {
        sfs_debug("error check link '%s': links to another host not supported (parent host = '%s')\n",
                  link->full_url.data, parent_addr.host.data);

        ret = sfs_string_cat_param(log,
           "error check link '%s': links to another host not supported ", link->full_url.data);
        if (ret) goto out;

        ret = sfs_string_cat_param(log,
            " (parent host = '%s')\n", parent_addr.host.data);
        goto out;
    }

    if (strcmp(link->path.data, parent_addr.path.data) == 0)
    {
        sfs_debug("error check link '%s': link to same path (parent path = '%s')\n",
                  link->full_url.data, parent_addr.path.data);

        ret = sfs_string_cat_param(log,
           "error check link '%s': link to same path ", link->full_url.data);
        if (ret) goto out;

        ret = sfs_string_cat_param(log,
            " (parent path = '%s')\n", parent_addr.path.data);
        goto out;
    }

    *ok = 1;

out:
    sfs_leave();
    return ret;
}

int add_number_to_title(sfs_string *title, int num)
{
    int ret = 0;
    char tmpbuf[256];

    sfs_enter();
    sfs_debug("add_number_to_title: %d\n", num)

    snprintf(tmpbuf, sizeof(tmpbuf), "_link%d", num);

    ret = sfs_string_cat(title, tmpbuf);
    if (ret) goto out;


out:
    sfs_leave();
    return ret;
}

int make_html_links(char *html,
                    struct list_head *links_list,
                    struct SURFFS_WEB_ADDRESS parent_addr,
                    sfs_string *log)
{
    int ret = 0;
    int ok;
    int n = 1;
    const char *textpos = html;
    sfs_string a_block = {0};
    struct SURFFS_HTML_LINK *link;

    sfs_enter();
    sfs_debug("make_html_links\n");

    ret = sfs_string_createz(&a_block, 256); if (ret) goto out;

    while (1)
    {
        ret = extract_html_block(&textpos, "a", &a_block);
        if (ret) goto out;
        if (!a_block.textlen) break;

        ret = SURFFS_HTML_LINK_alloc(&link);
        if (ret) goto out;

        ret = extract_html_link_params(a_block.data, link);
        if (ret) goto out;

        ret = check_html_link(link, parent_addr, log, &ok);
        if (ret) goto out;

        if (ok)
        {
            ret = add_number_to_title(&link->title, n);
            if (ret) goto out;

            sfs_debug("add html link: title = '%s' url = '%s'\n",
                      link->title.data, link->full_url.data);

            list_add(&link->html_links, links_list);


            ret = sfs_string_cat_param(log, "add html link: title = '%s' ",
                                 link->title.data);
            if (ret) goto out;

            ret = sfs_string_cat_param(log, "url = '%s'\n",
                                 link->full_url.data);
            if (ret) goto out;

            n++;
        }
        else
        {
            sfs_debug("skip html link: title = '%s' url = '%s'\n",
                      link->title.data, link->full_url.data);
            SURFFS_HTML_LINK_free(link);
        }
    }

    sfs_debug("make_html_links OK\n");

out:
    sfs_string_free(&a_block);
    sfs_leave();
    return ret;
}

int obtain_webpage(struct SURFFS_WEB_ADDRESS address, struct SURFFS_WEB_PAGE *page)
{
    int ret = 0;

    sfs_enter();
    sfs_debug("obtain_webpage\n");

    if (page->status != STATUS_NEED_GET)
    {
        sfs_error("error obtain page: page is already obtained\n");
        ret = -EINVAL;
        goto out;
    }

    ret = sfs_string_set(&page->address.ip, address.ip.data); if (ret) goto out;
    ret = sfs_string_set(&page->address.host, address.host.data); if (ret) goto out;
    ret = sfs_string_set(&page->address.path, address.path.data); if (ret) goto out;
    ret = sfs_string_clear(&page->full_url); if (ret) goto out;
    ret = sfs_string_cat(&page->full_url, page->address.host.data); if (ret) goto out;
    ret = sfs_string_cat(&page->full_url, page->address.path.data); if (ret) goto out;

    ret = surffs_get_http(  address.ip.data,
                            address.host.data,
                            address.path.data,
                            &page->http_resp,
                            &page->http_payload,
                            &page->log);
    if (ret) goto out;

    if (!page->http_payload)
    {
        page->status = STATUS_HTTP_ERROR;
        ret = sfs_string_set(&page->status_str, STATUS_HTTP_ERROR_STR);
        goto out;
    }

    ret = make_html_links(page->http_payload, &page->html_links, address, &page->log);
    if (ret) goto out;


    page->status = STATUS_OK;
    ret = sfs_string_set(&page->status_str, STATUS_OK_STR);
    if (ret) goto out;

out:
    sfs_leave();
    return ret;
}

