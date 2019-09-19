#pragma once

#include <foundation/math/vmath_type.h>
#include <foundation/type.h>
#include "../bbox.h"

struct BXIAllocator;

struct poly_shape_t
{
    poly_shape_t( int pNElem = 3, int nNElem = 3, int tNElem = 2 )
        : positions(0)
        , normals(0)
        , tangents(0)
        , bitangents(0)
        , texcoords(0)
        , indices(0)
        , num_vertices(0)
        , num_indices(0)
        , n_elem_pos(pNElem)
        , n_elem_nrm(nNElem)
        , n_elem_tex(tNElem)
    {}

    BXIAllocator* allocator = nullptr;

    float* positions;
    float* normals;
    float* tangents;
    float* bitangents;
    float* texcoords;
    unsigned* indices;

    int num_vertices;
    int num_indices;

    const int n_elem_pos;
    const int n_elem_nrm;
    const int n_elem_tex;

    inline float* position     ( int index ) const { return positions  + index*n_elem_pos; }
    inline float* normal       ( int index ) const { return normals    + index*n_elem_nrm; }
    inline float* tangent      ( int index ) const { return tangents   + index*n_elem_nrm; }
    inline float* bitangent    ( int index ) const { return bitangents + index*n_elem_nrm; }
    inline float* texcoord     ( int index ) const { return texcoords  + index*n_elem_tex; }
    inline unsigned*   triangle     ( int index ) const { return indices    + index*3; }
    inline int    ntriangles()               const { return num_indices/3; }   
    inline int    nvertices ()               const { return num_vertices; }   
        
private:
    poly_shape_t& operator = ( const poly_shape_t& ) { return *this; }
};

namespace poly_shape
{
    int computeNumPoints( const int iterations[6] );
    int computeNumTriangles( const int iterations[6] );

    int computeNumPointsPerSide( int iterations );
    int computeNumTrianglesPerSide( int iterations );

    void allocateShape( poly_shape_t* shape, int numVertices, int numIndices, BXIAllocator* allocator );
    void deallocateShape( poly_shape_t* shape );
    void copy( poly_shape_t* dst, const poly_shape_t& src );

    void allocateCube( poly_shape_t* shape, const int iterations[6], BXIAllocator* allocator );
    void createCube( poly_shape_t* shape, const int iterations[6], float extent = 1.0f );
    void normalize( poly_shape_t* shape, int begin, int count, float tolerance = FLT_EPSILON );
    void transform( poly_shape_t* shape, int begin, int count, const mat44_t& tr );
    void generateSphereUV( poly_shape_t* shape, int firstPoint, int count );
    void generateFlatNormals( poly_shape_t* shape, int firstTriangle, int count );
    void generateSmoothNormals( poly_shape_t* shape, int firstTriangle, int count );
    void generateSmoothNormals1( poly_shape_t* shape, int firstPoint, int count );
    void computeTangentSpace( poly_shape_t* shape );

    //////////////////////////////////////////////////////////////////////////
    void createBox( poly_shape_t* shape, int iterations, BXIAllocator* allocator );
    void createShpere( poly_shape_t* shape, int iterations, BXIAllocator* allocator );
    void createCapsule( poly_shape_t* shape, int surfaces[6], int vertexCount[3], int iterations, BXIAllocator* allocator );
}//