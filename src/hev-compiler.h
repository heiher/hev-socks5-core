/*
 ============================================================================
 Name        : hev-compiler.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Compiler
 ============================================================================
 */

#ifndef __HEV_COMPILER_H__
#define __HEV_COMPILER_H__

#include <stddef.h>

#if defined(_MSC_VER)
#include <stdint.h>
#include <string.h>
#include <intrin.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
/* MSVC C does not support GNU typeof; these helpers operate on integers.
 * Cast an object pointer to uintptr_t before passing it here. */
#define ALIGN_UP(addr, align) \
    (((uintptr_t)(addr) + (uintptr_t)(align) - 1) & \
     ~((uintptr_t)(align) - 1))
#define ALIGN_DOWN(addr, align) \
    ((uintptr_t)(addr) & ~((uintptr_t)(align) - 1))
#else
#define ALIGN_UP(addr, align) \
    ((addr + (typeof (addr))align - 1) & ~((typeof (addr))align - 1))

#define ALIGN_DOWN(addr, align) ((addr) & ~((typeof (addr))align - 1))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))
#endif

#ifndef EXPORT_SYMBOL
#if defined(_MSC_VER)
#if defined(HEV_SOCKS5_TUNNEL_BUILD)
#define EXPORT_SYMBOL __declspec (dllexport)
#else
#define EXPORT_SYMBOL
#endif
#else
#define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
#endif
#endif

#if defined(_MSC_VER)
/* Compiler barrier; equivalent in intent to the GNU empty asm barrier. */
#define barrier() _ReadWriteBarrier ()
#else
#define barrier() __asm__ __volatile__ ("" : : : "memory")
#endif

static inline void
__read_once_size (void *dst, const volatile void *src, int size)
{
    switch (size) {
    case sizeof (char):
        *(char *)dst = *(volatile char *)src;
        break;
    case sizeof (short):
        *(short *)dst = *(volatile short *)src;
        break;
    case sizeof (int):
        *(int *)dst = *(volatile int *)src;
        break;
    default:
        barrier ();
#if defined(_MSC_VER)
        memcpy ((void *)dst, (const void *)src, size);
#else
        __builtin_memcpy ((void *)dst, (const void *)src, size);
#endif
        barrier ();
    }
}

static inline void
__write_once_size (volatile void *dst, const void *src, int size)
{
    switch (size) {
    case sizeof (char):
        *(volatile char *)dst = *(char *)src;
        break;
    case sizeof (short):
        *(volatile short *)dst = *(short *)src;
        break;
    case sizeof (int):
        *(volatile int *)dst = *(int *)src;
        break;
    default:
        barrier ();
#if defined(_MSC_VER)
        memcpy ((void *)dst, (const void *)src, size);
#else
        __builtin_memcpy ((void *)dst, (const void *)src, size);
#endif
        barrier ();
    }
}

#if defined(_MSC_VER)
/*
 * MSVC C11 lacks GNU statement expressions and typeof.  The _Generic
 * implementation covers arithmetic scalar types and object pointers.
 * Pointer results use void * in C (object pointers are implicitly convertible);
 * these are compiler single-access primitives, not atomic operations or
 * inter-thread synchronization.
 */
#ifdef __cplusplus
/* The C++ spelling works with both MSVC and clang-cl. */
#define READ_ONCE(x) (*reinterpret_cast<volatile decltype(x) *>(&(x)))
#define WRITE_ONCE(x, val) \
    (*reinterpret_cast<volatile decltype(x) *>(&(x)) = (val))
#else
#define HEV_COMPILER_DEFINE_ONCE(type, suffix)                               \
    static inline type hev__read_once_##suffix (const volatile void *src)   \
    { return *(const volatile type *)src; }                                 \
    static inline type hev__write_once_##suffix (volatile void *dst,        \
                                                   type value)               \
    { *(volatile type *)dst = value; return value; }

HEV_COMPILER_DEFINE_ONCE (_Bool, bool)
HEV_COMPILER_DEFINE_ONCE (char, char)
HEV_COMPILER_DEFINE_ONCE (signed char, schar)
HEV_COMPILER_DEFINE_ONCE (unsigned char, uchar)
HEV_COMPILER_DEFINE_ONCE (short, short)
HEV_COMPILER_DEFINE_ONCE (unsigned short, ushort)
HEV_COMPILER_DEFINE_ONCE (int, int)
HEV_COMPILER_DEFINE_ONCE (unsigned int, uint)
HEV_COMPILER_DEFINE_ONCE (long, long)
HEV_COMPILER_DEFINE_ONCE (unsigned long, ulong)
HEV_COMPILER_DEFINE_ONCE (long long, llong)
HEV_COMPILER_DEFINE_ONCE (unsigned long long, ullong)
HEV_COMPILER_DEFINE_ONCE (float, float)
HEV_COMPILER_DEFINE_ONCE (double, double)
HEV_COMPILER_DEFINE_ONCE (long double, ldouble)
#undef HEV_COMPILER_DEFINE_ONCE

static inline void *
hev__read_once_ptr (const volatile void *src)
{
    void *value;
    barrier ();
    memcpy (&value, (const void *)src, sizeof (value));
    barrier ();
    return value;
}

static inline void *
hev__write_once_ptr (volatile void *dst, void *value)
{
    barrier ();
    memcpy ((void *)dst, &value, sizeof (value));
    barrier ();
    return value;
}

#define HEV_ONCE_SELECT(prefix, x)                                            \
    _Generic ((x),                                                          \
              _Bool: prefix##_bool,                                         \
              char: prefix##_char,                                          \
              signed char: prefix##_schar,                                  \
              unsigned char: prefix##_uchar,                                \
              short: prefix##_short,                                        \
              unsigned short: prefix##_ushort,                              \
              int: prefix##_int,                                            \
              unsigned int: prefix##_uint,                                  \
              long: prefix##_long,                                          \
              unsigned long: prefix##_ulong,                                \
              long long: prefix##_llong,                                    \
              unsigned long long: prefix##_ullong,                          \
              float: prefix##_float,                                        \
              double: prefix##_double,                                      \
              long double: prefix##_ldouble,                                \
              default: prefix##_ptr)

#define READ_ONCE(x) (HEV_ONCE_SELECT (hev__read_once, x) (&(x)))
#define WRITE_ONCE(x, val) \
    (HEV_ONCE_SELECT (hev__write_once, x) (&(x), (val)))
#endif /* __cplusplus */
#else /* GNU-compatible compilers: keep the upstream implementation. */
#define READ_ONCE(x)                                  \
    ({                                                \
        union                                         \
        {                                             \
            typeof (x) __val;                         \
            char __c[1];                              \
        } __u;                                        \
        __read_once_size (__u.__c, &(x), sizeof (x)); \
        __u.__val;                                    \
    })

#define WRITE_ONCE(x, val)                             \
    ({                                                 \
        union                                          \
        {                                              \
            typeof (x) __val;                          \
            char __c[1];                               \
        } __u = { .__val = (typeof (x))(val) };        \
        __write_once_size (&(x), __u.__c, sizeof (x)); \
        __u.__val;                                     \
    })

#endif /* _MSC_VER */

#ifndef container_of
#if defined(_MSC_VER)
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof (type, member)))
#else
#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *)(ptr);                 \
        ((type *)(__mptr - offsetof (type, member))); \
    })
#endif /* _MSC_VER */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __HEV_COMPILER_H__ */
