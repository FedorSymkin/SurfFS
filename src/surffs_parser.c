#include "surffs_parser.h"
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/list.h>
#include "surffs_debug.h"
#include "surffs_helpers.h"

static inline int is_space(char c)
{
    return  (c == ' ') ||
            (c == '\n') ||
            (c == '\r') ||
            (c == '\t');
}

static int is_anyof(char c, const char *chars)
{
    const char *i;

    for (i = chars; *i; i++)
        if (c == *i)
            return 1;

    return 0;
}

static char *find_anyof(const char *str, const char *chars)
{
    for (; *str; str++)
        if (is_anyof(*str, chars))
            return (char*)str;

    return 0;
}

static void find_spaced_patterns(const char *str,
                                 const char **patterns, int patterns_cnt,
                                 char **found_start, char **found_end)
{
    int i;
    int pattern_len;
    const char* pos = str;
    const char* start = 0;
    const char* end = 0;
    *found_start = 0;
    *found_end = 0;

    sfs_enter();
    if (patterns_cnt < 2) goto out;

    while (!start || !end)
    {
        start = strstr(pos, patterns[0]);
        if (!start) goto out;
        pos = start + strlen(patterns[0]);

        for (i = 1; i < patterns_cnt; i++)
        {
            pattern_len = strlen(patterns[i]);

            for (; (*pos) && is_space(*pos); pos++) {}
            if (strncmp(pos, patterns[i], pattern_len) != 0) break;
            pos += pattern_len;
            if (i + 1 == patterns_cnt) end = pos;
        }
    }

    *found_start = (char*)start;
    *found_end = (char*)end;

out:
    sfs_leave();
}



static void trim(sfs_string *str, const char *chars)
{
    char *c;
    size_t i;
    char *start = str->data;
    char *end = str->data + str->textlen;

    if (!str->textlen) return;

    for (; (*start) && is_anyof(*start, chars); start++);
    for (; (end > start) && (*end-1) && is_anyof(*(end-1), chars); end--);


    if (end < start) return;

    if (end == start)
    {
        str->data[0] = 0;
        str->textlen = 0;
        return;
    }

    for (c = start, i = 0; c < end; c++, i++)
        str->data[i] = *c;
    str->data[i] = 0;

    str->textlen = end - start;
}



int extract_url_params(const char *url,
                       sfs_string *protocol, sfs_string *host, sfs_string *path,
                       enum EXTRACT_PARAMS_POLICY extract_policy)
{
    int ret = 0;
    char *found;
    const char *pos = url;

    sfs_enter();
    sfs_trace("extract_url_params, url = '%s'\n", url);

    ret = sfs_string_clear(protocol); if (ret) goto out;
    ret = sfs_string_clear(host); if (ret) goto out;
    ret = sfs_string_clear(path); if (ret) goto out;

    found = strstr(url, "://");
    if (found)
    {
        ret = sfs_string_ncat(protocol, pos, found - pos);
        if (ret) goto out;
        pos = found + 3;
    }

    found = strchr(pos, '/');
    if (found)
    {
        ret = sfs_string_ncat(host, pos, found - pos); if (ret) goto out;
        ret = sfs_string_cat(path, found); if (ret) goto out;
    }
    else
    {
        if (extract_policy == PREFER_HOST)
        {
            ret = sfs_string_cat(host, pos); if (ret) goto out;
            ret = sfs_string_cat(path, "/"); if (ret) goto out;
        }
        else
        {
            ret = sfs_string_cat(path, pos); if (ret) goto out;
        }
    }

    if (path->textlen && (path->data[0] != '/'))
    {
        ret = sfs_string_insert_begin(path, "/");
        if (ret) goto out;
    }

    sfs_trace("extract_url_params result: protocol = '%s' host = '%s' path = '%s'\n",
              protocol->data, host->data, path->data);

out:
    sfs_leave();
    return ret;
}

static int extract_link_href(const char *text, sfs_string *href)
{
    int ret = 0;
    const char *p;
    const char *href_patterns[] = {"href", "="};
    char terminators[3] = {'>',0,0};
    char *start = 0;
    char *end = 0;

    sfs_enter();
    sfs_trace("extract_link_href from text = '%s'\n", text);

    ret = sfs_string_clear(href);
    if (ret) goto out;

    find_spaced_patterns(text, href_patterns, 2, &start, &end);
    if (!start) goto out;

    for (p = end; *p; p++)
    {
        if (*p == '>') goto out;
        if (*p == '\'') {terminators[1] = *p; break;}
        if (*p == '\"') {terminators[1] = *p; break;}
        if (is_space(*p)) continue;
        terminators[1] = ' ';
        break;
    }
    if (!*p) goto out;

    start = (char*)p;
    end = find_anyof(start + 1, terminators);
    if (!end) goto out;

    ret = sfs_string_ncat(href, start, end - start);
    if (ret) goto out;

    trim(href, " \"\'");

    sfs_trace("extract_link_href result: '%s'\n", href->data);

out:
    sfs_leave();
    return ret;
}

