#ifndef TYPES_H_
#define TYPES_H_
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include <float.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>

typedef int8_t int8;  
typedef int16_t int16; 
typedef int32_t int32; 
typedef int64_t int64; 

typedef int8_t s8;  
typedef int16_t s16; 
typedef int32_t s32; 
typedef int64_t s64; 

typedef int32 bool32;
typedef bool32 b32;

typedef uint8_t uint8;
typedef uint16_t uint16; 
typedef uint32_t uint32; 
typedef uint64_t uint64; 

typedef uint8_t u8;  
typedef uint16_t u16; 
typedef uint32_t u32; 
typedef uint64_t u64; 

typedef float f32; 
typedef double f64; 

#define KB(X) (X << 10LL)
#define MB(X) (KB(X) << 10LL)
#define GB(X) (MB(X) << 10LL)

#endif // TYPES_H_
