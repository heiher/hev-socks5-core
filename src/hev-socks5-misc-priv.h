/*
 ============================================================================
 Name        : hev-socks5-misc-priv.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Misc Private
 ============================================================================
 */

#ifndef __HEV_SOCKS5_MISC_PRIV_H__
#define __HEV_SOCKS5_MISC_PRIV_H__

#include <netinet/in.h>

#include <hev-task.h>
#include <hev-task-io.h>

#include "hev-socks5-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

int hev_socks5_task_io_yielder (HevTaskYieldType type, void *data);

int hev_socks5_open_socket (int type);
int hev_socks5_resolve_addr (const char *addr, int port,
                             struct sockaddr_in6 *saddr);

int hev_socks5_addr_to_sockaddr (HevSocks5Addr *addr, struct sockaddr *saddr);
int hev_socks5_addr_from_sockaddr (HevSocks5Addr *addr, struct sockaddr *saddr);
const char *hev_socks5_addr_to_string (HevSocks5Addr *addr, char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_MISC_PRIV_H__ */
