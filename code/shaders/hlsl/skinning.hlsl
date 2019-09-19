<pass name = "pos_nrm" compute = "cs_pos_nrm">
</pass>
#~header

#define SHADER_IMPLEMENTATION

#include "common.h"
#include "skinning.h"

ByteAddressBuffer in_pos : TSLOT( SKINNING_INPUT_POS_SLOT );
ByteAddressBuffer in_nrm : TSLOT( SKINNING_INPUT_NRM_SLOT );
ByteAddressBuffer in_bw  : TSLOT( SKINNING_INPUT_BW_SLOT );
ByteAddressBuffer in_bi  : TSLOT( SKINNING_INPUT_BI_SLOT );
Buffer<float4> in_skinning_matrices : TSLOT( SKINNING_INPUT_MATRICES_SLOT );

RWByteAddressBuffer out_pos : USLOT( SKINNING_OUTPUT_POS_SLOT );
RWByteAddressBuffer out_nrm : USLOT( SKINNING_OUTPUT_NRM_SLOT );

[numthreads( 64, 1, 1 )]
void cs_pos_nrm( uint3 DTid : SV_DispatchThreadID )
{
    const uint ivertex = DTid.x;

    const float4 bw = asfloat( in_bw.Load4( ivertex * 16 ) );
    uint  bi = in_bi.Load( ivertex * 4 );

    const float4 base_pos = float4( asfloat( in_pos.Load3( ivertex * 12 ) ), 1.0 );
    const float3 base_nrm = asfloat( in_nrm.Load3( ivertex * 12 ) );

    float4 pos = float4(0, 0, 0, 0);
    float3 nrm = float3(0, 0, 0);
    
    for( uint i = 0; i < 4; ++i )
    {
        uint matrix_index = bi & 0xFF;
        bi = bi >> 8;

        uint first_col_index = _data.pin_index + (matrix_index * 4);
        float4x4 mtx = float4x4(
            in_skinning_matrices[first_col_index],
            in_skinning_matrices[first_col_index + 1],
            in_skinning_matrices[first_col_index + 2],
            in_skinning_matrices[first_col_index + 3]
            );

        pos += mul( base_pos, mtx ) * bw[i];
        nrm += mul( (float3x3)mtx, base_nrm ) * bw[i];

    }

    nrm = normalize( nrm );

    out_pos.Store3( ivertex * 12, asuint( pos.xyz ) );
    out_nrm.Store3( ivertex * 12, asuint( nrm ) );

}