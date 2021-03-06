/*
 ============================================================================
 Name        : hev-socks5-udp.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 UDP
 ============================================================================
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-socks5.h"
#include "hev-socks5-misc-priv.h"
#include "hev-socks5-logger-priv.h"

#include "hev-socks5-udp.h"

#define task_io_yielder hev_socks5_task_io_yielder

typedef struct _HevSocks5UDPSplice HevSocks5UDPSplice;

struct _HevSocks5UDPSplice
{
    HevSocks5UDP *udp;
    HevTask *task;
    int fd;
};

int
hev_socks5_udp_sendto (HevSocks5UDP *self, const void *buf, size_t len,
                       struct sockaddr *addr)
{
    HevSocks5UDPHdr udp;
    struct iovec iov[3];
    struct msghdr mh;
    int addrlen;
    int res;

    LOG_D ("%p socks5 udp sendto", self);

    addrlen = hev_socks5_addr_from_sockaddr (&udp.addr, addr);
    if (addrlen <= 0) {
        LOG_E ("%p socks5 udp from sockaddr", self);
        return -1;
    }

    udp.datlen = htons (len);
    udp.hdrlen = 5 + addrlen;

    memset (&mh, 0, sizeof (mh));
    mh.msg_iov = iov;
    mh.msg_iovlen = 3;

    iov[0].iov_base = &udp;
    iov[0].iov_len = udp.hdrlen - 2;
    iov[1].iov_base = &udp.datlen;
    iov[1].iov_len = sizeof (udp.datlen);
    iov[2].iov_base = (void *)buf;
    iov[2].iov_len = len;

    res = hev_task_io_socket_sendmsg (HEV_SOCKS5 (self)->fd, &mh, MSG_WAITALL,
                                      task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp write udp", self);
        return -1;
    }

    return res;
}

static int
hev_socks5_udp_recvfrom_fallback (HevSocks5UDP *self, HevSocks5UDPHdr *udp,
                                  void *buf, size_t len, struct sockaddr *addr)
{
    uint16_t datalen;
    int addrlen;
    int res;

    LOG_D ("%p socks5 udp recvfrom fallback", self);

    switch (udp->addr.atype) {
    case HEV_SOCKS5_ADDR_TYPE_IPV4:
        addrlen = 6;
        break;
    case HEV_SOCKS5_ADDR_TYPE_IPV6:
        addrlen = 18;
        break;
    default:
        LOG_E ("%p socks5 udp addr.atype %u", self, udp->addr.atype);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &udp->addr.ipv4,
                                   addrlen, MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read addr", self);
        return -1;
    }

    res = hev_socks5_addr_to_sockaddr (&udp->addr, addr);
    if (res < 0) {
        LOG_E ("%p socks5 udp to sockaddr", self);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &datalen,
                                   sizeof (datalen), MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read data len", self);
        return -1;
    }

    datalen = ntohs (datalen);
    if (datalen > len) {
        LOG_E ("%p socks5 udp data len", self);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, buf, datalen,
                                   MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read data", self);
        return -1;
    }

    return res;
}

int
hev_socks5_udp_recvfrom (HevSocks5UDP *self, void *buf, size_t len,
                         struct sockaddr *addr)
{
    HevSocks5UDPHdr udp;
    struct iovec iov[2];
    struct msghdr mh;
    int res;

    LOG_D ("%p socks5 udp recvfrom", self);

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &udp, 4, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read udp head", self);
        return -1;
    }

    if (udp.hdrlen == 0)
        return hev_socks5_udp_recvfrom_fallback (self, &udp, buf, len, addr);

    udp.datlen = ntohs (udp.datlen);
    if (udp.datlen > len) {
        LOG_E ("%p socks5 udp data len", self);
        return -1;
    }

    memset (&mh, 0, sizeof (mh));
    mh.msg_iov = iov;
    mh.msg_iovlen = 2;

    iov[0].iov_base = &udp.addr.ipv4;
    iov[0].iov_len = udp.hdrlen - 4;
    iov[1].iov_base = buf;
    iov[1].iov_len = udp.datlen;

    res = hev_task_io_socket_recvmsg (HEV_SOCKS5 (self)->fd, &mh, MSG_WAITALL,
                                      task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read udp data", self);
        return -1;
    }

    res = hev_socks5_addr_to_sockaddr (&udp.addr, addr);
    if (res < 0) {
        LOG_E ("%p socks5 udp to sockaddr", self);
        return -1;
    }

    return udp.datlen;
}

static int
hev_socks5_udp_fwd_f (HevSocks5UDP *self, int fd)
{
    struct sockaddr_in6 addr;
    struct sockaddr *saddr;
    uint8_t buf[1500];
    int res;

    LOG_D ("%p socks5 udp fwd f", self);

    addr.sin6_family = AF_INET6;
    saddr = (struct sockaddr *)&addr;
    res = hev_socks5_udp_recvfrom (self, buf, sizeof (buf), saddr);
    if (res <= 0) {
        LOG_E ("%p socks5 udp fwd f recv", self);
        return -1;
    }

    res = sendto (fd, buf, res, 0, saddr, sizeof (addr));
    if (res <= 0) {
        if ((res < 0) && (errno == EAGAIN))
            return 0;
        LOG_E ("%p socks5 udp fwd f send", self);
        return -1;
    }

    return 0;
}

static int
hev_socks5_udp_fwd_b (HevSocks5UDP *self, int fd)
{
    struct sockaddr_in6 addr;
    struct sockaddr *saddr;
    socklen_t addrlen;
    uint8_t buf[1500];
    int res;

    LOG_D ("%p socks5 udp fwd b", self);

    addrlen = sizeof (addr);
    saddr = (struct sockaddr *)&addr;

    res = hev_task_io_socket_recvfrom (fd, buf, sizeof (buf), 0, saddr,
                                       &addrlen, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp fwd b recv", self);
        return -1;
    }

    res = hev_socks5_udp_sendto (self, buf, res, saddr);
    if (res <= 0) {
        LOG_E ("%p socks5 udp fwd b send", self);
        return -1;
    }

    return 0;
}

static void
splice_task_entry (void *data)
{
    HevSocks5UDPSplice *splice = data;
    HevSocks5UDP *self = splice->udp;
    HevTask *task = hev_task_self ();
    int fd;

    fd = hev_task_io_dup (HEV_SOCKS5 (self)->fd);
    if (fd < 0)
        goto exit;

    if (hev_task_add_fd (task, fd, POLLIN) < 0)
        hev_task_mod_fd (task, fd, POLLIN);

    for (;;) {
        int res;

        res = hev_socks5_udp_fwd_f (self, splice->fd);
        if (res < 0)
            break;

        res = task_io_yielder (HEV_TASK_YIELD, self);
        if (res < 0)
            break;
    }

    hev_task_del_fd (task, fd);
    close (fd);

exit:
    hev_task_wakeup (splice->task);
}

static int
hev_socks5_udp_splicer (HevSocks5UDP *self, int fd)
{
    HevTask *task = hev_task_self ();
    HevSocks5UDPSplice splice;
    int stack_size;

    LOG_D ("%p socks5 udp splicer", self);

    if (hev_task_add_fd (task, fd, POLLIN) < 0)
        hev_task_mod_fd (task, fd, POLLIN);

    splice.task = task;
    splice.udp = self;
    splice.fd = fd;

    stack_size = hev_socks5_get_task_stack_size ();
    task = hev_task_new (stack_size);
    hev_task_run (task, splice_task_entry, &splice);
    task = hev_task_ref (task);

    for (;;) {
        int res;

        res = hev_socks5_udp_fwd_b (self, fd);
        if (res < 0)
            break;

        res = task_io_yielder (HEV_TASK_YIELD, self);
        if (res < 0)
            break;
    }

    for (;;) {
        if (hev_task_get_state (task) == HEV_TASK_STOPPED)
            break;

        hev_task_wakeup (task);
        hev_task_yield (HEV_TASK_WAITIO);
    }

    hev_task_unref (task);

    return 0;
}

int
hev_socks5_udp_splice (HevSocks5UDP *self, int fd)
{
    HevSocks5UDPIface *iface;

    iface = HEV_OBJECT_GET_IFACE (self, HEV_SOCKS5_UDP_TYPE);
    return iface->splicer (self, fd);
}

void *
hev_socks5_udp_iface (void)
{
    static HevSocks5UDPIface type = {
        .splicer = hev_socks5_udp_splicer,
    };

    return &type;
}
