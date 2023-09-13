/*
 ============================================================================
 Name        : hev-object.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Object
 ============================================================================
 */

#include <stdatomic.h>

#include "hev-object.h"

int
hev_object_get_atomic (HevObject *self)
{
    return self->ref_count >> 31;
}

void
hev_object_set_atomic (HevObject *self, int atomic)
{
    if (atomic)
        self->ref_count |= 1U << 31;
    else
        self->ref_count &= ~(1U << 31);
}

HevObject *
hev_object_ref (HevObject *self)
{
    if (hev_object_get_atomic (self))
        atomic_fetch_add_explicit (&self->ref_count, 1, memory_order_relaxed);
    else
        self->ref_count++;

    return self;
}

void
hev_object_unref (HevObject *self)
{
    HevObjectClass *kptr = HEV_OBJECT_GET_CLASS (self);
    unsigned int ref_count;

    if (hev_object_get_atomic (self)) {
        ref_count = atomic_fetch_sub_explicit (&self->ref_count, 1,
                                               memory_order_relaxed);
        ref_count &= ~(1U << 31);
    } else {
        ref_count = self->ref_count--;
    }

    if (ref_count > 1)
        return;

    if (kptr->finalizer)
        kptr->finalizer (self);
}

int
hev_object_construct (HevObject *self)
{
    self->klass = HEV_OBJECT_TYPE;

    self->ref_count = 1;

    return 0;
}

static void
hev_object_destruct (HevObject *self)
{
}

HevObjectClass *
hev_object_class (void)
{
    static HevObjectClass klass;
    HevObjectClass *kptr = &klass;

    if (!kptr->name) {
        kptr->name = "HevObject";
        kptr->finalizer = hev_object_destruct;
    }

    return kptr;
}
