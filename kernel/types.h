#pragma once

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

#if defined(__x86_64__) || defined(_M_X64)
typedef __int128          int128_t;
typedef unsigned __int128 uint128_t;
#endif

#if defined(__x86_64__) || defined(_M_X64)
typedef unsigned long long size_t;
typedef signed long long   ssize_t;
typedef unsigned long long uintptr_t;
typedef signed long long   intptr_t;
typedef signed long long   ptrdiff_t;
#elif defined(__i386__) || defined(_M_IX86)
typedef unsigned int       size_t;
typedef signed int         ssize_t;
typedef unsigned int       uintptr_t;
typedef signed int         intptr_t;
typedef signed int         ptrdiff_t;
#endif

#ifndef __cplusplus
typedef _Bool bool;
#define true  1
#define false 0
#endif

#define NULL ((void *)0)

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
