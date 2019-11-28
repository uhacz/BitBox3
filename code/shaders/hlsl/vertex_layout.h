#ifndef SHADER_VERTEX_LAYOUT_H
#define SHADER_VERTEX_LAYOUT_H

#define MAX_VERTEX_STREAMS 15
#define MAX_VERTEX_ATTRIBS 4

struct VertexStream
{
    uint4 attrib;
    uint nb_attribs;
    uint stride;
    uint2 __padding;
};

struct VertexLayout
{
    VertexStream stream[MAX_VERTEX_STREAMS];
    uint nb_streams;
    uint3 __padding0;
    uint4 __padding1;
};

struct DrawRange
{
    uint begin;
    uint count;
    uint base_vertex;
    uint stride;
};

#ifdef SHADER_IMPLEMENTATION
shared cbuffer VertexLayoutCB
{
    VertexLayout _vlayout;
};

shared cbuffer DrawRangeCB
{
    DrawRange _draw_range;
};


uint VertexOffset( in VertexStream stream, in uint attrib_index, in uint vertex_index )
{
    return stream.stride * vertex_index + stream.attrib[attrib_index];
}

#define VertexLoad1F( stream_desc, attrib_index, vindex, vstream ) asfloat( vstream.Load( VertexOffset( stream_desc, attrib_index, vindex ) ) )
#define VertexLoad1I( stream_desc, attrib_index, vindex, vstream )          vstream.Load( VertexOffset( stream_desc, attrib_index, vindex ) )

#define VertexLoadNF( num_elements, stream_desc, attrib_index, vindex, vstream ) asfloat( vstream.Load##num_elements( VertexOffset( stream_desc, attrib_index, vindex ) ) )
#define VertexLoadNI( num_elements, stream_desc, attrib_index, vindex, vstream )        ( vstream.Load##num_elements( VertexOffset( stream_desc, attrib_index, vindex  ) ) )

#define VertexLoad2F( stream_desc, attrib_index, vindex, vstream ) VertexLoadNF( 2, stream_desc, attrib_index, vindex, vstream )
#define VertexLoad3F( stream_desc, attrib_index, vindex, vstream ) VertexLoadNF( 3, stream_desc, attrib_index, vindex, vstream )
#define VertexLoad4F( stream_desc, attrib_index, vindex, vstream ) VertexLoadNF( 4, stream_desc, attrib_index, vindex, vstream )


#define LoadVertexIndex( draw_range, index, istream ) ( istream.Load( ( index + draw_range.begin ) * draw_range.stride ) + draw_range.base_vertex )

#else
inline bool AddAttrib( VertexStream* stream, RDIFormat format )
{
    if( stream->nb_attribs >= MAX_VERTEX_ATTRIBS )
        return false;

    stream->attrib[stream->nb_attribs++] = stream->stride;
    stream->stride += format.ByteWidth();
    return true;

}
inline VertexStream* AddStream( VertexLayout* layout )
{
    if( layout->nb_streams >= MAX_VERTEX_STREAMS )
        return nullptr;

    return &layout->stream[layout->nb_streams++];
}

#endif

#endif