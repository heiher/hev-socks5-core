/*
 ============================================================================
 Name        : hev-socks5-udp.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 - 2023 hev
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

typedef enum _HevSocks5UDPAlive HevSocks5UDPAlive;
typedef struct _HevSocks5UDPSplice HevSocks5UDPSplice;

enum _HevSocks5UDPAlive
{
    HEV_SOCKS5_UDP_ALIVE_F = (1 << 0),
    HEV_SOCKS5_UDP_ALIVE_B = (1 << 1),
};

struct _HevSocks5UDPSplice
{
    HevSocks5UDP *udp;
    HevTask *task;
    HevSocks5UDPAlive alive;
    int fd;
};

int
hev_socks5_udp_get_fd (HevSocks5UDP *self)
{
    HevSocks5UDPIface *iface;

    iface = HEV_OBJECT_GET_IFACE (self, HEV_SOCKS5_UDP_TYPE);
    return iface->get_fd (self);
}

int
hev_socks5_udp_sendto (HevSocks5UDP *self, const void *buf, size_t len,
                       struct sockaddr *addr)
{
    HevSocks5UDPHdr udp;
    struct iovec iov[2];
    struct msghdr mh;
    int addrlen;
    int res;

    LOG_D ("%p socks5 udp sendto", self);

    addrlen = hev_socks5_addr_from_sockaddr (&udp.addr, addr);
    if (addrlen <= 0) {
        LOG_D ("%p socks5 udp from sockaddr", self);
        return -1;
    }

    switch (HEV_SOCKS5 (self)->type) {
    case HEV_SOCKS5_TYPE_UDP_IN_TCP:
        udp.datlen = htons (len);
        udp.hdrlen = 3 + addrlen;
        break;
    case HEV_SOCKS5_TYPE_UDP_IN_UDP:
        udp.datlen = 0;
        udp.hdrlen = 0;
        break;
    default:
        return -1;
    }

    memset (&mh, 0, sizeof (mh));
    mh.msg_iov = iov;
    mh.msg_iovlen = 2;
    mh.msg_name = HEV_SOCKS5 (self)->data;
    mh.msg_namelen = sizeof (struct sockaddr_in6);

    iov[0].iov_base = &udp;
    iov[0].iov_len = 3 + addrlen;
    iov[1].iov_base = (void *)buf;
    iov[1].iov_len = len;

    res = hev_task_io_socket_sendmsg (hev_socks5_udp_get_fd (self), &mh,
                                      MSG_WAITALL, task_io_yielder, self);
    if (res <= 0)
        LOG_D ("%p socks5 udp write udp", self);

    return res;
}

static int
hev_socks5_udp_recvfrom_tcp (HevSocks5UDP *self, void *buf, size_t len,
                             struct sockaddr *addr)
{
    HevSocks5UDPHdr udp;
    struct iovec iov[2];
    struct msghdr mh;
    int res;
    int fd;

    LOG_D ("%p socks5 udp recvfrom tcp", self);

    fd = hev_socks5_udp_get_fd (self);
    res = hev_task_io_socket_recv (fd, &udp, 4, MSG_WAITALL, task_io_yielder,
                                   self);
    if (res <= 0) {
        LOG_D ("%p socks5 udp read udp head", self);
        return res;
    }

    udp.datlen = ntohs (udp.datlen);
    if (udp.datlen > len) {
        LOG_D ("%p socks5 udp data len", self);
        return -1;
    }

    memset (&mh, 0, sizeof (mh));
    mh.msg_iov = iov;
    mh.msg_iovlen = 2;

    iov[0].iov_base = &udp.addr.ipv4;
    iov[0].iov_len = udp.hdrlen - 4;
    iov[1].iov_base = buf;
    iov[1].iov_len = udp.datlen;

    res = hev_task_io_socket_recvmsg (fd, &mh, MSG_WAITALL, task_io_yielder,
                                      self);
    if (res <= 0) {
        LOG_D ("%p socks5 udp read udp data", self);
        return res;
    }

    res = hev_socks5_addr_to_sockaddr (&udp.addr, addr);
    if (res < 0) {
        LOG_D ("%p socks5 udp to sockaddr", self);
        return -1;
    }

    return udp.datlen;
}

static int
hev_socks5_udp_recvfrom_udp (HevSocks5UDP *self, void *buf, size_t len,
                             struct sockaddr *addr)
{
    HevSocks5UDPHdr *udp;
    uint8_t rbuf[1500];
    socklen_t alen;
    ssize_t rlen;
    void *adat;
    int doff;
    int res;

    LOG_D ("%p socks5 udp recvfrom udp", self);

    adat = HEV_SOCKS5 (self)->data;
    alen = sizeof (struct sockaddr_in6);
    rlen = hev_task_io_socket_recvfrom (hev_socks5_udp_get_fd (self), rbuf,
                                        sizeof (rbuf), 0, adat, &alen,
                                        task_io_yielder, self);
    if (rlen < 4) {
        LOG_D ("%p socks5 udp read", self);
        return rlen;
    }

    udp = (HevSocks5UDPHdr *)rbuf;
    switch (udp->addr.atype) {
    case HEV_SOCKS5_ADDR_TYPE_IPV4:
        doff = 10;
        break;
    case HEV_SOCKS5_ADDR_TYPE_IPV6:
        doff = 22;
        break;
    default:
        return -1;
    }

    if (doff > rlen) {
        LOG_D ("%p socks5 udp data len", self);
        return -1;
    }

    rlen -= doff;
    if (len < rlen)
        rlen = len;
    memcpy (buf, rbuf + doff, rlen);

    res = hev_socks5_addr_to_sockaddr (&udp->addr, addr);
    if (res < 0) {
        LOG_D ("%p socks5 udp to sockaddr", self);
        return -1;
    }

    return rlen;
}

int
hev_socks5_udp_recvfrom (HevSocks5UDP *self, void *buf, size_t len,
                         struct sockaddr *addr)
{
    int res;

    switch (HEV_SOCKS5 (self)->type) {
    case HEV_SOCKS5_TYPE_UDP_IN_TCP:
        res = hev_socks5_udp_recvfrom_tcp (self, buf, len, addr);
        break;
    case HEV_SOCKS5_TYPE_UDP_IN_UDP:
        res = hev_socks5_udp_recvfrom_udp (self, buf, len, addr);
        break;
    default:
        return -1;
    }

    return res;
}

static int
hev_socks5_udp_fwd_f (HevSocks5UDP *self, HevSocks5UDPSplice *splice)
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
        if (res < -1) {
            splice->alive &= ~HEV_SOCKS5_UDP_ALIVE_F;
            if (splice->alive)
                return 0;
        }
        LOG_D ("%p socks5 udp fwd f recv", self);
        return -1;
    }

    res = sendto (splice->fd, buf, res, 0, saddr, sizeof (addr));
    if (res <= 0) {
        if ((res < 0) && (errno == EAGAIN))
            return 0;
        LOG_D ("%p socks5 udp fwd f send", self);
        return -1;
    }

    splice->alive |= HEV_SOCKS5_UDP_ALIVE_F;

    return 0;
}

static int
hev_socks5_udp_fwd_b (HevSocks5UDP *self, HevSocks5UDPSplice *splice)
{
    struct sockaddr_in6 addr;
    struct sockaddr *saddr;
    socklen_t addrlen;
    uint8_t buf[1500];
    int res;

    LOG_D ("%p socks5 udp fwd b", self);

    addrlen = sizeof (addr);
    saddr = (struct sockaddr *)&addr;

    res = hev_task_io_socket_recvfrom (splice->fd, buf, sizeof (buf), 0, saddr,
                                       &addrlen, task_io_yielder, self);
    if (res <= 0) {
        if (res < -1) {
            splice->alive &= ~HEV_SOCKS5_UDP_ALIVE_B;
            if (splice->alive && hev_socks5_get_timeout (HEV_SOCKS5 (self)))
                return 0;
        }
        LOG_D ("%p socks5 udp fwd b recv", self);
        return -1;
    }

    res = hev_socks5_udp_sendto (self, buf, res, saddr);
    if (res <= 0) {
        if (res < -1) {
            splice->alive &= ~HEV_SOCKS5_UDP_ALIVE_B;
            if (splice->alive && hev_socks5_get_timeout (HEV_SOCKS5 (self)))
                return 0;
        }
        LOG_D ("%p socks5 udp fwd b send", self);
        return -1;
    }

    splice->alive |= HEV_SOCKS5_UDP_ALIVE_B;

    return 0;
}

static void
splice_task_entry (void *data)
{
    HevSocks5UDPSplice *splice = data;
    HevSocks5UDP *self = splice->udp;
    HevTask *task = hev_task_self ();
    int fd;

    fd = hev_task_io_dup (hev_socks5_udp_get_fd (self));
    if (fd < 0)
        goto exit;

    if (hev_task_add_fd (task, fd, POLLIN) < 0)
        hev_task_mod_fd (task, fd, POLLIN);

    for (;;) {
        if (hev_socks5_udp_fwd_f (self, splice) < 0)
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
    int ufd;

    LOG_D ("%p socks5 udp splicer", self);

    splice.task = task;
    splice.udp = self;
    splice.alive = HEV_SOCKS5_UDP_ALIVE_F | HEV_SOCKS5_UDP_ALIVE_B;
    splice.fd = fd;

    stack_size = hev_socks5_get_task_stack_size ();
    task = hev_task_new (stack_size);
    hev_task_run (task, splice_task_entry, &splice);
    task = hev_task_ref (task);

    if (hev_task_add_fd (splice.task, fd, POLLIN) < 0)
        hev_task_mod_fd (splice.task, fd, POLLIN);

    ufd = hev_socks5_udp_get_fd (self);
    if (hev_task_mod_fd (splice.task, ufd, POLLOUT) < 0)
        hev_task_add_fd (splice.task, ufd, POLLOUT);

    for (;;) {
        if (hev_socks5_udp_fwd_b (self, &splice) < 0)
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
