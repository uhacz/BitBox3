#ifndef TRANSFORM_INSTANCE_DATA
#define TRANSFORM_INSTANCE_DATA

#define TRANSFORM_INSTANCE_DATA_SLOT 0

#define TRANSFORM_INSTANCE_WORLD_SLOT 0
#define TRANSFORM_INSTANCE_WORLD_IT_SLOT 1

struct InstanceData
{
	uint offset;
    uint camera_index;
    uint material_index;
    uint __padding[1];
};

#ifdef SHADER_IMPLEMENTATION
shared cbuffer _InstanceData : register(BSLOT( TRANSFORM_INSTANCE_DATA_SLOT ))
{
    InstanceData _idata;
};
Buffer<float4> _instance_world   : register(TSLOT( TRANSFORM_INSTANCE_WORLD_SLOT ));
Buffer<float3> _instance_worldIT : register(TSLOT( TRANSFORM_INSTANCE_WORLD_IT_SLOT ));

void LoadWorld( out float4 row0, out float4 row1, out float4 row2, uint instanceID )
{
	uint row0Index = (_idata.offset + instanceID) * 3;
	row0 = _instance_world[row0Index];
	row1 = _instance_world[row0Index + 1];
	row2 = _instance_world[row0Index + 2];
}

void LoadWorldIT( out float3 row0IT, out float3 row1IT, out float3 row2IT, uint instanceID )
{
	uint row0Index = (_idata.offset + instanceID) * 3;
	row0IT = _instance_worldIT[row0Index];
	row1IT = _instance_worldIT[row0Index + 1];
	row2IT = _instance_worldIT[row0Index + 2];
}

float3 TransformPosition( in float4 row0, in float4 row1, in float4 row2, in float4 localPos )
{
	return float3( dot( row0, localPos ), dot( row1, localPos ), dot( row2, localPos ) );
}
float3 TransformNormal( in float3 row0IT, in float3 row1IT, in float3 row2IT, in float3 normal )
{
	return float3( dot( row0IT, normal ), dot( row1IT, normal ), dot( row2IT, normal ) );
}
#endif

#endif