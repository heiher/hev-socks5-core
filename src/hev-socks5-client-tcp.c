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

static HevSocks5Addr *
hev_socks5_client_tcp_get_upstream_addr (HevSocks5Client *base)
{
    HevSocks5ClientTCP *self = HEV_SOCKS5_CLIENT_TCP (base);
    HevSocks5Addr *addr;

    addr = self->addr;
    self->addr = NULL;

    return addr;
}

static HevSocks5ClientTCPClass _klass = {
    {
        {
            .name = "HevSocks5ClientTCP",
            .finalizer = hev_socks5_client_tcp_destruct,
        },

        .get_upstream_addr = hev_socks5_client_tcp_get_upstream_addr,
    },
};

int
hev_socks5_client_tcp_construct (HevSocks5ClientTCP *self)
{
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_CLIENT_TYPE_TCP);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 client tcp construct", self);

    HEV_SOCKS5 (self)->klass = HEV_SOCKS5_CLASS (&_klass);

    return 0;
}

void
hev_socks5_client_tcp_destruct (HevSocks5 *base)
{
    HevSocks5ClientTCP *self = HEV_SOCKS5_CLIENT_TCP (base);

    LOG_D ("%p socks5 client tcp destruct", self);

    if (self->addr)
        hev_free (self->addr);

    hev_socks5_client_destruct (base);
}

static HevSocks5ClientTCP *
hev_socks5_client_tcp_new_internal (void)
{
    HevSocks5ClientTCP *self;
    int res;

    self = hev_malloc0 (sizeof (HevSocks5ClientTCP));
    if (!self)
        return NULL;

    LOG_D ("%p socks5 client tcp new internal", self);

    res = hev_socks5_client_tcp_construct (self);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    return self;
}

HevSocks5ClientTCP *
hev_socks5_client_tcp_new (const char *addr, int port)
{
    HevSocks5ClientTCP *self;
    HevSocks5Addr *s5addr;
    int addrlen;
    int nport;

    addrlen = strlen (addr);
    s5addr = hev_malloc (4 + addrlen);
    if (!s5addr)
        return NULL;

    nport = htons (port);
    s5addr->atype = HEV_SOCKS5_ADDR_TYPE_NAME;
    s5addr->domain.len = addrlen;
    memcpy (s5addr->domain.addr, addr, addrlen);
    memcpy (s5addr->domain.addr + addrlen, &nport, 2);

    self = hev_socks5_client_tcp_new_internal ();
    if (!self) {
        hev_free (s5addr);
        return NULL;
    }

    LOG_I ("%p socks5 client tcp new [%s]:%d", self, addr, port);

    self->addr = s5addr;

    return self;
}

HevSocks5ClientTCP *
hev_socks5_client_tcp_new_ip (struct sockaddr *addr)
{
    HevSocks5ClientTCP *self;
    HevSocks5Addr *s5addr;
    int res;

    s5addr = hev_malloc (19);
    if (!s5addr)
        return NULL;

    res = hev_socks5_addr_from_sockaddr (s5addr, addr);
    if (res <= 0) {
        hev_free (s5addr);
        return NULL;
    }

    self = hev_socks5_client_tcp_new_internal ();
    if (!self) {
        hev_free (s5addr);
        return NULL;
    }

    if (LOG_ON ()) {
        char buf[128];
        const char *str;

        str = hev_socks5_addr_to_string (s5addr, buf, sizeof (buf));
        LOG_I ("%p socks5 client tcp new ip %s", self, str);
    }

    self->addr = s5addr;

    return self;
}
