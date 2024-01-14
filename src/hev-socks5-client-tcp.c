/*
 ============================================================================
 Name        : hev-socks5-client-tcp.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 - 2023 hev
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

    LOG_D ("%p socks5 client tcp new", self);

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

    LOG_D ("%p socks5 client tcp new ip", self);

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

static int
hev_socks5_client_tcp_set_upstream_addr (HevSocks5Client *base,
                                         HevSocks5Addr *addr)
{
    return 0;
}

int
hev_socks5_client_tcp_construct (HevSocks5ClientTCP *self, const char *addr,
                                 int port)
{
    int addrlen;
    int nport;
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_TYPE_TCP);
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

    LOG_I ("%p socks5 client tcp -> [%s]:%d", self, addr, port);

    return 0;
}

int
hev_socks5_client_tcp_construct_ip (HevSocks5ClientTCP *self,
                                    struct sockaddr *addr)
{
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_TYPE_TCP);
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

    if (LOG_ON ()) {
        char buf[128];
        const char *str;

        str = hev_socks5_addr_to_string (self->addr, buf, sizeof (buf));
        LOG_I ("%p socks5 client tcp -> %s", self, str);
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

    HEV_SOCKS5_CLIENT_TYPE->destruct (base);
}

static void *
hev_socks5_client_tcp_iface (HevObject *base, void *type)
{
    HevSocks5ClientTCPClass *klass = HEV_OBJECT_GET_CLASS (base);

    return &klass->tcp;
}

HevObjectClass *
hev_socks5_client_tcp_class (void)
{
    static HevSocks5ClientTCPClass klass;
    HevSocks5ClientTCPClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        HevSocks5ClientClass *ckptr;
        HevSocks5TCPIface *tiptr;

        memcpy (kptr, HEV_SOCKS5_CLIENT_TYPE, sizeof (HevSocks5ClientClass));

        okptr->name = "HevSocks5ClientTCP";
        okptr->destruct = hev_socks5_client_tcp_destruct;
        okptr->iface = hev_socks5_client_tcp_iface;

        ckptr = HEV_SOCKS5_CLIENT_CLASS (kptr);
        ckptr->get_upstream_addr = hev_socks5_client_tcp_get_upstream_addr;
        ckptr->set_upstream_addr = hev_socks5_client_tcp_set_upstream_addr;

        tiptr = &kptr->tcp;
        memcpy (tiptr, HEV_SOCKS5_TCP_TYPE, sizeof (HevSocks5TCPIface));
    }

    return okptr;
}
