#include "hardware_state_util.h"
#include <string.h>

namespace bx{ namespace tool {

namespace DepthFunc
{
	RDIEDepthFunc::Enum FromString( const char* str )
    {
		RDIEDepthFunc::Enum value = RDIEDepthFunc::LEQUAL;

	    if	   ( !strcmp( str, "NEVER" 	  ) ) value = RDIEDepthFunc::NEVER;
	    else if( !strcmp( str, "LESS" 	  ) ) value = RDIEDepthFunc::LESS;
	    else if( !strcmp( str, "EQUAL" 	  ) ) value = RDIEDepthFunc::EQUAL;
	    else if( !strcmp( str, "LEQUAL"   ) ) value = RDIEDepthFunc::LEQUAL;
	    else if( !strcmp( str, "GREATER"  ) ) value = RDIEDepthFunc::GREATER;
	    else if( !strcmp( str, "NOTEQUAL" ) ) value = RDIEDepthFunc::NOTEQUAL;
	    else if( !strcmp( str, "GEQUAL"   ) ) value = RDIEDepthFunc::GEQUAL;
	    else if( !strcmp( str, "ALWAYS"   ) ) value = RDIEDepthFunc::ALWAYS;

	    return value;
    }
}//

namespace BlendFactor
{
    RDIEBlendFactor::Enum FromString( const char* str )
    {
        RDIEBlendFactor::Enum value = RDIEBlendFactor::ZERO;
	
	    if	   ( !strcmp( str, "ZERO" 				 ) )	value = RDIEBlendFactor::ZERO;
	    else if( !strcmp( str, "ONE" 				 ) )	value = RDIEBlendFactor::ONE;
	    else if( !strcmp( str, "SRC_COLOR" 			 ) )	value = RDIEBlendFactor::SRC_COLOR;
	    else if( !strcmp( str, "ONE_MINUS_SRC_COLOR" ) )	value = RDIEBlendFactor::ONE_MINUS_SRC_COLOR;
	    else if( !strcmp( str, "DST_COLOR" 			 ) )	value = RDIEBlendFactor::DST_COLOR;
	    else if( !strcmp( str, "ONE_MINUS_DST_COLOR" ) )	value = RDIEBlendFactor::ONE_MINUS_DST_COLOR;
	    else if( !strcmp( str, "SRC_ALPHA" 			 ) )	value = RDIEBlendFactor::SRC_ALPHA;
	    else if( !strcmp( str, "ONE_MINUS_SRC_ALPHA" ) )	value = RDIEBlendFactor::ONE_MINUS_SRC_ALPHA;
	    else if( !strcmp( str, "DST_ALPHA" 			 ) )	value = RDIEBlendFactor::DST_ALPHA;
	    else if( !strcmp( str, "ONE_MINUS_DST_ALPHA" ) )	value = RDIEBlendFactor::ONE_MINUS_DST_ALPHA;
	    else if( !strcmp( str, "SRC_ALPHA_SATURATE"  ) )	value = RDIEBlendFactor::SRC_ALPHA_SATURATE;
	    return value;
    }
}//

namespace BlendEquation
{
    RDIEBlendEquation::Enum FromString( const char* str )
    {
        RDIEBlendEquation::Enum value = RDIEBlendEquation::ADD;
        if     ( !strcmp( str, "ADD"         ) ) value = RDIEBlendEquation::ADD; 
        else if( !strcmp( str, "SUB"         ) ) value = RDIEBlendEquation::SUB;
        else if( !strcmp( str, "REVERSE_SUB" ) ) value = RDIEBlendEquation::REVERSE_SUB;
        else if( !strcmp( str, "MIN"         ) ) value = RDIEBlendEquation::MIN;
        else if( !strcmp( str, "MAX"         ) ) value = RDIEBlendEquation::MAX;

        return value;
    }
}//

namespace Culling
{
    RDIECullMode::Enum FromString( const char* str )
    {
        RDIECullMode::Enum value = RDIECullMode::BACK;
	    if	   ( !strcmp( str, "NONE"  ) ) value = RDIECullMode::NONE;
	    else if( !strcmp( str, "BACK"  ) ) value = RDIECullMode::BACK;
	    else if( !strcmp( str, "FRONT" ) ) value = RDIECullMode::FRONT;
	    return value;
    }
}//

namespace Fillmode
{
    RDIEFillMode::Enum FromString( const char* str )
    {
        RDIEFillMode::Enum value = RDIEFillMode::SOLID;
        if( !strcmp( str, "SOLID" ) ) value = RDIEFillMode::SOLID;
        if( !strcmp( str, "WIREFRAME" ) ) value = RDIEFillMode::WIREFRAME;
	    return value;
    }
}// 

namespace ColorMask
{
	uint8_t FromString( const char* str )
    {
		uint8_t value = 0;
	
        const char* r = strchr( str, 'R' );
        const char* g = strchr( str, 'G' );
        const char* b = strchr( str, 'B' );
        const char* a = strchr( str, 'A' );

        value |= ( r ) ? RDIEColorMask::RED : 0;
        value |= ( g ) ? RDIEColorMask::GREEN : 0;
        value |= ( b ) ? RDIEColorMask::BLUE : 0;
        value |= ( a ) ? RDIEColorMask::ALPHA : 0;

	    return value;
    }
}//

}}///