/*
 ============================================================================
 Name        : hev-socks5-tcp.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 TCP
 ============================================================================
 */

#ifndef __HEV_SOCKS5_TCP_H__
#define __HEV_SOCKS5_TCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_TCP(p) ((HevSocks5TCP *)p)

typedef struct _HevSocks5TCP HevSocks5TCP;

int hev_socks5_tcp_splice (HevSocks5TCP *self, int fd);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_TCP_H__ */
