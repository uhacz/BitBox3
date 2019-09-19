#include "rdi_backend.h"
#include "rdi_backend_dx11.h"
#include <stdio.h>

RDIEType::Enum RDIEType::FromName( const char* name )
{
    for( int itype = 0; itype < RDIEType::COUNT; ++itype )
    {
        if( !strcmp( name, RDIEType::name[itype] ) )
            return (RDIEType::Enum)itype;
    }
    return RDIEType::UNKNOWN;
}

RDIEType::Enum RDIEType::FindBaseType( const char* name )
{
    for( int itype = 0; itype < RDIEType::COUNT; ++itype )
    {
        if( strstr( name, RDIEType::name[itype] ) )
            return (RDIEType::Enum)itype;
    }
    return RDIEType::UNKNOWN;
}


RDIEVertexSlot::Enum RDIEVertexSlot::FromString( const char* n )
{
    for( int i = 0; i < RDIEVertexSlot::COUNT; ++i )
    {
        if( !strcmp( n, RDIEVertexSlot::name[i] ) )
            return (RDIEVertexSlot::Enum )i;
    }

    return RDIEVertexSlot::COUNT;
}
bool RDIEVertexSlot::ToString( char* output, uint32_t output_size, RDIEVertexSlot::Enum slot )
{
    int res = 0;
    switch( slot )
    {
    case RDIEVertexSlot::POSITION:
    case RDIEVertexSlot::NORMAL:
    case RDIEVertexSlot::TANGENT:
    case RDIEVertexSlot::BINORMAL:
    case RDIEVertexSlot::BLENDWEIGHT:
    case RDIEVertexSlot::BLENDINDICES:
        res = snprintf( output, output_size, "%s", name[slot] );
        break;
    case RDIEVertexSlot::COLOR0:
    case RDIEVertexSlot::COLOR1:
    case RDIEVertexSlot::TEXCOORD0:
    case RDIEVertexSlot::TEXCOORD1:
    case RDIEVertexSlot::TEXCOORD2:
    case RDIEVertexSlot::TEXCOORD3:
    case RDIEVertexSlot::TEXCOORD4:
    case RDIEVertexSlot::TEXCOORD5:
    {
        res = snprintf( output, output_size, "%s%d", name[slot], semanticIndex[slot] );
    }break;

   default:
       return false;
       break;
    }

    return res && (uint32_t)res < output_size;
}

void Startup( RDIDevice** dev, RDICommandQueue** cmdq, uintptr_t hWnd, int winWidth, int winHeight, int fullScreen, BXIAllocator* allocator )
{
    bx::rdi::StartupDX11( dev, cmdq, hWnd, winWidth, winHeight, fullScreen, allocator );
}

void Shutdown( RDIDevice** dev, RDICommandQueue** cmdq, BXIAllocator* allocator )
{
	bx::rdi::ShutdownDX11( dev, cmdq, allocator );
}
