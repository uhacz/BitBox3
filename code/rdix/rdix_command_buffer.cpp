#include "rdix_command_buffer.h"
#include "rdix.h"
#include <rdi_backend/rdi_backend.h>

#include <memory/memory.h>
#include <foundation/buffer.h>

#include <string.h>
#include <algorithm>

#define RDIX_DEFINE_COMMAND( name, block )\
void Dispatch_##name( RDICommandQueue* cmdq, RDIXCommand* cmdAddr )\
{\
    name* cmd = (name*)cmdAddr;\
    block\
}\
const DispatchFunction name::DISPATCH_FUNCTION = Dispatch_##name

RDIX_DEFINE_COMMAND( RDIXSetRenderTargetCmd,
{ BindRenderTarget( cmdq, cmd->rtarget, cmd->color_textures_mask, cmd->depth ? true : false ); } );

RDIX_DEFINE_COMMAND( RDIXClearRenderTargetCmd,
{
    if( cmd->r >= 0.f )
        ClearRenderTarget( cmdq, cmd->rtarget, cmd->rgbad );
    else
        ClearRenderTargetDepth( cmdq, cmd->rtarget, cmd->d );
} );


RDIX_DEFINE_COMMAND( RDIXSetPipelineCmd, 
{ BindPipeline( cmdq, cmd->pipeline, cmd->bindResources ? true : false ); } );

RDIX_DEFINE_COMMAND( RDIXSetResourcesCmd,
{ BindResources( cmdq, cmd->rbind ); } );

RDIX_DEFINE_COMMAND( RDIXSetResourceROCmd,
{ SetResourcesRO( cmdq, &cmd->resource, cmd->slot, 1, cmd->stage_mask ); } );

RDIX_DEFINE_COMMAND( RDIXSetConstantBufferCmd,
{ SetCbuffers( cmdq, &cmd->resource, cmd->slot, 1, cmd->stage_mask ); } );

RDIX_DEFINE_COMMAND( RDIXSetRenderSourceCmd, 
{ BindRenderSource( cmdq, cmd->rsource ); } );

RDIX_DEFINE_COMMAND( RDIXDrawRenderSourceCmd,
{
    BindRenderSource( cmdq, cmd->rsource );
    SubmitRenderSourceInstanced( cmdq, cmd->rsource, cmd->num_instances, cmd->rsouce_range );
} );

RDIX_DEFINE_COMMAND( RDIXDrawCmd,
{ Draw( cmdq, cmd->num_vertices, cmd->start_index ); } );

RDIX_DEFINE_COMMAND( RDIXDrawInstancedCmd,
{ DrawInstanced( cmdq, cmd->num_vertices, cmd->start_index, cmd->num_instances ); } );

RDIX_DEFINE_COMMAND( RDIXUpdateConstantBufferCmd,
{ UpdateCBuffer( cmdq, cmd->cbuffer, cmd->DataPtr() ); } );

RDIX_DEFINE_COMMAND( RDIXUpdateBufferCmd,
{
    uint8_t* mapped_ptr = Map( cmdq, cmd->resource, 0, RDIEMapType::WRITE );
    memcpy( mapped_ptr, cmd->DataPtr(), cmd->size );
    Unmap( cmdq, cmd->resource );
} );

RDIX_DEFINE_COMMAND( RDIXDrawCallbackCmd,
{ (*cmd->ptr)(cmdq, cmd->flags, cmd->user_data); } );

// ---
struct RDIXCommandBuffer
{
	struct CmdInternal
	{
		uint64_t key;
		RDIXCommand* cmd;
	};

	struct Data
	{
		void* _memory_handle = nullptr;
		CmdInternal* commands = nullptr;
		uint8_t* data = nullptr;

		uint32_t max_commands = 0;
		uint32_t num_commands = 0;
		uint32_t data_capacity = 0;
		uint32_t data_size = 0;
	}_data;

	struct
	{
        uint32_t commands;
        uint32_t data;
	} _overflow;

	uint32_t _can_add_commands = 0;

	void AllocateData( uint32_t maxCommands, uint32_t dataCapacity, BXIAllocator* allocator );
	void FreeData( BXIAllocator* allocator );
};

void RDIXCommandBuffer::AllocateData( uint32_t maxCommands, uint32_t dataCapacity, BXIAllocator* allocator )
{
	SYS_ASSERT( _can_add_commands == 0 );

	if( _data.max_commands >= maxCommands && _data.data_capacity >= dataCapacity )
		return;

	uint32_t mem_size = maxCommands * sizeof( CmdInternal );
	mem_size += dataCapacity;

	void* mem = BX_MALLOC( allocator, mem_size, 16 );
	memset( mem, 0x00, mem_size );

	Data newdata = {};
	newdata._memory_handle = mem;
	newdata.max_commands = maxCommands;
	newdata.data_capacity = dataCapacity;

	BufferChunker chunker( mem, mem_size );
	newdata.commands = chunker.Add< CmdInternal >( maxCommands );
	newdata.data = chunker.AddBlock( dataCapacity ).begin;
	chunker.Check();

	FreeData( allocator );
	_data = newdata;
}

