#pragma once

#include <rdi_backend/rdi_backend_type.h>

namespace bx{ namespace tool{

namespace DepthFunc
{
	extern RDIEDepthFunc::Enum FromString( const char* str );
}//

namespace BlendFactor
{
	extern RDIEBlendFactor::Enum FromString( const char* str );
}//

namespace BlendEquation
{
    extern RDIEBlendEquation::Enum FromString( const char* str );
}//

namespace Culling
{
	extern RDIECullMode::Enum FromString( const char* str );
}//

namespace Fillmode
{
	extern RDIEFillMode::Enum FromString( const char* str );
}// 

namespace ColorMask
{
	extern uint8_t FromString( const char* str );
}//

}}///