/*
 ============================================================================
 Name        : hev-socks5-server.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Server
 ============================================================================
 */

#include <string.h>
#include <unistd.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

#include "hev-socks5-udp.h"
#include "hev-socks5-proto.h"
#include "hev-socks5-misc-priv.h"
#include "hev-socks5-logger-priv.h"

#include "hev-socks5-server.h"

#define task_io_yielder hev_socks5_task_io_yielder

HevSocks5Server *
hev_socks5_server_new (int fd)
{
    HevSocks5Server *self;
    int res;

    self = hev_malloc0 (sizeof (HevSocks5Server));
    if (!self)
        return NULL;

    res = hev_socks5_server_construct (self, fd);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    LOG_D ("%p socks5 server new", self);

    return self;
}

void
hev_socks5_server_set_connect_timeout (HevSocks5Server *self, int timeout)
{
    self->timeout = timeout;
}

static int
hev_socks5_server_read_auth_method (HevSocks5Server *self)
{
    HevSocks5Auth auth;
    HevSocks5AuthMethod method;
    int res;
    int i;

    LOG_D ("%p socks5 server read auth method", self);

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &auth, 2, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read auth method", self);
        return -1;
    }

    if (auth.ver != HEV_SOCKS5_VERSION_5) {
        LOG_E ("%p socks5 server auth.ver %u", self, auth.ver);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &auth.methods,
                                   auth.method_len, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read auth methods", self);
        return -1;
    }

    method = HEV_SOCKS5_AUTH_METHOD_NONE;
    if (self->base.auth.user && self->base.auth.pass)
        method = HEV_SOCKS5_AUTH_METHOD_USER;

    res = -1;
    for (i = 0; i < auth.method_len; i++) {
        if (auth.methods[i] == method) {
            res = method;
            break;
        }
    }

    return res;
}

static int
hev_socks5_server_write_auth_method (HevSocks5Server *self, int auth_method)
{
    HevSocks5Auth auth;
    int res;

    LOG_D ("%p socks5 server write auth method", self);

    auth.ver = HEV_SOCKS5_VERSION_5;
    auth.method = auth_method;

    res = hev_task_io_socket_send (HEV_SOCKS5 (self)->fd, &auth, 2, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server write auth method", self);
        return -1;
    }

    return 0;
}

static int
hev_socks5_server_read_auth_user (HevSocks5Server *self)
{
    uint8_t ulen, plen;
    uint8_t buf[257];
    int res;

    LOG_D ("%p socks5 server read auth user", self);

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, buf, 2, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read auth user.ver", self);
        return -1;
    }

    if (buf[0] != 1) {
        LOG_E ("%p socks5 server auth user.ver %u", self, buf[0]);
        return -1;
    }

    ulen = buf[1];
    if (ulen == 0) {
        LOG_E ("%p socks5 server auth user.ulen %u", self, ulen);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, buf, ulen + 1,
                                   MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read auth user.user", self);
        return -1;
    }

    plen = buf[ulen];
    if (plen == 0) {
        LOG_E ("%p socks5 server auth user.plen %u", self, plen);
        return -1;
    }

    buf[ulen] = '\0';
    if (strcmp (self->base.auth.user, (char *)buf) != 0) {
        LOG_E ("%p socks5 server auth user %s", self, buf);
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, buf, plen,
                                   MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read auth user.pass", self);
        return -1;
    }

    buf[plen] = '\0';
    if (strcmp (self->base.auth.pass, (char *)buf) != 0) {
        LOG_E ("%p socks5 server auth pass %s", self, buf);
        return -1;
    }

    return 0;
}

static int
hev_socks5_server_write_auth_user (HevSocks5Server *self, int auth_res)
{
    uint8_t buf[2];
    int res;

    LOG_D ("%p socks5 server write auth user", self);

    buf[0] = 1;
    buf[1] = auth_res;

    res = hev_task_io_socket_send (HEV_SOCKS5 (self)->fd, buf, 2, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server write auth user", self);
        return -1;
    }

    return 0;
}

