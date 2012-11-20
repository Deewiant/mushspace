// File created: 2012-11-20 16:35:12

#ifndef MUSH_CONFIG_COMPILER_H
#define MUSH_CONFIG_COMPILER_H

#if defined(__GNUC__)
#define MUSH_PACKED_STRUCT     struct __attribute__((__packed__))
#define MUSH_PACKED_STRUCT_END
#else
#define MUSH_PACKED_STRUCT \
   _Pragma("pack(push)") \
   _Pragma("pack(1)") \
   struct
#define MUSH_PACKED_STRUCT_END _Pragma("pack(pop)")
#endif

#if defined(__GNUC__)
#define MUSH_ALWAYS_INLINE __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define MUSH_ALWAYS_INLINE __forceinline
#else
#define MUSH_ALWAYS_INLINE
#endif

#endif
