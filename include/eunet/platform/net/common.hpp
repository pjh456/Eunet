/*
 * ============================================================================
 *  File Name   : common.hpp
 *  Module      : platform/net
 *
 *  Description :
 *      网络模块通用的枚举和定义，如地址族 (IPv4/IPv6) 定义。
 *
 *  Third-Party Dependencies :
 *      None
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_PLATFORM_NET_COMMON
#define INCLUDE_EUNET_PLATFORM_NET_COMMON

namespace platform::net
{
    enum class AddressFamily
    {
        IPv4,
        IPv6,
        Any
    };

}

#endif // INCLUDE_EUNET_PLATFORM_NET_COMMON