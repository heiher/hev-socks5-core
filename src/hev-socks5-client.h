/*
 ============================================================================
 Name        : hev-socks5-client.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Client
 ============================================================================
 */

#ifndef __HEV_SOCKS5_CLIENT_H__
#define __HEV_SOCKS5_CLIENT_H__

#include "hev-socks5.h"
#include "hev-socks5-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_CLIENT(p) ((HevSocks5Client *)p)
#define HEV_SOCKS5_CLIENT_CLASS(p) ((HevSocks5ClientClass *)p)

typedef struct _HevSocks5Client HevSocks5Client;
typedef struct _HevSocks5ClientClass HevSocks5ClientClass;
typedef enum _HevSocks5ClientType HevSocks5ClientType;

enum _HevSocks5ClientType
{
    HEV_SOCKS5_CLIENT_TYPE_TCP,
    HEV_SOCKS5_CLIENT_TYPE_UDP,
};

struct _HevSocks5Client
{
    HevSocks5 base;

    HevSocks5ClientType type;
};

struct _HevSocks5ClientClass
{
    HevSocks5Class base;

    HevSocks5Addr *(*get_upstream_addr) (HevSocks5Client *self);
};

HevSocks5Class *hev_socks5_client_get_class (void);

int hev_socks5_client_construct (HevSocks5Client *self,
                                 HevSocks5ClientType type);

int hev_socks5_client_connect (HevSocks5Client *self, const char *addr,
                               int port);

int hev_socks5_client_connect_fd (HevSocks5Client *self, int fd);

int hev_socks5_client_handshake (HevSocks5Client *self);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_CLIENT_H__ */