static int
hev_socks5_server_auth (HevSocks5Server *self)
{
    int method;
    int res;

    method = hev_socks5_server_read_auth_method (self);
    res = hev_socks5_server_write_auth_method (self, method);
    if (res < 0)
        return -1;

    if (method == HEV_SOCKS5_AUTH_METHOD_USER) {
        res = hev_socks5_server_read_auth_user (self);
        res = hev_socks5_server_write_auth_user (self, res);
        if (res < 0)
            return -1;
    }

    return 0;
}

static int
hev_socks5_server_read_addr_ip (HevSocks5Server *self, HevSocks5ReqRes *req,
                                struct sockaddr_in6 *addr)
{
    struct sockaddr *saddr;
    int addrlen;
    int res;

    LOG_D ("%p socks5 server read addr ip", self);

    switch (req->addr.atype) {
    case HEV_SOCKS5_ADDR_TYPE_IPV4:
        addrlen = 6;
        break;
    case HEV_SOCKS5_ADDR_TYPE_IPV6:
        addrlen = 18;
        break;
    default:
        return -1;
    }

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &req->addr.ipv4,
                                   addrlen, MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read addr ip", self);
        return -1;
    }

    addr->sin6_family = AF_INET6;
    saddr = (struct sockaddr *)addr;
    res = hev_socks5_addr_to_sockaddr (&req->addr, saddr);
    if (res < 0) {
        LOG_E ("%p socks5 server to sockaddr", self);
        return -1;
    }

    return 0;
}

static int
hev_socks5_server_read_addr_name (HevSocks5Server *self, HevSocks5ReqRes *req,
                                  struct sockaddr_in6 *addr)
{
    uint16_t port;
    int addrlen;
    char *name;
    int res;

    LOG_D ("%p socks5 server read addr name", self);

    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &req->addr.domain, 1,
                                   MSG_WAITALL, task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read addr name len", self);
        return -1;
    }

    addrlen = req->addr.domain.len + 2;
    res = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd,
                                   &req->addr.domain.addr, addrlen, MSG_WAITALL,
                                   task_io_yielder, self);
    if (res <= 0) {
        LOG_E ("%p socks5 server read addr name", self);
        return -1;
    }

    addr->sin6_family = AF_INET6;
    name = (char *)req->addr.domain.addr;
    memcpy (&port, name + req->addr.domain.len, 2);
    name[req->addr.domain.len] = '\0';
    port = ntohs (port);

    res = hev_socks5_resolve_to_sockaddr6 (name, port, addr);
    if (res < 0) {
        LOG_E ("%p socks5 server resolve [%s]:%d", self, name, port);
        return -1;
    }

    port = htons (port);
    memcpy (name + req->addr.domain.len, &port, 2);

    return 0;
}

static int
hev_socks5_server_read_request (HevSocks5Server *self, int *cmd, int *rep,
                                struct sockaddr_in6 *addr)
{
    HevSocks5ReqRes req;
    int ret;

    LOG_D ("%p socks5 server read request", self);

    ret = hev_task_io_socket_recv (HEV_SOCKS5 (self)->fd, &req, 4, MSG_WAITALL,
                                   task_io_yielder, self);
    if (ret <= 0) {
        LOG_E ("%p socks5 server read request", self);
        return -1;
    }

    if (req.ver != HEV_SOCKS5_VERSION_5) {
        *rep = HEV_SOCKS5_RES_REP_FAIL;
        LOG_E ("%p socks5 server req.ver %u", self, req.ver);
        return 0;
    }

    switch (req.addr.atype) {
    case HEV_SOCKS5_ADDR_TYPE_IPV4:
    case HEV_SOCKS5_ADDR_TYPE_IPV6:
        ret = hev_socks5_server_read_addr_ip (self, &req, addr);
        break;
    case HEV_SOCKS5_ADDR_TYPE_NAME:
        ret = hev_socks5_server_read_addr_name (self, &req, addr);
        break;
    default:
        *rep = HEV_SOCKS5_RES_REP_ADDR;
        LOG_E ("%p socks5 server req.atype %u", self, req.addr.atype);
        return 0;
    }

