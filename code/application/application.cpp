#include "application.h"
#include "../memory/memory.h"
#include "../foundation/string_util.h"

#include "ride/ride_app.h"

BXIApplication* CreateApplication( const char* name, BXIAllocator* allocator )
{
    if( string::equal( name, "ride" ) )
    {
        return BX_NEW( allocator, RideApplication );
    }

    return nullptr;
}

void DestroyApplication( BXIApplication** app, BXIAllocator* allocator )
{
    BX_DELETE0( allocator, app[0] );
}
