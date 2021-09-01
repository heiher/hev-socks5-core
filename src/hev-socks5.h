/*
 ============================================================================
 Name        : hev-socks5.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5
 ============================================================================
 */

#ifndef __HEV_SOCKS5_H__
#define __HEV_SOCKS5_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5(p) ((HevSocks5 *)p)
#define HEV_SOCKS5_CLASS(p) ((HevSocks5Class *)p)
#define HEV_SOCKS5_GET_CLASS(p) ((void *)((HevSocks5 *)p)->klass)

typedef struct _HevSocks5 HevSocks5;
typedef struct _HevSocks5Class HevSocks5Class;

struct _HevSocks5
{
    HevSocks5Class *klass;

    int fd;
    int timeout;
    int ref_count;

    struct
    {
        const char *user;
        const char *pass;
    } auth;
};

struct _HevSocks5Class
{
    const char *name;

    void (*finalizer) (HevSocks5 *self);
    int (*binder) (HevSocks5 *self, int sock);
};

HevSocks5Class *hev_socks5_get_class (void);

int hev_socks5_construct (HevSocks5 *self);

HevSocks5 *hev_socks5_ref (HevSocks5 *self);
void hev_socks5_unref (HevSocks5 *self);

int hev_socks5_get_timeout (HevSocks5 *self);
void hev_socks5_set_timeout (HevSocks5 *self, int timeout);

void hev_socks5_set_auth_user_pass (HevSocks5 *self, const char *user,
                                    const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_H__ */