    if (ret < 0) {
        *rep = HEV_SOCKS5_RES_REP_ADDR;
        LOG_E ("%p socks5 server read addr", self);
        return 0;
    }

    if (LOG_ON ()) {
        char buf[128];
        const char *str;
        const char *type;

        switch (req.cmd) {
        case HEV_SOCKS5_REQ_CMD_CONNECT:
            type = "tcp";
            break;
        case HEV_SOCKS5_REQ_CMD_FWD_UDP:
            type = "udp";
            break;
        default:
            type = "unknown";
            break;
        }

        str = hev_socks5_addr_to_string (&req.addr, buf, sizeof (buf));
        LOG_I ("%p socks5 server %s %s", self, type, str);
    }

    *cmd = req.cmd;

    return 0;
}

static int
hev_socks5_server_write_response (HevSocks5Server *self, int rep,
                                  struct sockaddr_in6 *addr)
{
    struct sockaddr *saddr;
    HevSocks5ReqRes res;
    int ret;

    LOG_D ("%p socks5 server write response", self);

    res.ver = HEV_SOCKS5_VERSION_5;
    res.rep = rep;
    res.rsv = 0;

    saddr = (struct sockaddr *)addr;
    ret = hev_socks5_addr_from_sockaddr (&res.addr, saddr);
    if (ret < 0) {
        LOG_E ("%p socks5 server from sockaddr", self);
        return -1;
    }

    ret = hev_task_io_socket_send (HEV_SOCKS5 (self)->fd, &res, 3 + ret,
                                   MSG_WAITALL, task_io_yielder, self);
    if (ret <= 0) {
        LOG_E ("%p socks5 server write response", self);
        return -1;
    }

    return 0;
}

static int
hev_socks5_server_connect (HevSocks5Server *self, struct sockaddr_in6 *addr)
{
    HevSocks5Class *klass;
    int timeout;
    int res;
    int fd;

    LOG_D ("%p socks5 server connect", self);

    fd = hev_socks5_socket (SOCK_STREAM);
    if (fd < 0) {
        LOG_E ("%p socks5 server socket stream", self);
        return -1;
    }

    klass = HEV_OBJECT_GET_CLASS (self);
    res = klass->binder (HEV_SOCKS5 (self), fd);
    if (res < 0) {
        LOG_E ("%p socks5 server bind", self);
        close (fd);
        return -1;
    }

    timeout = hev_socks5_get_timeout (HEV_SOCKS5 (self));
    hev_socks5_set_timeout (HEV_SOCKS5 (self), self->timeout);
    res = hev_task_io_socket_connect (fd, (struct sockaddr *)addr,
                                      sizeof (*addr), task_io_yielder, self);
    hev_socks5_set_timeout (HEV_SOCKS5 (self), timeout);

    if (res < 0) {
        LOG_E ("%p socks5 server connect", self);
        close (fd);
        return -1;
    }

    self->fd = fd;

    return 0;
}

static int
hev_socks5_server_bind (HevSocks5Server *self)
{
    HevSocks5Class *klass;
    int res;
    int fd;

    LOG_D ("%p socks5 server bind", self);

    fd = hev_socks5_socket (SOCK_DGRAM);
    if (fd < 0) {
        LOG_E ("%p socks5 server socket dgram", self);
        return -1;
    }

    klass = HEV_OBJECT_GET_CLASS (self);
    res = klass->binder (HEV_SOCKS5 (self), fd);
    if (res < 0) {
        LOG_E ("%p socks5 server bind", self);
        close (fd);
        return -1;
    }

    self->fd = fd;

    return 0;
}

