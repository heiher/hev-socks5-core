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

    LOG_I ("%p socks5 client udp new", self);

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

    LOG_D ("%p socks5 client udp construct", self);

    HEV_SOCKS5 (self)->klass = hev_socks5_client_udp_get_class ();

    return 0;
}

static void
hev_socks5_client_udp_destruct (HevSocks5 *base)
{
    HevSocks5ClientUDP *self = HEV_SOCKS5_CLIENT_UDP (base);

    LOG_D ("%p socks5 client udp destruct", self);

    hev_socks5_client_get_class ()->finalizer (base);
}

HevSocks5Class *
hev_socks5_client_udp_get_class (void)
{
    static HevSocks5ClientUDPClass klass;
    HevSocks5ClientUDPClass *kptr = &klass;

    if (!HEV_SOCKS5_CLASS (kptr)->name) {
        HevSocks5ClientClass *ckptr;
        HevSocks5Class *skptr;
        void *ptr;

        ptr = hev_socks5_client_get_class ();
        memcpy (kptr, ptr, sizeof (HevSocks5ClientClass));

        skptr = HEV_SOCKS5_CLASS (kptr);
        skptr->name = "HevSocks5ClientUDP";
        skptr->finalizer = hev_socks5_client_udp_destruct;

        ckptr = HEV_SOCKS5_CLIENT_CLASS (kptr);
        ckptr->get_upstream_addr = hev_socks5_client_udp_get_upstream_addr;
    }

    return HEV_SOCKS5_CLASS (kptr);
}
