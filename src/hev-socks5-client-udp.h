/*
 ============================================================================
 Name        : hev-socks5-client-udp.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Client UDP
 ============================================================================
 */

#ifndef __HEV_SOCKS5_CLIENT_UDP_H__
#define __HEV_SOCKS5_CLIENT_UDP_H__

#include "hev-socks5-client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_CLIENT_UDP(p) ((HevSocks5ClientUDP *)p)
#define HEV_SOCKS5_CLIENT_UDP_CLASS(p) ((HevSocks5ClientUDPClass *)p)
#define HEV_SOCKS5_CLIENT_UDP_TYPE (hev_socks5_client_udp_class ())

typedef struct _HevSocks5ClientUDP HevSocks5ClientUDP;
typedef struct _HevSocks5ClientUDPClass HevSocks5ClientUDPClass;

struct _HevSocks5ClientUDP
{
    HevSocks5Client base;
};

struct _HevSocks5ClientUDPClass
{
    HevSocks5ClientClass base;
};

HevObjectClass *hev_socks5_client_udp_class (void);

int hev_socks5_client_udp_construct (HevSocks5ClientUDP *self);

HevSocks5ClientUDP *hev_socks5_client_udp_new (void);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_CLIENT_UDP_H__ */