static int
hev_socks5_server_handshake (HevSocks5Server *self)
{
    struct sockaddr_in6 addr;
    int cmd;
    int rep;
    int res;

    LOG_D ("%p socks5 server handshake", self);

    res = hev_socks5_server_auth (self);
    if (res < 0)
        return -1;

    rep = HEV_SOCKS5_RES_REP_SUCC;
    res = hev_socks5_server_read_request (self, &cmd, &rep, &addr);
    if (res < 0)
        return -1;

    if (rep == HEV_SOCKS5_RES_REP_SUCC) {
        switch (cmd) {
        case HEV_SOCKS5_REQ_CMD_CONNECT:
            res = hev_socks5_server_connect (self, &addr);
            if (res < 0)
                rep = HEV_SOCKS5_RES_REP_HOST;
            self->type = HEV_SOCKS5_SERVER_TYPE_TCP;
            break;
        case HEV_SOCKS5_REQ_CMD_FWD_UDP:
            res = hev_socks5_server_bind (self);
            if (res < 0)
                rep = HEV_SOCKS5_RES_REP_FAIL;
            self->type = HEV_SOCKS5_SERVER_TYPE_UDP;
            break;
        default:
            rep = HEV_SOCKS5_RES_REP_IMPL;
            break;
        }
    }

    res = hev_socks5_server_write_response (self, rep, &addr);
    if (res < 0)
        return -1;

    return 0;
}

static int
hev_socks5_server_service (HevSocks5Server *self)
{
    LOG_D ("%p socks5 server service", self);

    switch (self->type) {
    case HEV_SOCKS5_SERVER_TYPE_TCP:
        hev_socks5_tcp_splice (HEV_SOCKS5_TCP (self), self->fd);
        break;
    case HEV_SOCKS5_SERVER_TYPE_UDP:
        hev_socks5_udp_splice (HEV_SOCKS5_UDP (self), self->fd);
        break;
    }

    return 0;
}

int
hev_socks5_server_run (HevSocks5Server *self)
{
    HevTask *task = hev_task_self ();
    int res;
    int fd;

    LOG_D ("%p socks5 server run", self);

    fd = HEV_SOCKS5 (self)->fd;
    res = hev_task_add_fd (task, fd, POLLIN | POLLOUT);
    if (res < 0)
        hev_task_mod_fd (task, fd, POLLIN | POLLOUT);

    res = hev_socks5_server_handshake (self);
    if (res < 0)
        return -1;

    res = hev_socks5_server_service (self);
    if (res < 0)
        return -1;

    return 0;
}

int
hev_socks5_server_construct (HevSocks5Server *self, int fd)
{
    int res;

    res = hev_socks5_construct (&self->base);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 server construct", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_SERVER_TYPE;

    HEV_SOCKS5 (self)->fd = fd;

    self->fd = -1;
    self->timeout = -1;

    return 0;
}

static void
hev_socks5_server_destruct (HevObject *base)
{
    HevSocks5Server *self = HEV_SOCKS5_SERVER (base);

    LOG_D ("%p socks5 server destruct", self);

    if (self->fd >= 0)
        close (self->fd);

    HEV_SOCKS5_TYPE->finalizer (base);
}

static void *
hev_socks5_server_iface (HevObject *base, void *type)
{
    HevSocks5ServerClass *klass = HEV_OBJECT_GET_CLASS (base);

    if (type == HEV_SOCKS5_TCP_TYPE)
        return &klass->tcp;

    return NULL;
}

HevObjectClass *
hev_socks5_server_class (void)
{
    static HevSocks5ServerClass klass;
    HevSocks5ServerClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        HevSocks5TCPIface *tiptr;

        memcpy (kptr, HEV_SOCKS5_TYPE, sizeof (HevSocks5Class));

        okptr->name = "HevSocks5Server";
        okptr->finalizer = hev_socks5_server_destruct;
        okptr->iface = hev_socks5_server_iface;

        tiptr = &kptr->tcp;
        memcpy (tiptr, HEV_SOCKS5_TCP_TYPE, sizeof (HevSocks5TCPIface));
    }

    return okptr;
}
