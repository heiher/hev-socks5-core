/*
 ============================================================================
 Name        : hev-socks5-authenticator.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2023 hev
 Description : Socks5 Authenticator
 ============================================================================
 */

#ifndef __HEV_SOCKS5_AUTHENTICATOR_H__
#define __HEV_SOCKS5_AUTHENTICATOR_H__

#include "hev-object.h"
#include "hev-rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_AUTHENTICATOR(p) ((HevSocks5Authenticator *)p)
#define HEV_SOCKS5_AUTHENTICATOR_CLASS(p) ((HevSocks5AuthenticatorClass *)p)
#define HEV_SOCKS5_AUTHENTICATOR_TYPE (hev_socks5_authenticator_class ())

typedef struct _HevSocks5Authenticator HevSocks5Authenticator;
typedef struct _HevSocks5AuthenticatorClass HevSocks5AuthenticatorClass;
typedef enum _HevSocks5AuthenticatorType HevSocks5AuthenticatorType;

struct _HevSocks5Authenticator
{
    HevObject base;

    HevRBTree tree;
};

struct _HevSocks5AuthenticatorClass
{
    HevObjectClass base;
};

HevObjectClass *hev_socks5_authenticator_class (void);

int hev_socks5_authenticator_construct (HevSocks5Authenticator *self);

HevSocks5Authenticator *hev_socks5_authenticator_new (void);

int hev_socks5_authenticator_add (HevSocks5Authenticator *self,
                                  const char *user, const char *pass);
int hev_socks5_authenticator_del (HevSocks5Authenticator *self,
                                  const char *user);
int hev_socks5_authenticator_cmp (HevSocks5Authenticator *self,
                                  const char *user, const char *pass);
void hev_socks5_authenticator_clear (HevSocks5Authenticator *self);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_AUTHENTICATOR_H__ */