void RDIXCommandBuffer::FreeData( BXIAllocator* allocator )
{
	BX_FREE( allocator, _data._memory_handle );
	_data = {};
}

RDIXCommandBuffer* CreateCommandBuffer( BXIAllocator* allocator, uint32_t maxCommands, uint32_t dataCapacity )
{
	RDIXCommandBuffer* impl = BX_NEW( allocator, RDIXCommandBuffer );

	dataCapacity += maxCommands * sizeof( RDIXDrawRenderSourceCmd );
	impl->AllocateData( maxCommands, dataCapacity, allocator );
	return impl;
}

void DestroyCommandBuffer( RDIXCommandBuffer** cmdBuff, BXIAllocator* allocator )
{
	if( !cmdBuff[0] )
		return;

	RDIXCommandBuffer* impl = cmdBuff[0];
	impl->FreeData( allocator );
	BX_FREE0( allocator, cmdBuff[0] );
}

void ClearCommandBuffer( RDIXCommandBuffer* cmdBuff, BXIAllocator* allocator )
{
	if( cmdBuff->_overflow.data + cmdBuff->_overflow.commands )
	{
        const uint32_t max_commands = cmdBuff->_data.max_commands + cmdBuff->_overflow.commands;
        const uint32_t max_data = cmdBuff->_data.data_capacity + cmdBuff->_overflow.data;
        if( max_commands != cmdBuff->_data.max_commands || max_data != cmdBuff->_data.data_capacity )
        {
            cmdBuff->AllocateData( max_commands, max_data, allocator );
        }

		cmdBuff->_overflow.data = 0;
        cmdBuff->_overflow.commands = 0;
    }
	cmdBuff->_data.num_commands = 0;
	cmdBuff->_data.data_size = 0;
}

void BeginCommandBuffer( RDIXCommandBuffer* cmdBuff )
{
	cmdBuff->_can_add_commands = 1;
}
void EndCommandBuffer( RDIXCommandBuffer* cmdBuff )
{
	cmdBuff->_can_add_commands = 0;
}
void SubmitCommandBuffer( RDICommandQueue* cmdq, RDIXCommandBuffer* cmdBuff )
{
	SYS_ASSERT( cmdBuff->_can_add_commands == 0 );

	struct CmdInternalCmp {
		inline bool operator () ( const RDIXCommandBuffer::CmdInternal a, const RDIXCommandBuffer::CmdInternal b ) { return a.key < b.key; }
	};

	RDIXCommandBuffer::Data& data = cmdBuff->_data;
	std::sort( data.commands, data.commands + data.num_commands, CmdInternalCmp() );

	for( uint32_t i = 0; i < data.num_commands; ++i )
	{
		RDIXCommand* cmd = data.commands[i].cmd;
		while( cmd )
		{
			(*cmd->_dispatch_ptr)(cmdq, cmd);
			cmd = cmd->_next;
		}
	}
}
bool SubmitCommand( RDIXCommandBuffer* cmdbuff, RDIXCommand* cmdPtr, uint64_t sortKey )
{
	SYS_ASSERT( cmdbuff->_can_add_commands );
	RDIXCommandBuffer::Data& data = cmdbuff->_data;
    if( data.num_commands >= data.max_commands )
    {
        cmdbuff->_overflow.commands += 1;
        return false;
    }

    if( cmdPtr )
    {
        uint32_t index = data.num_commands++;
        RDIXCommandBuffer::CmdInternal cmd_int = {};
        cmd_int.key = sortKey;
        cmd_int.cmd = cmdPtr;
        data.commands[index] = cmd_int;
    }
	return cmdPtr != nullptr;
}
void* _AllocateCommand( RDIXCommandBuffer* cmdbuff, uint32_t cmdSize )
{
	SYS_ASSERT( cmdbuff->_can_add_commands );
	RDIXCommandBuffer::Data& data = cmdbuff->_data;
    if( data.data_size + cmdSize > data.data_capacity )
    {
        cmdbuff->_overflow.data += cmdSize;
        return nullptr;
    }
	uint8_t* ptr = data.data + data.data_size;
	data.data_size += cmdSize;

	return ptr;
}

RDIXUpdateConstantBufferCmd::RDIXUpdateConstantBufferCmd( const RDIConstantBuffer& cb, const void* data, uint32_t data_size ) : cbuffer( cb )
{
    SYS_ASSERT( data_size <= cbuffer.size_in_bytes );
    memcpy( DataPtr(), data, data_size );
}

RDIXUpdateBufferCmd::RDIXUpdateBufferCmd( const RDIResource& rs, const void* data, uint32_t datasize ) : resource( rs ), size( datasize )
{
    memcpy( DataPtr(), data, datasize );
}
