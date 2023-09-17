/*
 ============================================================================
 Name        : hev-object.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2021 - 2023 hev
 Description : Object
 ============================================================================
 */

#ifndef __HEV_OBJECT_H__
#define __HEV_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_OBJECT(p) ((HevObject *)p)
#define HEV_OBJECT_CLASS(p) ((HevObjectClass *)p)
#define HEV_OBJECT_GET_CLASS(p) ((void *)((HevObject *)p)->klass)
#define HEV_OBJECT_GET_IFACE(p, i) (((HevObject *)p)->klass->iface (p, i))
#define HEV_OBJECT_TYPE (hev_object_class ())

typedef struct _HevObject HevObject;
typedef struct _HevObjectClass HevObjectClass;

struct _HevObject
{
    HevObjectClass *klass;

    unsigned int atomic;
    unsigned int ref_count;
};

struct _HevObjectClass
{
    const char *name;

    void (*finalizer) (HevObject *self);
    void *(*iface) (HevObject *self, void *type);
};

HevObjectClass *hev_object_class (void);

int hev_object_construct (HevObject *self);

int hev_object_get_atomic (HevObject *self);
void hev_object_set_atomic (HevObject *self, int atomic);

HevObject *hev_object_ref (HevObject *self);
void hev_object_unref (HevObject *self);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_OBJECT_H__ */
