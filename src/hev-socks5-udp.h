/*
 ============================================================================
 Name        : hev-socks5-udp.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 UDP
 ============================================================================
 */

#ifndef __HEV_SOCKS5_UDP_H__
#define __HEV_SOCKS5_UDP_H__

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEV_SOCKS5_UDP(p) ((HevSocks5UDP *)p)
#define HEV_SOCKS5_UDP_IFACE(p) ((HevSocks5UDPIface *)p)
#define HEV_SOCKS5_UDP_TYPE (hev_socks5_udp_iface ())

typedef void HevSocks5UDP;
typedef struct _HevSocks5UDPIface HevSocks5UDPIface;

struct _HevSocks5UDPIface
{
    int (*splicer) (HevSocks5UDP *self, int fd);
};

void *hev_socks5_udp_iface (void);

int hev_socks5_udp_sendto (HevSocks5UDP *self, const void *buf, size_t len,
                           struct sockaddr *addr);

int hev_socks5_udp_recvfrom (HevSocks5UDP *self, void *buf, size_t len,
                             struct sockaddr *addr);

int hev_socks5_udp_splice (HevSocks5UDP *self, int fd);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_UDP_H__ */
