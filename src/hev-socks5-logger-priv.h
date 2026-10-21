/*
 ============================================================================
 Name        : hev-socks5-logger-priv.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2021 hev
 Description : Socks5 Logger Private
 ============================================================================
 */

#ifndef __HEV_SOCKS5_LOGGER_PRIV_H__
#define __HEV_SOCKS5_LOGGER_PRIV_H__

#include "hev-socks5-logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_D(...) hev_socks5_logger_log (HEV_SOCKS5_LOGGER_DEBUG, __VA_ARGS__)
#define LOG_I(...) hev_socks5_logger_log (HEV_SOCKS5_LOGGER_INFO, __VA_ARGS__)
#define LOG_W(...) hev_socks5_logger_log (HEV_SOCKS5_LOGGER_WARN, __VA_ARGS__)
#define LOG_E(...) hev_socks5_logger_log (HEV_SOCKS5_LOGGER_ERROR, __VA_ARGS__)

#define LOG_ON() hev_socks5_logger_enabled (HEV_SOCKS5_LOGGER_UNSET)
#define LOG_ON_D() hev_socks5_logger_enabled (HEV_SOCKS5_LOGGER_DEBUG)
#define LOG_ON_I() hev_socks5_logger_enabled (HEV_SOCKS5_LOGGER_INFO)
#define LOG_ON_W() hev_socks5_logger_enabled (HEV_SOCKS5_LOGGER_WARN)
#define LOG_ON_E() hev_socks5_logger_enabled (HEV_SOCKS5_LOGGER_ERROR)

int hev_socks5_logger_enabled (HevSocks5LoggerLevel level);
void hev_socks5_logger_log (HevSocks5LoggerLevel level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __HEV_SOCKS5_LOGGER_PRIV_H__ */
