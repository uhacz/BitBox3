#ifndef SHADER_SKINNING_H
#define SHADER_SKINNING_H

#define SKINNING_INPUT_POS_SLOT 0
#define SKINNING_INPUT_NRM_SLOT 1
#define SKINNING_INPUT_BW_SLOT 2
#define SKINNING_INPUT_BI_SLOT 3
#define SKINNING_INPUT_MATRICES_SLOT 4
#define SKINNING_CBUFFER_SLOT 0

#define SKINNING_OUTPUT_POS_SLOT 0
#define SKINNING_OUTPUT_NRM_SLOT 1

struct SkinningData
{
    uint pin_index;
    float3 padding_;
};

#ifdef SHADER_IMPLEMENTATION
shared cbuffer _MaterialData : BSLOT( SKINNING_CBUFFER_SLOT )
{
    SkinningData _data;
};

#endif

#endif