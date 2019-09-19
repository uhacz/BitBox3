#pragma once

#include <stdint.h>
#include <float.h>

#ifdef _MSC_VER
#pragma warning( disable: 4512 )
#endif

#ifdef _MSC_VER
#pragma warning( disable: 4351 )
#endif

//nonstandard extension used: nameless struct/union
#ifdef _MSC_VER
#pragma warning( disable: 4201 )
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

union TypeReinterpert
{
    u32 u;
    i32 i;
    f32 f;

    explicit TypeReinterpert( u32 v ) : u( v ) {}
    explicit TypeReinterpert( i32 v ) : i( v ) {}
    explicit TypeReinterpert( f32 v ) : f( v ) {}
};

#define FORCE_INLINE __forceinline


#define BIT_KILO_BYTE( x ) ( x << 10 )
#define BIT_MEGA_BYTE( x ) ( x << 20 )

#define BIT_OFFSET(n) (1<<n)
#define PTR_TO_UINT32( p )   ((uint32_t)(uintptr_t) (uint32_t*) (p))
#define UINT32_TO_PTR( ui )  ((void *)(uint32_t*)((uint32_t)ui))

#define TYPE_OFFSET_GET_POINTER(type,offset) ( (offset)? (type*)((intptr_t)(&offset)+(intptr_t)(offset)) : (type*)0 )
#define TYPE_POINTER_GET_OFFSET(base, address) ( (base) ? (PTR_TO_UINT32(address) - PTR_TO_UINT32(base)) : 0 )

template< typename T >
inline const T* Offset2Pointer( const u32& offset )
{
    return (offset) ? (T*)((intptr_t)(&offset) + (intptr_t)(offset)) : (T*)0;
}

#ifdef _MSC_VER
#define BIT_ALIGNMENT( alignment )	__declspec(align(alignment))	
#else
#define BIT_ALIGNMENT( alignment ) __attribute__ ((aligned(alignment)))
#endif


#define BIT_ALIGNMENT_16 BIT_ALIGNMENT(16)
#define BIT_ALIGNMENT_64 BIT_ALIGNMENT(64)
#define BIT_ALIGNMENT_128 BIT_ALIGNMENT(128)

// "alignment" must be a power of 2
#define TYPE_IS_ALIGNED(value, alignment)                       \
	(                                                           \
		!(((intptr_t)(value)) & (((intptr_t)(alignment)) - 1U))         \
	) 


#define TYPE_ALIGN(value, alignment)                                \
	(                                                               \
		(((intptr_t) (value)) + (((intptr_t) (alignment)) - 1U))			\
		& ~(((intptr_t) (alignment)) - 1U)								\
	)



#define MAKE_STR(x) MAKE_STR_(x)
#define MAKE_STR_(x) #x

template <typename T, size_t N>
constexpr size_t sizeof_array( const T( &)[N] )
{
    return N;
}

inline bool is_pow2( int n )
{
    return ( n&( n - 1 ) ) == 0;
}

struct Blob
{
    uint8_t* data;
    uint32_t size;
};