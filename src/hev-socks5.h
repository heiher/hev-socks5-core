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

#include "hev-object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5(p) ((HevSocks5 *)p)
#define HEV_SOCKS5_CLASS(p) ((HevSocks5Class *)p)
#define HEV_SOCKS5_TYPE (hev_socks5_class ())

typedef struct _HevSocks5 HevSocks5;
typedef struct _HevSocks5Class HevSocks5Class;

struct _HevSocks5
{
    HevObject base;

    int fd;
    int timeout;

    struct
    {
        const char *user;
        const char *pass;
    } auth;
};

struct _HevSocks5Class
{
    HevObjectClass base;

    int (*binder) (HevSocks5 *self, int sock);
};

HevObjectClass *hev_socks5_class (void);

int hev_socks5_construct (HevSocks5 *self);

int hev_socks5_get_timeout (HevSocks5 *self);
void hev_socks5_set_timeout (HevSocks5 *self, int timeout);

void hev_socks5_set_auth_user_pass (HevSocks5 *self, const char *user,
                                    const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_H__ */
