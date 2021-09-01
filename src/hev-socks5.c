/*
 ============================================================================
 Name        : hev-socks5.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
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

HevSocks5 *
hev_socks5_ref (HevSocks5 *self)
{
    self->ref_count++;

    LOG_D ("%p socks5 ref++ %u", self, self->ref_count);

    return self;
}

void
hev_socks5_unref (HevSocks5 *self)
{
    HevSocks5Class *klass = HEV_SOCKS5_GET_CLASS (self);

    self->ref_count--;

    LOG_D ("%p socks5 ref-- %u", self, self->ref_count);

    if (self->ref_count > 0)
        return;

    if (klass->finalizer)
        klass->finalizer (self);
}

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

void
hev_socks5_set_auth_user_pass (HevSocks5 *self, const char *user,
                               const char *pass)
{
    self->auth.user = user;
    self->auth.pass = pass;
}

static int
hev_socks5_bind (HevSocks5 *self, int sock)
{
    return 0;
}

int
hev_socks5_construct (HevSocks5 *self)
{
    LOG_D ("%p socks5 construct", self);

    HEV_SOCKS5 (self)->klass = hev_socks5_get_class ();

    self->fd = -1;
    self->timeout = -1;
    self->ref_count = 1;

    return 0;
}

static void
hev_socks5_destruct (HevSocks5 *self)
{
    LOG_D ("%p socks5 destruct", self);

    if (self->fd >= 0)
        close (self->fd);
    hev_free (self);
}

HevSocks5Class *
hev_socks5_get_class (void)
{
    static HevSocks5Class klass;
    HevSocks5Class *kptr = &klass;

    if (!kptr->name) {
        kptr->name = "HevSocks5";
        kptr->finalizer = hev_socks5_destruct;
        kptr->binder = hev_socks5_bind;
    }

    return kptr;
}
