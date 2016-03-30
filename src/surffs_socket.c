#include "surffs_socket.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include <linux/errno.h>
#include <linux/types.h>

#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/delay.h>
#include <linux/un.h>
#include <linux/unistd.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/unistd.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <linux/inet.h>
#include <net/inet_connection_sock.h>
#include <net/request_sock.h>
#include "surffs_debug.h"
#include "surffs_helpers.h"
#include "surffs.h"

static int surffs_alloc_and_connect_socket(struct socket **skt, char *ip, int *connect_ok, sfs_string *log)
{
    mm_segment_t oldfs;
    int ret = 0;
    int errcode;
    struct sockaddr_in* dest = {0};
    char tmpbuf[256];
    struct timeval tv = {
        .tv_sec = SURFFS_SOCKET_TOUT_SEC,
        .tv_usec = SURFFS_SOCKET_TOUT_USEC
    };

    sfs_enter();
    sfs_debug("surffs_alloc_and_connect_socket\n");

    *connect_ok = 0;

    ret = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,skt);
    if (ret) goto out;


    oldfs=get_fs();
    set_fs(KERNEL_DS);

    ret = sock_setsockopt(*skt, SOL_SOCKET, SO_RCVTIMEO,
                         (char *)&tv, sizeof(tv));
    if (ret)
    {
        set_fs(oldfs);
        goto out;
    }

    ret = sock_setsockopt(*skt, SOL_SOCKET, SO_SNDTIMEO,
                         (char *)&tv, sizeof(tv));
    if (ret)
    {
        set_fs(oldfs);
        goto out;
    }
    set_fs(oldfs);

    dest = (struct sockaddr_in*)kmalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
    if (!dest) {ret = -ENOMEM; goto out;}

    dest->sin_family = AF_INET;
    dest->sin_addr.s_addr = in_aton(ip);
    dest->sin_port = htons(SURFFS_HTTP_PORT);
    sfs_debug("connect to %X:%u\n", dest->sin_addr.s_addr, SURFFS_HTTP_PORT);

    errcode = (*skt)->ops->connect(*skt, (struct sockaddr*)dest, sizeof(struct sockaddr_in), !O_NONBLOCK);
    if (errcode < 0)
    {
        sfs_debug("socket connect error\n");
        snprintf(tmpbuf, sizeof(tmpbuf),
                 "error connecting to %s:%d, errcode = %d\n",
                 ip, SURFFS_HTTP_PORT, errcode);

        ret = sfs_string_cat(log, tmpbuf);
        goto out;
    }

    sfs_debug("socket connect OK\n");
    snprintf(tmpbuf, sizeof(tmpbuf),
             "connected to %s:%d\n",
             ip, SURFFS_HTTP_PORT);
    ret = sfs_string_cat(log, tmpbuf);
    if (ret) goto out;

    *connect_ok = 1;

out:
    sfs_leave();
    return ret;
}

static int surffs_make_request(char *path, char *host, sfs_string *request, sfs_string *log)
{
    int ret = 0;
    sfs_enter();
    sfs_debug("surffs_make_request\n");

    ret = sfs_string_clear(request);
    if (ret) goto out;

    ret = sfs_string_cat_param(request, "GET %s HTTP/1.1\n", path);
    if (ret) goto out;

    ret = sfs_string_cat_param(request, "Host: %s\n", host);
    if (ret) goto out;

    ret = sfs_string_cat(request, "User-Agent: surffs_filesystem\n"
                                  "Accept: text/html\n"
                                  "Connection: close\n\n");
    if (ret) goto out;

    sfs_debug("request:\n%s\n----------\n", request->data);

    ret = sfs_string_cat_param(log, "request:\n%s\n----------\n", request->data);
    if (ret) goto out;

out:
    sfs_leave();
    return ret;
}

static int surffs_send(struct socket *skt, sfs_string *request, int *send_ok, sfs_string *log)
{
    struct msghdr msg;
    struct iovec iov;
    int size;
    int ret = 0;
    mm_segment_t oldfs;
    char tmpbuf[256];
    *send_ok = 0;

    sfs_enter();
    sfs_debug("surffs_send: '%s'\n", request->data);

    iov.iov_base=request->data;
    iov.iov_len=request->textlen;

    msg.msg_control=NULL;
    msg.msg_controllen=0;
    msg.msg_flags=0;
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
    msg.msg_name=0;
    msg.msg_namelen=0;

    oldfs=get_fs(); //Store current virtual address bounds
    set_fs(KERNEL_DS); //Switch to kernel address bounds
    size=sock_sendmsg(skt,&msg,request->textlen);
    set_fs(oldfs); //return to prev address bounds

    if (size != request->textlen)
    {
        sfs_debug("surffs_send error\n");
        snprintf(tmpbuf, sizeof(tmpbuf),
                 "error sending data to socket: sent only %d of %d bytes\n",
                 size, (int)request->textlen);
        ret = sfs_string_cat(log, tmpbuf);
        goto out;
    }

    sfs_debug("surffs_send OK\n");
    *send_ok = 1;

out:
    sfs_leave();
    return ret;
}

