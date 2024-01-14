/*
 ============================================================================
 Name        : hev-socks5.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 - 2023 hev
 Description : Socks5
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-task-dns.h>
#include <hev-memory-allocator.h>

#include "hev-socks5-logger-priv.h"

#include "hev-socks5.h"

int
hev_socks5_get_timeout (HevSocks5 *self)
{
    return self->timeout;
}

void
hev_socks5_set_timeout (HevSocks5 *self, int timeout)
{
    self->timeout = timeout;
}

HevSocks5DomainAddrType
hev_socks5_get_domain_addr_type (HevSocks5 *self)
{
    return self->domain_addr_type;
}

void
hev_socks5_set_domain_addr_type (HevSocks5 *self, HevSocks5DomainAddrType type)
{
    self->domain_addr_type = type;
}

static int
hev_socks5_bind (HevSocks5 *self, int sock, const struct sockaddr *dest)
{
    return 0;
}

int
hev_socks5_construct (HevSocks5 *self, HevSocks5Type type)
{
    int res;

    res = hev_object_construct (&self->base);
    if (res < 0)
        return res;

    LOG_D ("%p socks5 construct", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_TYPE;

    self->fd = -1;
    self->timeout = -1;
    self->type = type;
    self->domain_addr_type = HEV_SOCKS5_DOMAIN_ADDR_TYPE_UNSPEC;

    return 0;
}

static void
hev_socks5_destruct (HevObject *base)
{
    HevSocks5 *self = HEV_SOCKS5 (base);

    LOG_D ("%p socks5 destruct", self);

    if (self->fd >= 0)
        close (self->fd);

    HEV_OBJECT_TYPE->destruct (base);
    hev_free (base);
}

HevObjectClass *
hev_socks5_class (void)
{
    static HevSocks5Class klass;
    HevSocks5Class *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        memcpy (kptr, HEV_OBJECT_TYPE, sizeof (HevObjectClass));

        okptr->name = "HevSocks5";
        okptr->destruct = hev_socks5_destruct;

        kptr->binder = hev_socks5_bind;
    }

    return okptr;
}
