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

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>

#include "hev-socks5.h"
#include "hev-socks5-misc-priv.h"
#include "hev-socks5-logger-priv.h"

#include "hev-socks5-udp.h"

#define task_io_yielder hev_socks5_task_io_yielder

int
hev_socks5_udp_sendto (HevSocks5UDP *self, const void *buf, size_t len,
                       struct sockaddr *addr)
{
    HevSocks5UDPHdr udp;
    struct iovec iov[3];
    struct msghdr mh;
    uint16_t datalen;
    int addrlen;
    int res;

    LOG_D ("%p socks5 udp sendto", self);

    udp.rsv[0] = 0;
    udp.rsv[1] = 0;
    udp.rsv[2] = 0;

    addrlen = hev_socks5_addr_from_sockaddr (&udp.addr, addr);
    if (addrlen <= 0) {
        LOG_E ("%p socks5 udp from sockaddr", self);
        return -1;
    }

    datalen = htons (len);

    memset (&mh, 0, sizeof (mh));
    mh.msg_iov = iov;
    mh.msg_iovlen = 3;

    iov[0].iov_base = &udp;
    iov[0].iov_len = 3 + addrlen;
    iov[1].iov_base = &datalen;
    iov[1].iov_len = sizeof (datalen);
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

int
hev_socks5_udp_recvfrom (HevSocks5UDP *self, void *buf, size_t len,
                         struct sockaddr *addr)
{
    HevSocks5UDPHdr udp;
    uint16_t datalen;
    int addrlen;
    int res;

    LOG_D ("%p socks5 udp recvfrom", self);

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &udp, 4, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read udp", self);
        return -1;
    }

    switch (udp.addr.atype) {
    case HEV_SOCKS5_ADDR_TYPE_IPV4:
        addrlen = 6;
        break;
    case HEV_SOCKS5_ADDR_TYPE_IPV6:
        addrlen = 18;
        break;
    default:
        LOG_E ("%p socks5 udp addr.atype %u", self, udp.addr.atype);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &udp.addr.ipv4,
                                   addrlen, MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp read addr", self);
        return -1;
    }

    res = hev_socks5_addr_to_sockaddr (&udp.addr, addr);
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

static int
hev_socks5_udp_fwd_f (HevSocks5UDP *self, int fd)
{
    struct sockaddr_in6 addr;
    struct sockaddr *saddr;
    uint8_t buf[1500];
    int cfd;
    int res;

    cfd = HEV_SOCKS5 (self)->fd;
    if (cfd < 0)
        return -1;

    LOG_D ("%p socks5 udp fwd f", self);

    res = recv (cfd, buf, 1, MSG_PEEK);
    if (res <= 0) {
        if ((res < 0) && (errno == EAGAIN))
            return 0;
        LOG_E ("%p socks5 udp fwd f peek", self);
        return -1;
    }

    addr.sin6_family = AF_INET6;
    saddr = (struct sockaddr *)&addr;
    res = hev_socks5_udp_recvfrom (self, buf, sizeof (buf), saddr);
    if (res <= 0) {
        LOG_E ("%p socks5 udp fwd f recv", self);
        return -1;
    }

    res = hev_task_io_socket_sendto (fd, buf, res, 0, saddr, sizeof (addr),
                                     task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 udp fwd f send", self);
        return -1;
    }

    return 1;
}

static int
hev_socks5_udp_fwd_b (HevSocks5UDP *self, int fd)
{
    struct sockaddr_in6 addr;
    struct sockaddr *saddr;
    socklen_t addrlen;
    uint8_t buf[1500];
    int cfd;
    int res;

    cfd = HEV_SOCKS5 (self)->fd;
    if (cfd < 0)
        return -1;

    LOG_D ("%p socks5 udp fwd b", self);

    addrlen = sizeof (addr);
    saddr = (struct sockaddr *)&addr;

    res = recvfrom (fd, buf, sizeof (buf), 0, saddr, &addrlen);
    if (res <= 0) {
        if ((res < 0) && (errno == EAGAIN))
            return 0;
        LOG_E ("%p socks5 udp fwd b recv", self);
        return -1;
    }

    res = hev_socks5_udp_sendto (self, buf, res, saddr);
    if (res <= 0) {
        LOG_E ("%p socks5 udp fwd b send", self);
        return -1;
    }

    return 1;
}

int
hev_socks5_udp_splice (HevSocks5UDP *self, int fd)
{
    HevTask *task = hev_task_self ();
    int res_f = 1;
    int res_b = 1;

    LOG_D ("%p socks5 udp splice", self);

    if (hev_task_add_fd (task, fd, POLLIN | POLLOUT) < 0)
        hev_task_mod_fd (task, fd, POLLIN | POLLOUT);

    for (;;) {
        HevTaskYieldType type;

        if (res_f >= 0)
            res_f = hev_socks5_udp_fwd_f (self, fd);
        if (res_b >= 0)
            res_b = hev_socks5_udp_fwd_b (self, fd);

        if (res_f < 0 || res_b < 0)
            break;
        else if (res_f > 0 || res_b > 0)
            type = HEV_TASK_YIELD;
        else
            type = HEV_TASK_WAITIO;

        if (task_io_yielder (type, self) < 0)
            break;
    }

    return 0;
}