static int surffs_rcv_chunk(struct socket *skt, void *chunkbuf, int chunklen)
{
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    int ret = 0;

    sfs_enter();
    sfs_debug("surffs_rcv_chunk\n");

    if(skt->sk==NULL) goto out;

    iov.iov_base = chunkbuf;
    iov.iov_len = chunklen;

    msg.msg_control=NULL;
    msg.msg_controllen=0;
    msg.msg_flags=0;
    msg.msg_name=0;
    msg.msg_namelen=0;
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;

    oldfs=get_fs();  //Store current virtual address bounds
    set_fs(KERNEL_DS);  //Switch to kernel address bounds
    sfs_debug("sock_recvmsg...\n");
    ret=sock_recvmsg(skt,&msg,chunklen,msg.msg_flags);
    sfs_debug("sock_recvmsg OK\n");
    set_fs(oldfs);//return to prev address bounds

    if (ret > 0)
    {
        sfs_debug("received chunk, %d bytes\n", ret);
    }
    else
    {
        sfs_debug("receive chunk ret = %d\n", ret);
    }

out:
    sfs_leave();
    return ret;
}

static int surffs_rcv(struct socket *skt, sfs_string *text, int *rcv_ok, sfs_string *log)
{
    int ret = 0;
    int readret;
    char *chunkbuf = 0;
    int size = 0;
    char tmpbuf[256];

    sfs_enter();
    sfs_debug("surffs_rcv\n");

    *rcv_ok = 0;

    chunkbuf = kmalloc(SURFFS_HTTP_CHUNK_SIZE + 1, GFP_KERNEL);
    if (!chunkbuf) {ret = -ENOMEM; goto out;}

    ret = sfs_string_clear(text);
    if (ret) goto out;

    while (1)
    {
        readret = surffs_rcv_chunk(skt, chunkbuf, SURFFS_HTTP_CHUNK_SIZE);
        if ((readret > 0) && (readret <= SURFFS_HTTP_CHUNK_SIZE))
        {
            chunkbuf[readret] = 0;
            sfs_string_cat(text, chunkbuf);
            size += readret;
        }
        else if (readret < 0)
        {
            ret = readret;
            goto out;
        }
        else if (readret > SURFFS_HTTP_CHUNK_SIZE)
        {
            ret = -EIO;
            goto out;
        }
        else break;
    }

    sfs_debug("received %d bytes\n", size);

    snprintf(tmpbuf, sizeof(tmpbuf), "received %d bytes\n", size);
    ret = sfs_string_cat(log, tmpbuf);
    if (ret) goto out;

    *rcv_ok = 1;

out:
    if (chunkbuf) kfree(chunkbuf);
    sfs_leave();
    return ret;
}

static void surffs_free_socket(struct socket *skt)
{
    sock_release(skt);
}

static int surffs_extract_http_payload(sfs_string *text, char** payload, sfs_string *log)
{
    int ret = 0;
    char* found;

    sfs_enter();
    sfs_debug("surffs_extract_payload\n");

    *payload = 0;

    if (!text->textlen)
    {
        sfs_debug("no text\n");
        ret = sfs_string_cat(log, "error extract http payload: empty server response\n");
        goto out;
    }

    found = strstr(text->data, "200 OK");
    if (!found)
    {
        sfs_debug("\"200 OK\" not found\n");
        ret = sfs_string_cat_param(log,
          "error extract http payload: bad http status code (\"200 OK\" not found). "
          "Server response:\n%s\n---------\n", text->data);
        goto out;
    }

#define SURFFS_HTTP_HEADERS_SEPARATOR "\r\n\r\n"
    found = strstr(text->data, SURFFS_HTTP_HEADERS_SEPARATOR);
    if (!found)
    {
        sfs_debug("SURFFS_HTTP_HEADERS_SEPARATOR ('%s') not found\n"
                  ,SURFFS_HTTP_HEADERS_SEPARATOR);

        ret = sfs_string_cat_param(log,
            "error extract http payload: http headers separator not found ('%s')\n",
            SURFFS_HTTP_HEADERS_SEPARATOR);

        goto out;
    }

    if ((found - text->data) + strlen(SURFFS_HTTP_HEADERS_SEPARATOR) >= text->textlen)
    {
        sfs_debug("no http data\n");
        ret = sfs_string_cat(log, "error extract http payload: no http body\n");
        goto out;
    }

    *payload = found + strlen(SURFFS_HTTP_HEADERS_SEPARATOR);

    ret = sfs_string_cat(log, "http data extracted\n");
    if (ret) goto out;

out:
    sfs_leave();
    return ret;
}

int surffs_get_http(char *ip, char *host, char *path,
                    sfs_string *http_response,
                    char** http_payload_start,
                    sfs_string *log)
{
    struct socket *skt = 0;
    sfs_string request = {0};
    int ret = 0;
    int ok = 0;

    sfs_enter();
    sfs_debug("surffs_get_http, ip='%s', host='%s', path='%s'\n", ip, host, path);

    *http_payload_start = 0;

    ret = sfs_string_clear(http_response);
    if (ret) goto out;

    ret = surffs_alloc_and_connect_socket(&skt, ip, &ok, log);
    if (ret || !ok) goto out;

    ret = sfs_string_createz(&request, 512);
    if (ret) goto out;

    ret = surffs_make_request(path, host, &request, log);
    if (ret) goto out;

    ret = surffs_send(skt, &request, &ok, log);
    if (ret) goto out;
    if (ret || !ok) goto out;

    ret = surffs_rcv(skt, http_response, &ok, log);
    if (ret || !ok) goto out;

    ret = surffs_extract_http_payload(http_response, http_payload_start, log);
    if (ret) goto out;

    if (*http_payload_start)
        {sfs_debug("http_payload_start extacted\n");}
    else
        {sfs_debug("http responce doesn't contain http_payload_start\n");}

out:
    if (skt) surffs_free_socket(skt);
    sfs_string_free(&request);
    sfs_leave();
    return ret;
}

