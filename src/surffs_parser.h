#ifndef _SURFFS_PARSER_H_
#define _SURFFS_PARSER_H_

#include "surffs_helpers.h"
#include "surffs_webpages.h"

/*Maybe more correct way to implement these functions is to find out
 * how to compile module with linux kernel regexp (linux/kernel/trace/trace.h)*/

/*
 * this enum defines behaviour of parsing urls like "resource.xxx"
 * where "resource.xxx" may be both host and path
 * */
enum EXTRACT_PARAMS_POLICY
{
    PREFER_PATH = 0,
    PREFER_HOST
};

int extract_html_block(const char **textpos, const char *html_tag, sfs_string *block);

int extract_html_link_params(const char *text, struct SURFFS_HTML_LINK *link);

int extract_url_params(const char *url,
                       sfs_string *protocol, sfs_string *host, sfs_string *path,
                       enum EXTRACT_PARAMS_POLICY extract_policy);

#endif
