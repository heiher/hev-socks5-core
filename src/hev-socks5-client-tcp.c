/*
 ============================================================================
 Name        : hev-socks5-client-tcp.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Client TCP
 ============================================================================
 */

#include <string.h>

#include <hev-memory-allocator.h>

#include "hev-socks5-misc-priv.h"
#include "hev-socks5-logger-priv.h"

#include "hev-socks5-client-tcp.h"

HevSocks5ClientTCP *
hev_socks5_client_tcp_new (const char *addr, int port)
{
    HevSocks5ClientTCP *self;
    int res;

    self = hev_malloc0 (sizeof (HevSocks5ClientTCP));
    if (!self)
        return NULL;

    res = hev_socks5_client_tcp_construct (self, addr, port);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    LOG_I ("%p socks5 client tcp new [%s]:%d", self, addr, port);

    return self;
}

HevSocks5ClientTCP *
hev_socks5_client_tcp_new_ip (struct sockaddr *addr)
{
    HevSocks5ClientTCP *self;
    int res;

    self = hev_malloc0 (sizeof (HevSocks5ClientTCP));
    if (!self)
        return NULL;

    res = hev_socks5_client_tcp_construct_ip (self, addr);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    if (LOG_ON ()) {
        char buf[128];
        const char *str;

        str = hev_socks5_addr_to_string (self->addr, buf, sizeof (buf));
        LOG_I ("%p socks5 client tcp new ip %s", self, str);
    }

    return self;
}

static HevSocks5Addr *
hev_socks5_client_tcp_get_upstream_addr (HevSocks5Client *base)
{
    HevSocks5ClientTCP *self = HEV_SOCKS5_CLIENT_TCP (base);
    HevSocks5Addr *addr;

    addr = self->addr;
    self->addr = NULL;

    return addr;
}

int
hev_socks5_client_tcp_construct (HevSocks5ClientTCP *self, const char *addr,
                                 int port)
{
    int addrlen;
    int nport;
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_CLIENT_TYPE_TCP);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 client tcp construct", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_CLIENT_TCP_TYPE;

    addrlen = strlen (addr);
    self->addr = hev_malloc (4 + addrlen);
    if (!self->addr)
        return -1;

    nport = htons (port);
    self->addr->atype = HEV_SOCKS5_ADDR_TYPE_NAME;
    self->addr->domain.len = addrlen;
    memcpy (self->addr->domain.addr, addr, addrlen);
    memcpy (self->addr->domain.addr + addrlen, &nport, 2);

    return 0;
}

int
hev_socks5_client_tcp_construct_ip (HevSocks5ClientTCP *self,
                                    struct sockaddr *addr)
{
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_CLIENT_TYPE_TCP);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 client tcp construct ip", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_CLIENT_TCP_TYPE;

    self->addr = hev_malloc (19);
    if (!self->addr)
        return -1;

    res = hev_socks5_addr_from_sockaddr (self->addr, addr);
    if (res <= 0) {
        hev_free (self->addr);
        return -1;
    }

    return 0;
}

static void
hev_socks5_client_tcp_destruct (HevObject *base)
{
    HevSocks5ClientTCP *self = HEV_SOCKS5_CLIENT_TCP (base);

    LOG_D ("%p socks5 client tcp destruct", self);

    if (self->addr)
        hev_free (self->addr);

    HEV_SOCKS5_CLIENT_TYPE->finalizer (base);
}

HevObjectClass *
hev_socks5_client_tcp_class (void)
{
    static HevSocks5ClientTCPClass klass;
    HevSocks5ClientTCPClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        HevSocks5ClientClass *ckptr;

        memcpy (kptr, HEV_SOCKS5_CLIENT_TYPE, sizeof (HevSocks5ClientClass));

        okptr->name = "HevSocks5ClientTCP";
        okptr->finalizer = hev_socks5_client_tcp_destruct;

        ckptr = HEV_SOCKS5_CLIENT_CLASS (kptr);
        ckptr->get_upstream_addr = hev_socks5_client_tcp_get_upstream_addr;
    }

    return okptr;
}
