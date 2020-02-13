#include "low_level.h"
#include "../window/window.h"
#include "../filesystem/filesystem.h"
#include "../job/job.h"
#include "../rdi_backend/rdi_backend.h"
#include "../resource_manager/resource_manager.h"

#include "../rdix/rdix_debug_draw.h"


bool ENGLowLevel::Startup( ENGLowLevel* e, int argc, const char** argv, BXWindow* window, BXIAllocator* main_allocator )
{
    e->allocator = main_allocator;

    JOB::StartUp();

    BXFilesystemStartup( main_allocator );
    FileSys()->SetRoot( "x:/dev/assets/" );

    RSM::StartUp( FileSys(), main_allocator );

    ::Startup( &e->rdidev, &e->rdicmdq, window->GetSystemHandle( window ), window->width, window->height, 0, e->allocator );

    RDIXDebug::StartUp( e->rdidev, e->allocator );

    return true;
}

void ENGLowLevel::Shutdown( ENGLowLevel* e )
{
    RDIXDebug::ShutDown( e->rdidev );
    RSM::ShutDown();
    ::Shutdown( &e->rdidev, &e->rdicmdq, e->allocator );

    BXFilesystemShutdown( e->allocator );
    
    JOB::ShutDown();

    e->allocator = nullptr;
}
