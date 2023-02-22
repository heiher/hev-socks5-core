/*
 ============================================================================
 Name        : hev-socks5-server.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 - 2023 hev
 Description : Socks5 Server
 ============================================================================
 */

#ifndef __HEV_SOCKS5_SERVER_H__
#define __HEV_SOCKS5_SERVER_H__

#include "hev-socks5.h"
#include "hev-socks5-tcp.h"
#include "hev-socks5-udp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_SERVER(p) ((HevSocks5Server *)p)
#define HEV_SOCKS5_SERVER_CLASS(p) ((HevSocks5ServerClass *)p)
#define HEV_SOCKS5_SERVER_TYPE (hev_socks5_server_class ())

typedef struct _HevSocks5Server HevSocks5Server;
typedef struct _HevSocks5ServerClass HevSocks5ServerClass;

struct _HevSocks5Server
{
    HevSocks5 base;

    int fds[2];
    int timeout;
};

struct _HevSocks5ServerClass
{
    HevSocks5Class base;

    HevSocks5TCPIface tcp;
    HevSocks5UDPIface udp;
};

HevObjectClass *hev_socks5_server_class (void);

int hev_socks5_server_construct (HevSocks5Server *self, int fd);

HevSocks5Server *hev_socks5_server_new (int fd);

void hev_socks5_server_set_connect_timeout (HevSocks5Server *self, int timeout);

int hev_socks5_server_run (HevSocks5Server *self);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_SERVER_H__ */
