/*
 ============================================================================
 Name        : hev-object.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Object
 ============================================================================
 */

#include "hev-object.h"

HevObject *
hev_object_ref (HevObject *self)
{
    self->ref_count++;

    return self;
}

void
hev_object_unref (HevObject *self)
{
    HevObjectClass *kptr = HEV_OBJECT_GET_CLASS (self);

    self->ref_count--;

    if (self->ref_count)
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
