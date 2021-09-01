/*
 ============================================================================
 Name        : hev-socks5-client-tcp.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Client TCP
 ============================================================================
 */

#ifndef __HEV_SOCKS5_CLIENT_TCP_H__
#define __HEV_SOCKS5_CLIENT_TCP_H__

#include <netinet/in.h>

#include "hev-socks5-client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_CLIENT_TCP(p) ((HevSocks5ClientTCP *)p)
#define HEV_SOCKS5_CLIENT_TCP_CLASS(p) ((HevSocks5ClientTCPClass *)p)

typedef struct _HevSocks5ClientTCP HevSocks5ClientTCP;
typedef struct _HevSocks5ClientTCPClass HevSocks5ClientTCPClass;

struct _HevSocks5ClientTCP
{
    HevSocks5Client base;

    HevSocks5Addr *addr;
};

struct _HevSocks5ClientTCPClass
{
    HevSocks5ClientClass base;
};

HevSocks5Class *hev_socks5_client_tcp_get_class (void);

int hev_socks5_client_tcp_construct (HevSocks5ClientTCP *self);

HevSocks5ClientTCP *hev_socks5_client_tcp_new (const char *addr, int port);
HevSocks5ClientTCP *hev_socks5_client_tcp_new_ip (struct sockaddr *addr);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_CLIENT_TCP_H__ */
