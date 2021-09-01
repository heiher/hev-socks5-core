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

    LOG_D ("%p socks5 client udp new", self);

    return self;
}

static HevSocks5Addr *
hev_socks5_client_udp_get_upstream_addr (HevSocks5Client *base)
{
    HevSocks5Addr *addr;

    addr = hev_malloc0 (7);
    if (addr)
        addr->atype = HEV_SOCKS5_ADDR_TYPE_IPV4;

    return addr;
}

int
hev_socks5_client_udp_construct (HevSocks5ClientUDP *self)
{
    int res;

    res = hev_socks5_client_construct (&self->base, HEV_SOCKS5_CLIENT_TYPE_UDP);
    if (res < 0)
        return res;

    LOG_I ("%p socks5 client udp construct", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_CLIENT_UDP_TYPE;

    return 0;
}

static void
hev_socks5_client_udp_destruct (HevObject *base)
{
    HevSocks5ClientUDP *self = HEV_SOCKS5_CLIENT_UDP (base);

    LOG_D ("%p socks5 client udp destruct", self);

    HEV_SOCKS5_CLIENT_TYPE->finalizer (base);
}

HevObjectClass *
hev_socks5_client_udp_class (void)
{
    static HevSocks5ClientUDPClass klass;
    HevSocks5ClientUDPClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        HevSocks5ClientClass *ckptr;

        memcpy (kptr, HEV_SOCKS5_CLIENT_TYPE, sizeof (HevSocks5ClientClass));

        okptr->name = "HevSocks5ClientUDP";
        okptr->finalizer = hev_socks5_client_udp_destruct;

        ckptr = HEV_SOCKS5_CLIENT_CLASS (kptr);
        ckptr->get_upstream_addr = hev_socks5_client_udp_get_upstream_addr;
    }

    return okptr;
}
