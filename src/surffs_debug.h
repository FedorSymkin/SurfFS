#ifndef _SURFFS_DEBUG_H_
#define _SURFFS_DEBUG_H_

#include <linux/kernel.h>
#include <linux/printk.h>

extern int sfs_log_nesting;

#define SURFFS_LOGLEVEL_QUIET       0
#define SURFFS_LOGLEVEL_ERROR       1
#define SURFFS_LOGLEVEL_WARNING     2
#define SURFFS_LOGLEVEL_INFO        3
#define SURFFS_LOGLEVEL_DEBUG       4
#define SURFFS_LOGLEVEL_TRACE       5
#define SURFFS_CURRENT_LOGLEVEL     SURFFS_LOGLEVEL_ERROR

#define sfs_print_log(level, tag, fmt, ...) { \
            if (SURFFS_CURRENT_LOGLEVEL >= level) { \
                pr_info("[surffs "tag"]: %s%s"fmt, \
                    get_sfs_log_padding(), get_sfs_log_start(), ##__VA_ARGS__); \
            } sfs_after_log();}


#define sfs_error(fmt, ...)     sfs_print_log(SURFFS_LOGLEVEL_ERROR,   "ERR", fmt, ##__VA_ARGS__)
#define sfs_warning(fmt, ...)   sfs_print_log(SURFFS_LOGLEVEL_WARNING, "WRN", fmt, ##__VA_ARGS__)
#define sfs_info(fmt, ...)      sfs_print_log(SURFFS_LOGLEVEL_INFO,    "inf", fmt, ##__VA_ARGS__)
#define sfs_debug(fmt, ...)     sfs_print_log(SURFFS_LOGLEVEL_DEBUG,   "dbg", fmt, ##__VA_ARGS__)
#define sfs_trace(fmt, ...)     sfs_print_log(SURFFS_LOGLEVEL_TRACE,   "trc", fmt, ##__VA_ARGS__)

char *get_sfs_log_padding(void);
char *get_sfs_log_start(void);
inline void sfs_after_log(void);
inline void sfs_enter(void);
inline void sfs_leave(void);

#endif
