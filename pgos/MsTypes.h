////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¨MStar Confidential Information〃) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   MsTypes.h
/// @brief  MStar General Data Types
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MSTYPES_H
#define _MSTYPES_H




#ifdef __cplusplus
extern "C"
{
#endif


//-------------------------------------------------------------------------------------------------
//  System Data Type
//-------------------------------------------------------------------------------------------------

/// data type unsigned char, data length 1 byte
typedef unsigned char               MS_U8;                              // 1 byte
/// data type unsigned short, data length 2 byte
typedef unsigned short              MS_U16;                             // 2 bytes
/// data type unsigned int, data length 4 byte
typedef unsigned int                MS_U32;                             // 4 bytes
/// data type signed char, data length 1 byte
typedef signed char                 MS_S8;                              // 1 byte
/// data type signed short, data length 2 byte
typedef signed short                MS_S16;                             // 2 bytes
/// data type signed int, data length 4 byte
typedef signed int                  MS_S32;                             // 4 bytes

typedef long long       MS_S64;    // 8 bytes

typedef unsigned long long MS_U64;    // 8 bytes

/// data type unsigned char, data length 1 byte
typedef unsigned char   U8;     // 1 byte
/// data type unsigned short, data length 2 byte
typedef unsigned short  U16;    // 2 bytes
/// data type unsigned int, data length 4 byte
typedef unsigned int    U32;    // 4 bytes
/// data type signed char, data length 1 byte
typedef signed char     S8;     // 1 byte
/// data type signed short, data length 2 byte
typedef signed short    S16;    // 2 bytes
/// data type signed int, data length 4 byte
typedef signed int      S32;    // 4 bytes
/// data type signed int, data length 8 byte
typedef long long       S64;    // 8 bytes
/// data type unsigned int, data length 8 byte
typedef unsigned long long U64;    // 8 bytes

typedef	 double			DB;
/// data type null pointer
#ifdef NULL
#undef NULL
#endif
#define NULL                        0

typedef int                 BOOL;


typedef void 				MS_VOID;

typedef char                MS_CHAR;
//-------------------------------------------------------------------------------------------------
//  Software Data Type
//-------------------------------------------------------------------------------------------------
//typedef BOOL                      BOOLEAN;
/// definition for VOID
//typedef void                        VOID;
/// definition for MS_BOOL
typedef int	                        MS_BOOL;

/// definition for FILEID
typedef MS_S32                      FILEID;

//[TODO] use MS_U8, ... instead
// data type for 8051 code
//typedef MS_U16                      WORD;
//typedef MS_U8                       BYTE;
#ifndef WORD
#define		WORD			unsigned  short
//#define		BYTE			unsigned  char
#endif

#if !defined(TRUE) && !defined(FALSE)
/// definition for TRUE
#define TRUE                        1
/// definition for FALSE
#define FALSE                       0
#endif

#define MS_TRUE                     1
#define MS_FALSE                    0

#if !defined(ENABLE) && !defined(DISABLE)
/// definition for ENABLE
#define ENABLE                      1
/// definition for DISABLE
#define DISABLE                     0
#endif


#if !defined(ON) && !defined(OFF)
/// definition for ON
#define ON                          1
/// definition for OFF
#define OFF                         0
#endif


#define __UNUSED                    __attribute__ ((unused))
#define __PACKED                    __attribute__ ((packed))
#define __ALIGNED(n)                __attribute__ ((aligned(n)))
#define __WEAK                      __attribute__ ((weak))


//[RESERVED]
// @name BIT#
// definition of one bit mask
// @{
#if !defined(BIT0) && !defined(BIT1)
#define BIT0	                    0x00000001
#define BIT1	                    0x00000002
#define BIT2	                    0x00000004
#define BIT3	                    0x00000008
#define BIT4	                    0x00000010
#define BIT5	                    0x00000020
#define BIT6	                    0x00000040
#define BIT7	                    0x00000080
#define BIT8	                    0x00000100
#define BIT9	                    0x00000200
#define BIT10	                    0x00000400
#define BIT11	                    0x00000800
#define BIT12	                    0x00001000
#define BIT13	                    0x00002000
#define BIT14	                    0x00004000
#define BIT15  	                    0x00008000
#define BIT16                       0x00010000
#define BIT17                       0x00020000
#define BIT18                       0x00040000
#define BIT19                       0x00080000
#define BIT20                       0x00100000
#define BIT21                       0x00200000
#define BIT22                       0x00400000
#define BIT23                       0x00800000
#define BIT24                       0x01000000
#define BIT25                       0x02000000
#define BIT26                       0x04000000
#define BIT27                       0x08000000
#define BIT28                       0x10000000
#define BIT29                       0x20000000
#define BIT30                       0x40000000
#define BIT31                       0x80000000
#endif
// @}
//[RESERVED]

#define HIGH_BYTE(x)                ((U8)(x>>8))
#define LOW_BYTE(x)                 ((U8)x)

/// Macros for setting/clearing of bits
#define SETBIT(_data, _bit)         ((_data) |=  (0x1 << (_bit)))
#define CLRBIT(_data, _bit)         ((_data) &= ~(0x1 << (_bit)))
#define GETBIT(_data, _bit)         (((_data) >> (_bit)) & 0X1)
#define ALIGN_4(_x)     ((_x + 3) & ~3)
#define ALIGN_16(_x)    ((_x + 15) & ~15)           // No data type specified, optimized by complier
#define ALIGN_32(_x)    ((_x + 31) & ~31)           // No data type specified, optimized by complier

#define MASK(x)         (((1<<(x##_BITS))-1) << x##_SHIFT)
#define BIT(x)          (1<<(x))

/// Macros for setting/clearing of bits
#define SETBIT(_data, _bit)     ((_data) |=  (0x1 << (_bit)))
#define CLRBIT(_data, _bit)     ((_data) &= ~(0x1 << (_bit)))
#define GETBIT(_data, _bit)     (((_data) >> (_bit)) & 0X1)
#define U64_MAX         (0xFFFFFFFFFFFFFFFFULL)
#define S64_MAX         (0x7FFFFFFFFFFFFFFFLL)
#define U32_MAX         (0xFFFFFFFFUL)
#define S32_MAX         (0x7FFFFFFFL)
#define U16_MAX         (0xFFFF)
#define S16_MAX         (0x7FFF)
#define U8_MAX          (0xFF)
#define S8_MAX          (0x7F)
#define _BIT0           (0x00000001UL)
#define _BIT1           (0x00000002UL)
#define _BIT2           (0x00000004UL)
#define _BIT3           (0x00000008UL)
#define _BIT4           (0x00000010UL)
#define _BIT5           (0x00000020UL)
#define _BIT6           (0x00000040UL)
#define _BIT7           (0x00000080UL)
#define _BIT8           (0x00000100UL)
#define _BIT9           (0x00000200UL)
#define _BIT10          (0x00000400UL)
#define _BIT11          (0x00000800UL)
#define _BIT12          (0x00001000UL)
#define _BIT13          (0x00002000UL)
#define _BIT14          (0x00004000UL)
#define _BIT15          (0x00008000UL)
#define _BIT16          (0x00010000UL)
#define _BIT17          (0x00020000UL)
#define _BIT18          (0x00040000UL)
#define _BIT19          (0x00080000UL)
#define _BIT20          (0x00100000UL)
#define _BIT21          (0x00200000UL)
#define _BIT22          (0x00400000UL)
#define _BIT23          (0x00800000UL)
#define _BIT24          (0x01000000UL)
#define _BIT25          (0x02000000UL)
#define _BIT26          (0x04000000UL)
#define _BIT27          (0x08000000UL)
#define _BIT28          (0x10000000UL)
#define _BIT29          (0x20000000UL)
#define _BIT30          (0x40000000UL)
#define _BIT31          (0x80000000UL)


#ifdef __cplusplus
}
#endif


#endif // _MSTYPES_H
