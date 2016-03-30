#include "surffs_debug.h"
#include "surffs_internet.h"
#include "surffs_helpers.h"
#include "surffs_socket.h"
#include <linux/hashtable.h>
#include <linux/slab.h>

enum SFS_LOG_MODE
{
    LOG_NORMAL = 0,
    LOG_ENTER
};

int sfs_log_nesting = 0;
enum SFS_LOG_MODE sfs_log_mode = LOG_NORMAL;

inline void sfs_enter(void)
{
    sfs_log_mode = LOG_ENTER;
    sfs_log_nesting++;
}

inline void sfs_leave(void)
{
    sfs_log_mode = LOG_NORMAL;
    if (sfs_log_nesting) sfs_log_nesting--;
}

char sfs_log_paddings[256];
int logpd_init = 0;

inline void log_paddings_init(void)
{
    memset(sfs_log_paddings, ' ', sizeof(sfs_log_paddings) - 1);
    sfs_log_paddings[sizeof(sfs_log_paddings) - 1] = 0;
}

char *get_sfs_log_padding(void)
{
    char *res;
    int shift;
    #define one_shift_size 4

    if (!logpd_init)
    {
        log_paddings_init();
        logpd_init = 1;
    }

    shift = sfs_log_nesting * one_shift_size;
    if (shift > sizeof(sfs_log_paddings) - 1) return "";

    res = sfs_log_paddings + sizeof(sfs_log_paddings) - 1 - shift;

    return res;
}

char *get_sfs_log_start(void)
{
    switch (sfs_log_mode)
    {
        case LOG_NORMAL: return "|";
        case LOG_ENTER: return ">";
        default: return "";
    }
}

inline void sfs_after_log(void)
{
    sfs_log_mode = LOG_NORMAL;
}




