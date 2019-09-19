#pragma once

#include <foundation/type.h>
#include <foundation/common.h>

union color32_t
{
    uint32_t value;
    struct  
    {
        u8 a;
        u8 b;
        u8 g;
        u8 r;
    };

    constexpr color32_t() : value( 0 ) {}
    constexpr color32_t( u32 rgba ) : value( rgba ) {}

    constexpr operator uint32_t() const { return value; }
    
    static constexpr color32_t WHITE	() { return { 0xFFFFFFFF }; }
    static constexpr color32_t SILVER	() { return { 0xC0C0C0FF }; }
    static constexpr color32_t GRAY	    () { return { 0x808080FF }; }
    static constexpr color32_t BLACK	() { return { 0x000000FF }; }
    static constexpr color32_t RED	    () { return { 0xFF0000FF }; }
    static constexpr color32_t MAROON	() { return { 0x800000FF }; }
    static constexpr color32_t YELLOW	() { return { 0xFFFF00FF }; }
    static constexpr color32_t OLIVE	() { return { 0x808000FF }; }
    static constexpr color32_t LIME	    () { return { 0x00FF00FF }; }
    static constexpr color32_t GREEN	() { return { 0x008000FF }; }
    static constexpr color32_t AQUA	    () { return { 0x00FFFFFF }; }
    static constexpr color32_t TEAL	    () { return { 0x008080FF }; }
    static constexpr color32_t BLUE	    () { return { 0x0000FFFF }; }
    static constexpr color32_t NAVY	    () { return { 0x000080FF }; }
    static constexpr color32_t FUCHSIA  () { return { 0xFF00FFFF }; }
    static constexpr color32_t PURPLE	() { return { 0x800080FF }; }

    static inline color32_t ToRGBA( u8 r, u8 g, u8 b, u8 a )
    {
        color32_t result;
        result.r = r;
        result.g = g;
        result.b = b;
        result.a = a;
        return result;
    }

    static inline color32_t ToRGBA( const f32* f4 )
    {
        u32 r = static_cast<u32>(f4[0] * 255.f);
        u32 g = static_cast<u32>(f4[1] * 255.f);
        u32 b = static_cast<u32>(f4[2] * 255.f);
        u32 a = static_cast<u32>(f4[3] * 255.f);
        r = clamp( r, 0U, 255U );
        g = clamp( g, 0U, 255U );
        b = clamp( b, 0U, 255U );
        a = clamp( a, 0U, 255U );
        return ToRGBA( (u8)r, (u8)g, (u8)b, (u8)a );
    }
    static inline u32 ToRGBA( const f32* f3, u8 a )
    {
        u32 r = static_cast<u32>(f3[0] * 255.f);
        u32 g = static_cast<u32>(f3[1] * 255.f);
        u32 b = static_cast<u32>(f3[2] * 255.f);
        r = clamp( r, 0U, 255U );
        g = clamp( g, 0U, 255U );
        b = clamp( b, 0U, 255U );
        return ToRGBA( (u8)r, (u8)g, (u8)b, (u8)a );
    }

    static inline void ToFloat4( f32* f4, const color32_t color )
    {
        static constexpr float c_div255 = 1.f / 255.f;
        f4[0] = color.r * c_div255;
        f4[1] = color.g * c_div255;
        f4[2] = color.b * c_div255;
        f4[3] = color.a * c_div255;
    }
    static inline void ToFloat3( f32* f3, const color32_t color )
    {
        static constexpr float c_div255 = 1.f / 255.f;
        f3[0] = color.r * c_div255;
        f3[1] = color.g * c_div255;
        f3[2] = color.b * c_div255;
    }

    static color32_t lerp( f32 t, color32_t a, color32_t b )
    {
        f32 fa[4];
        f32 fb[4];

        color32_t::ToFloat4( fa, a );
        color32_t::ToFloat4( fb, b );

        f32 result[4];
        result[0] = fa[0] + t * (fb[0] - fa[0]);
        result[1] = fa[1] + t * (fb[1] - fa[1]);
        result[2] = fa[2] + t * (fb[2] - fa[2]);
        result[3] = fa[3] + t * (fb[3] - fa[3]);

        return color32_t::ToRGBA( result );
    }
};

