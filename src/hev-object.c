/*
 ============================================================================
 Name        : hev-object.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2021 - 2023 hev
 Description : Object
 ============================================================================
 */

#include <stdatomic.h>

#include "hev-object.h"

int
hev_object_get_atomic (HevObject *self)
{
    return self->atomic;
}

void
hev_object_set_atomic (HevObject *self, int atomic)
{
    self->atomic = atomic;
}

HevObject *
hev_object_ref (HevObject *self)
{
    atomic_uint *rcp = (atomic_uint *)&self->ref_count;

    if (self->atomic)
        atomic_fetch_add_explicit (rcp, 1, memory_order_relaxed);
    else
        self->ref_count++;

    return self;
}

void
hev_object_unref (HevObject *self)
{
    HevObjectClass *kptr = HEV_OBJECT_GET_CLASS (self);
    atomic_uint *rcp = (atomic_uint *)&self->ref_count;
    unsigned int rc;

    if (self->atomic)
        rc = atomic_fetch_sub_explicit (rcp, 1, memory_order_relaxed);
    else
        rc = self->ref_count--;

    if (rc > 1)
        return;

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