static inline int is_illegal_title_char(char c)
{
    return (c < 0x20) || (c == '/');
}

/*modifies title string: removes all data in < > brackets
(including brackets), replaces all illegal chars to ' ', and trim spaces*/
static int normalize_title(sfs_string *title)
{
    int ret = 0;
    char do_delete = 0;
    size_t i,k;
    char *tmpbuf = 0;

    sfs_enter();
    sfs_trace("normalize_title: '%s'\n", title->data);

    if (!title->textlen) goto out;

    tmpbuf = kmalloc(title->textlen + 1, GFP_KERNEL);
    if (!tmpbuf) {ret = -ENOMEM; goto out;}

    for (i = 0; i < title->textlen; i++)
    {
        if (title->data[i] == '<') do_delete = 1;

        if (do_delete)
            tmpbuf[i] = '\b';
        else if (is_illegal_title_char(title->data[i]))
            tmpbuf[i] = ' ';
        else
            tmpbuf[i] = title->data[i];

        if (title->data[i] == '>') do_delete = 0;
    }
    tmpbuf[i] = 0;

    for (i = 0, k = 0; i < title->textlen; i++)
    {
        if (tmpbuf[i] != '\b')
            title->data[k++] = tmpbuf[i];
    }
    title->data[k] = 0;
    title->textlen = k;

    trim(title, " ");

    sfs_trace("normalize_title result: '%s'\n", title->data);

out:
    if (tmpbuf) kfree(tmpbuf);
    sfs_leave();
    return ret;
}

static int extract_link_title(const char *text, sfs_string *title)
{
    int ret = 0;
    char *start;
    char *end;
    char *title_start;
    char *title_end;
    const char *end_atag_patterns[] = {"<","/a",">"};

    sfs_enter();
    sfs_trace("extract_link_title from text '%s'\n", text);

    ret = sfs_string_clear(title); if (ret) goto out;

    start = strchr(text, '>');
    if (!start) goto out;
    title_start = start+1;

    find_spaced_patterns(title_start, end_atag_patterns, 3, &start, &end);
    if (!start) goto out;
    title_end = start;

    sfs_string_ncat(title, title_start, title_end - title_start);
    sfs_trace("extract_link_title result: '%s'\n", title->data);

out:
    sfs_leave();
    return ret;
}

/*Maybe more correct way to implement these functions is to find out
 * how to compile module with linux kernel regexp (linux/kernel/trace/trace.h)*/

int extract_html_link_params(const char *text, struct SURFFS_HTML_LINK *link)
{
    int ret = 0;
    sfs_string tmp = {0};

    sfs_enter();
    sfs_trace("extract_html_link_params from text '%s'\n", text);

    ret = sfs_string_createz(&tmp, 128);
    if (ret) goto out;

    ret = extract_link_title(text, &link->title);
    if (ret) goto out;

    ret = normalize_title(&link->title);
    if (ret) goto out;

    if (!link->title.textlen)
    {
        sfs_trace("cannot get link title from link '%s'\n", text);
        goto out;
    }



    ret = extract_link_href(text, &link->full_url);
    if (ret) {goto out;}
    if (!link->full_url.textlen)
    {
        sfs_trace("cannot get link url from link '%s'\n", text);
        goto out;
    }

    ret = extract_url_params(link->full_url.data,
                             &link->protocol,
                             &link->host,
                             &link->path,
                             PREFER_PATH);
    if (ret) {goto out;}
    if (!link->path.textlen)
    {
        sfs_trace("cannot get link path from link '%s'\n", text);
        goto out;
    }


    ret = SURFFS_HTML_LINK_print(link, &tmp); if (ret) goto out;
    sfs_trace("extract_html_link_params result: %s\n", tmp.data);

out:
    sfs_string_free(&tmp);
    sfs_leave();
    return ret;
}

/*Maybe more correct way to implement these functions is to find out
 * how to compile module with linux kernel regexp (linux/kernel/trace/trace.h)*/

int extract_html_block(const char **textpos, const char *html_tag, sfs_string *block)
{
    int ret = 0;
    const char *p = *textpos;
    char *start;
    char *end;
    char *block_start;
    char *block_end;
    const char *start_patterns[] = {"<", html_tag};
    const char *end_patterns[] = {"<", "/", html_tag, ">"};

    sfs_enter();
    sfs_trace("extract_html_block: %s\n", html_tag);

    ret = sfs_string_clear(block); if (ret) goto out;

    find_spaced_patterns(p, start_patterns, 2, &start, &end);
    if (!start) goto out;
    block_start = start;
    p = end;


    find_spaced_patterns(p, end_patterns, 4, &start, &end);
    if (!start) goto out;
    block_end = end;
    p = end;


    ret = sfs_string_ncat(block, block_start, block_end - block_start);
    if (ret) goto out;
    *textpos = p;

    sfs_trace("extract_html_block result: '%s'\n", block->data);

out:
    sfs_leave();
    return ret;
}




