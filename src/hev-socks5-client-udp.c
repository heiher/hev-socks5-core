/*
 ============================================================================
 Name        : hev-socks5-client-udp.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Client UDP
 ============================================================================
 */

#include <string.h>

#include <hev-memory-allocator.h>

#include "hev-socks5-logger-priv.h"

#include "hev-socks5-client-udp.h"

static HevSocks5Addr *
hev_socks5_client_udp_get_upstream_addr (HevSocks5Client *base)
{
    HevSocks5Addr *addr;

    addr = hev_malloc0 (7);
    if (addr)
        addr->atype = HEV_SOCKS5_ADDR_TYPE_IPV4;

    return addr;
}

static HevSocks5ClientUDPClass _klass = {
    {
        {
            .name = "HevSocks5ClientUDP",
            .finalizer = hev_socks5_client_udp_destruct,
        },

        .get_upstream_addr = hev_socks5_client_udp_get_upstream_addr,
    },
};

int
hev_socks5_client_udp_construct (HevSocks5ClientUDP *self)
{
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_CLIENT_TYPE_UDP);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 client udp construct", self);

    HEV_SOCKS5 (self)->klass = HEV_SOCKS5_CLASS (&_klass);

    return 0;
}

void
hev_socks5_client_udp_destruct (HevSocks5 *base)
{
    HevSocks5ClientUDP *self = HEV_SOCKS5_CLIENT_UDP (base);

    LOG_D ("%p socks5 client udp destruct", self);

    hev_socks5_client_destruct (base);
}

HevSocks5ClientUDP *
hev_socks5_client_udp_new (void)
{
    HevSocks5ClientUDP *self;
    int res;

    self = hev_malloc0 (sizeof (HevSocks5ClientUDP));
    if (!self)
        return NULL;

    res = hev_socks5_client_udp_construct (self);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    LOG_I ("%p socks5 client udp new", self);

    return self;
}
