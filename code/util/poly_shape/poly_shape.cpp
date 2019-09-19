#define PAR_SHAPES_IMPLEMENTATION

#include "poly_shape.h"
#include <memory/memory.h>
#include <foundation/debug.h>
#include <foundation/common.h>

#include <string.h>
#include <new.h>
#include <foundation/math/vec2.h>
#include <foundation/math/vec3.h>

struct Triangle
{
    unsigned p0;
    unsigned p1;
    unsigned p2;
};

using Vec2 = vec2_t;
using Vec3 = vec3_t;

namespace poly_shape
{
    void allocateShape( poly_shape_t* shape, int numVertices, int numIndices, BXIAllocator* allocator )
    {
        unsigned memSize = 0;
        memSize += numVertices * shape->n_elem_pos * sizeof( float );
        memSize += numVertices * shape->n_elem_nrm * sizeof( float );
        memSize += numVertices * shape->n_elem_nrm * sizeof( float );
        memSize += numVertices * shape->n_elem_nrm * sizeof( float );
        memSize += numVertices * shape->n_elem_tex * sizeof( float );
        memSize += numIndices * sizeof( unsigned );

        void* mem = BX_MALLOC( allocator, memSize, 4 ); // util::default_allocator()->allocate( memSize ); //memory_alloc_aligned( memSize, sizeof(void*) );
        memset( mem, 0, memSize );

        shape->positions = (float*)mem;
        shape->normals = shape->positions + numVertices * shape->n_elem_pos;
        shape->tangents = shape->normals + numVertices * shape->n_elem_nrm;
        shape->bitangents = shape->tangents + numVertices * shape->n_elem_nrm;
        shape->texcoords = shape->bitangents + numVertices * shape->n_elem_nrm;
        shape->indices = (unsigned*)(shape->texcoords + numVertices * shape->n_elem_tex);

        SYS_ASSERT( (uintptr_t)(shape->indices + numIndices) == (uintptr_t)((unsigned char*)mem + memSize) );

        shape->num_vertices = numVertices;
        shape->num_indices = numIndices;

        shape->allocator = allocator;
    }

    void deallocateShape( poly_shape_t* shape )
    {
        BXIAllocator* allocator = shape->allocator;
        BX_FREE( allocator, shape->positions );
        new(shape) poly_shape_t();
        //memset( shape, 0, sizeof(bxPolyShape) );
    }


    void copy( poly_shape_t* dst, int vOffset, int iOffset, const poly_shape_t& src )
    {
        SYS_ASSERT( dst->num_vertices >= (src.num_vertices + vOffset) );
        SYS_ASSERT( dst->num_indices >= (src.num_indices + iOffset) );

        SYS_ASSERT( dst->n_elem_pos == src.n_elem_pos );
        SYS_ASSERT( dst->n_elem_nrm == src.n_elem_nrm );
        SYS_ASSERT( dst->n_elem_tex == src.n_elem_tex );

        float* dstPos = dst->positions + vOffset*dst->n_elem_pos;
        float* dstNrm = dst->normals + vOffset*dst->n_elem_nrm;
        float* dstTan = dst->tangents + vOffset*dst->n_elem_nrm;
        float* dstBit = dst->bitangents + vOffset*dst->n_elem_nrm;
        float* dstTex = dst->texcoords + vOffset*dst->n_elem_tex;
        unsigned* dstIdx = dst->indices + iOffset;

        memcpy( dstPos, src.positions, src.num_vertices * src.n_elem_pos * sizeof( float ) );
        memcpy( dstNrm, src.normals, src.num_vertices * src.n_elem_nrm * sizeof( float ) );
        memcpy( dstTan, src.tangents, src.num_vertices * src.n_elem_nrm * sizeof( float ) );
        memcpy( dstBit, src.bitangents, src.num_vertices * src.n_elem_nrm * sizeof( float ) );
        memcpy( dstTex, src.texcoords, src.num_vertices * src.n_elem_tex * sizeof( float ) );
        memcpy( dstIdx, src.indices, src.num_indices * sizeof( unsigned ) );
    }


    void copy( poly_shape_t* dst, const poly_shape_t& src )
    {
        copy( dst, 0, 0, src );
    }

    void createCube( poly_shape_t* shape, const int iterations[6], float extent )
    {
        const float a = extent;
        const Vec3 baseCube[8] =
        {
            Vec3( -a, -a, a ),
            Vec3( a, -a, a ),
            Vec3( a,  a, a ),
            Vec3( -a,  a, a ),

            Vec3( -a, -a,-a ),
            Vec3( a, -a,-a ),
            Vec3( a,  a,-a ),
            Vec3( -a,  a,-a ),
        };

        const int NUM_SIDES = 6;
        const int quads[NUM_SIDES][4] =
        {
            0, 1, 2, 3, // f
            1, 5, 6, 2, // r
            5, 4, 7, 6, // b
            4, 0, 3, 7, // l
            3, 2, 6, 7, // t
            4, 5, 1, 0, // bt
        };

        Vec3* outPos = (Vec3*)shape->positions;
        Vec2* outTex = (Vec2*)shape->texcoords;
        unsigned* outIndices = shape->indices;

        int pointIndex = 0;
        int indexIndex = 0;
        for( int s = 0; s < NUM_SIDES; ++s )
        {
            const int iter = iterations[s];
            const int stepsI = iter + 1;
            const float step = 1.f / (float)iter;

            const int* sideQuad = quads[s];
            const Vec3 lb = baseCube[sideQuad[0]];
            const Vec3 rb = baseCube[sideQuad[1]];
            const Vec3 lu = baseCube[sideQuad[3]];
            const Vec3 ru = baseCube[sideQuad[2]];

            const int baseIndex = pointIndex;

            for( int y = 0; y < stepsI; ++y )
            {
                const float baseT = (float)y * step;
                const Vec3 base0 = lerp( baseT, lb, lu );
                const Vec3 base1 = lerp( baseT, rb, ru );

                for( int x = 0; x < stepsI; ++x )
                {
                    const float t = (float)x * step;
                    const Vec3 pos = lerp( t, base0, base1 );

                    outPos[pointIndex] = pos;
                    outTex[pointIndex] = Vec2( t, baseT );
                    ++pointIndex;
                }
            }

            for( int y = 0; y < stepsI - 1; ++y )
            {
                const int level0 = baseIndex + stepsI*y;
                const int level1 = baseIndex + stepsI*(y + 1);
                for( int x = 0; x < stepsI - 1; ++x )
                {
                    const int i0 = level0 + x;
                    const int i1 = i0 + 1;
                    const int i3 = level1 + x;
                    const int i2 = i3 + 1;

                    outIndices[indexIndex + 0] = i0;
                    outIndices[indexIndex + 1] = i1;
                    outIndices[indexIndex + 2] = i2;

                    outIndices[indexIndex + 3] = i0;
                    outIndices[indexIndex + 4] = i2;
                    outIndices[indexIndex + 5] = i3;

                    indexIndex += 6;
                }
            }
        }



        SYS_ASSERT( pointIndex <= shape->num_vertices );
        SYS_ASSERT( indexIndex <= shape->num_indices );
    }

    void normalize( poly_shape_t* shape, int begin, int count, float tolerance )
    {
#ifdef SYS_ASSERTIONS_ENABLED
        const int n = shape->num_vertices;
        SYS_ASSERT( (begin + count) <= n );
#endif //
        for( int i = begin; i < count; ++i )
        {
            Vec3* pos = (Vec3*)shape->position( i );
            const Vec3 posCpy = *pos;
            *pos = normalize( posCpy, tolerance );
        }
    }
    void transform( poly_shape_t* shape, int begin, int count, const mat44_t& tr )
    {
#ifdef SYS_ASSERTIONS_ENABLED
        const int n = shape->num_vertices;
        SYS_ASSERT( (begin + count) <= n );
#endif //

        for( int i = begin; i < count; ++i )
        {
            float* pos = shape->position( i );

            vec3_t point( pos[0], pos[1], pos[2] );
            point = mul_as_point( tr, point );

            pos[0] = point.x;
            pos[1] = point.y;
            pos[2] = point.z;
        }
    }

    int computeNumPoints( const int iterations[6] )
    {
        int sum = 0;
        for( int i = 0; i < 6; ++i )
        {
            sum += computeNumPointsPerSide( iterations[i] );
        }
        return sum;
    }
    int computeNumTriangles( const int iterations[6] )
    {
        int sum = 0;
        for( int i = 0; i < 6; ++i )
        {
            sum += computeNumTrianglesPerSide( iterations[i] );
        }
        return sum;
    }
    int computeNumPointsPerSide( int iterations )
    {
        const int x = iterations + 1;
        return x*x;
    }

    int computeNumTrianglesPerSide( int iterations )
    {
        const int x = iterations;
        return x*x * 2;
    }

    void allocateCube( poly_shape_t* shape, const int iterations[6], BXIAllocator* allocator )
    {

        const int numVertices = computeNumPoints( iterations );
        const int numTriangles = computeNumTriangles( iterations );
        allocateShape( shape, numVertices, numTriangles * 3, allocator );
    }

    void createBox( poly_shape_t* shape, int iterations, BXIAllocator* allocator )
    {
        const int boxIterations[6] =
        {
            iterations, iterations, iterations, iterations, iterations, iterations,
        };

        allocateCube( shape, boxIterations, allocator );
        createCube( shape, boxIterations, 0.5f );
        generateFlatNormals( shape, 0, shape->ntriangles() );
        computeTangentSpace( shape );
    }


    void createShpere( poly_shape_t* shape, int iterations, BXIAllocator* allocator )
    {
        int sphereIterations[6] =
        {
            iterations, iterations, iterations, iterations, iterations, iterations,
        };

        mat44_t scale = mat44_t::identity();
        scale *= 0.5f;

        allocateCube( shape, sphereIterations, allocator );
        createCube( shape, sphereIterations, 0.5f );
        normalize( shape, 0, shape->num_vertices, FLT_MIN );
        transform( shape, 0, shape->num_vertices, scale );
        generateSmoothNormals1( shape, 0, shape->num_vertices );
        generateSphereUV( shape, 0, shape->num_vertices );
        computeTangentSpace( shape );
    }

    void createCapsule( poly_shape_t* shape, int surfaces[6], int vertexCount[3], int iterations, BXIAllocator* allocator )
    {
        poly_shape_t topCap;
        poly_shape_t bottomCap;
        poly_shape_t cylinder;

        const int topCapIterations[6] =
        {
            iterations, iterations, iterations, iterations,
            iterations, 1,
        };
        const int bottomCapIterations[6] =
        {
            iterations, iterations, iterations, iterations,
            1, iterations,
        };
        const int cylinderIterations[6] =
        {
            iterations, iterations, iterations, iterations,
            1, 1,
        };

        allocateCube( &topCap, topCapIterations, allocator );
        allocateCube( &bottomCap, bottomCapIterations, allocator );
        allocateCube( &cylinder, cylinderIterations, allocator );

        createCube( &topCap, topCapIterations );
        createCube( &bottomCap, bottomCapIterations );
        createCube( &cylinder, cylinderIterations );

        const float capDisplacement = 1.0f;
        {
            mat44_t tr( mat33_t::identity(), vec3_t( 0.f, capDisplacement, 0.f ) );
            transform( &topCap, 0, topCap.num_vertices, tr );
            normalize( &topCap, 0, topCap.num_vertices );

            tr.c3.set_xyz( vec3_t( 0.f, 0.5f, 0.f ) );
            transform( &topCap, 0, topCap.num_vertices, tr );
            generateSmoothNormals1( &topCap, 0, topCap.num_vertices );
        }

        {
            mat44_t tr( mat33_t::identity(), vec3_t( 0.f, -capDisplacement, 0.f ) );
            transform( &bottomCap, 0, bottomCap.num_vertices, tr );
            normalize( &bottomCap, 0, bottomCap.num_vertices );
            tr.c3.set_xyz( vec3_t( 0.f, -0.5f, 0.f ) );
            transform( &bottomCap, 0, bottomCap.num_vertices, tr );
            generateSmoothNormals1( &bottomCap, 0, bottomCap.num_vertices );

            const int offset = topCap.num_vertices;
            for( int i = 0; i < bottomCap.num_indices; ++i )
            {
                bottomCap.indices[i] += offset;
            }
        }

        {
            const int n = cylinder.num_vertices;
            for( int i = 0; i < n; ++i )
            {
                float* pos = shape->position( i );
                const Vec3 posToNormalize( pos[0], 0.f, pos[2] );
                const Vec3 tmp = normalize( posToNormalize );
                const Vec3 finalPos( tmp.x, pos[1] * 0.5f, tmp.z );

                pos[0] = finalPos.x;
                pos[1] = finalPos.y;
                pos[2] = finalPos.z;
            }

            generateSmoothNormals1( &cylinder, 0, cylinder.num_vertices );

            const int offset = topCap.num_vertices + bottomCap.num_vertices;
            for( int i = 0; i < cylinder.num_indices; ++i )
            {
                cylinder.indices[i] += offset;
            }
        }

        const int totalNumVertices = topCap.num_vertices + bottomCap.num_vertices + cylinder.num_vertices;
        const int totalNumIndices = topCap.num_indices + bottomCap.num_indices + cylinder.num_indices;

        allocateShape( shape, totalNumVertices, totalNumIndices, allocator );
        copy( shape, 0, 0, topCap );
        copy( shape, topCap.num_vertices, topCap.num_indices, bottomCap );
        copy( shape, topCap.num_vertices + bottomCap.num_vertices, topCap.num_indices + bottomCap.num_indices, cylinder );

        computeTangentSpace( shape );

        //generateSmoothNormals( shape, 0, getNTriangles( *shape ) );
        //generateSphereUV( shape, 0, shape->numVertices );
        surfaces[0] = 0;
        surfaces[1] = topCap.num_indices;
        surfaces[2] = topCap.num_indices;
        surfaces[3] = bottomCap.num_indices;
        surfaces[4] = topCap.num_indices + bottomCap.num_indices;
        surfaces[5] = cylinder.num_indices;

        vertexCount[0] = topCap.num_vertices;
        vertexCount[1] = bottomCap.num_vertices;
        vertexCount[2] = cylinder.num_vertices;

        deallocateShape( &cylinder );
        deallocateShape( &bottomCap );
        deallocateShape( &topCap );

    }

    static Vec3 _get_triangle_normal( const Vec3& p0, const Vec3& p1, const Vec3& p2 )
    {
        const Vec3 a = p1 - p0;
        const Vec3 b = p2 - p0;

        const Vec3 n = cross( a, b );
        return n;
    }

    void generateSphereUV( poly_shape_t* shape, int firstPoint, int count )
    {
        SYS_ASSERT( (firstPoint + count) <= shape->num_vertices );

        const int begin = firstPoint;
        const int end = firstPoint + count;
        const float one_over_2PI = 1.f / (2.f*PI);


        for( int i = begin; i < end; ++i )
        {
            const Vec3 point = *(Vec3*)shape->position( i );
            const Vec3 pointNrm = normalize( point );
            const float u = atan2f( pointNrm.z, pointNrm.x ) * one_over_2PI;
            const float v = asinf( pointNrm.y ) / PI;

            //const float u = 0.5f + asin( pointNrm.x ) / PI;
            //const float v = 0.5f + asin( pointNrm.y ) / PI;

            float* uv = shape->texcoord( i );
            uv[0] = u;
            uv[1] = v;
        }
    }

    void generateFlatNormals( poly_shape_t* shape, int firstTriangle, int count )
    {
        const int numTris = shape->ntriangles();
        SYS_ASSERT( (firstTriangle + count) <= numTris );

        const int end = firstTriangle + count;
        for( int i = firstTriangle; i < end; ++i )
        {
            const unsigned* tri = shape->triangle( i );

            const Vec3* p0 = (Vec3*)shape->position( tri[0] );
            const Vec3* p1 = (Vec3*)shape->position( tri[1] );
            const Vec3* p2 = (Vec3*)shape->position( tri[2] );

            const Vec3 n = _get_triangle_normal( *p0, *p1, *p2 );

            Vec3* n0 = (Vec3*)shape->normal( tri[0] );
            Vec3* n1 = (Vec3*)shape->normal( tri[1] );
            Vec3* n2 = (Vec3*)shape->normal( tri[2] );

            *n0 = n;
            *n1 = n;
            *n2 = n;
        }

        for( int i = 0; i < shape->num_vertices; ++i )
        {
            Vec3* dstNrm = (Vec3*)shape->normal( i );
            const Vec3 srcNrm = *dstNrm;
            const Vec3 nrm = normalize( srcNrm );

            *dstNrm = nrm;
        }
    }

    void generateSmoothNormals( poly_shape_t* shape, int firstTriangle, int count )
    {
        const int numTris = shape->ntriangles();
        SYS_ASSERT( (firstTriangle + count) <= numTris );

        const int end = firstTriangle + count;
        for( int i = firstTriangle; i < end; ++i )
        {
            const unsigned* tri = shape->triangle( i );

            const Vec3* p0 = (Vec3*)shape->position( tri[0] );
            const Vec3* p1 = (Vec3*)shape->position( tri[1] );
            const Vec3* p2 = (Vec3*)shape->position( tri[2] );

            const Vec3 n = _get_triangle_normal( *p0, *p1, *p2 );

            Vec3* n0 = (Vec3*)shape->normal( tri[0] );
            Vec3* n1 = (Vec3*)shape->normal( tri[1] );
            Vec3* n2 = (Vec3*)shape->normal( tri[2] );

            *n0 = *n0 + n;
            *n1 = *n1 + n;
            *n2 = *n2 + n;
        }

        for( int i = 0; i < shape->num_vertices; ++i )
        {
            Vec3* dstNrm = (Vec3*)shape->normal( i );
            const Vec3 srcNrm = *dstNrm;
            const Vec3 nrm = normalize( srcNrm );

            *dstNrm = nrm;
        }
    }
    void generateSmoothNormals1( poly_shape_t* shape, int firstPoint, int count )
    {
        SYS_ASSERT( (firstPoint + count) <= shape->num_vertices );

        const int begin = firstPoint;
        const int end = firstPoint + count;

        for( int i = begin; i < end; ++i )
        {
            const float* p = shape->position( i );

            const Vec3 p3( p[0], p[1], p[2] );
            const Vec3 nrm = normalize( p3 );

            float* n = shape->normal( i );
            n[0] = nrm.x;
            n[1] = nrm.y;
            n[2] = nrm.z;
        }

        for( int i = 0; i < shape->num_vertices; ++i )
        {
            Vec3* dstNrm = (Vec3*)shape->normal( i );
            const Vec3 srcNrm = *dstNrm;
            const Vec3 nrm = normalize( srcNrm );

            *dstNrm = nrm;
        }

    }

    void computeTangentSpace( poly_shape_t* shape )
    {
        Vec3* tmpTan1 = new Vec3[shape->num_vertices];
        Vec3* tmpTan2 = new Vec3[shape->num_vertices];
        float* w = new float[shape->num_vertices];

        memset( tmpTan1, 0, shape->nvertices() * sizeof( Vec3 ) );
        memset( tmpTan2, 0, shape->nvertices() * sizeof( Vec3 ) );
        memset( w, 0, shape->nvertices() * sizeof( float ) );

        const int begin = 0;
        const int end = shape->ntriangles();

        for( int i = begin; i < end; ++i )
        {
            const Triangle* tri = (Triangle*)shape->triangle( i );
            const Vec3& v1 = *(Vec3*)shape->position( tri->p0 );
            const Vec3& v2 = *(Vec3*)shape->position( tri->p1 );
            const Vec3& v3 = *(Vec3*)shape->position( tri->p2 );

            const Vec2& w1 = *(Vec2*)shape->texcoord( tri->p0 );
            const Vec2& w2 = *(Vec2*)shape->texcoord( tri->p1 );
            const Vec2& w3 = *(Vec2*)shape->texcoord( tri->p2 );

            const float x1 = v2.x - v1.x;
            const float x2 = v3.x - v1.x;
            const float y1 = v2.y - v1.y;
            const float y2 = v3.y - v1.y;
            const float z1 = v2.z - v1.z;
            const float z2 = v3.z - v1.z;

            const float s1 = w2.x - w1.x;
            const float s2 = w3.x - w1.x;
            const float t1 = w2.y - w1.y;
            const float t2 = w3.y - w1.y;

            const float r = 1.0f / (s1 * t2 - s2 * t1);

            const Vec3 sdir( (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r );
            const Vec3 tdir( (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r );

            tmpTan1[tri->p0] += sdir;
            tmpTan1[tri->p1] += sdir;
            tmpTan1[tri->p2] += sdir;

            tmpTan2[tri->p0] += tdir;
            tmpTan2[tri->p1] += tdir;
            tmpTan2[tri->p2] += tdir;
        }

        for( int i = 0; i < shape->num_vertices; ++i )
        {
            const Vec3& n = *(Vec3*)shape->normal( i );
            const Vec3& t = tmpTan1[i];

            Vec3 tangent = normalize( t - n * dot( n, t ) );
            w[i] = (dot( cross( n, t ), tmpTan2[i] ) < 0.f) ? -1.f : 1.f;

            float* tan = shape->tangent( i );
            tan[0] = tangent.x;
            tan[1] = tangent.y;
            tan[2] = tangent.z;
        }

        for( int i = 0; i < shape->num_vertices; ++i )
        {
            const Vec3& n = *(Vec3*)shape->normal( i );
            const Vec3& t = *(Vec3*)shape->tangent( i );
            float* bit = shape->bitangent( i );

            const Vec3 b = normalize( cross( n, t ) * w[i] );

            bit[0] = b.x;
            bit[1] = b.y;
            bit[2] = b.z;
        }

        delete[] w;
        delete[] tmpTan2;
        delete[] tmpTan1;

    }
}//
//////////////////////////////////////////////////////////////////////////
/// debug code
/*
static picoPolyShape::bxPolyShape pBox;
static picoPolyShape::bxPolyShape pSphere;
static picoPolyShape::bxPolyShape pCapsule;
static int pCapsuleSurfaces[6];
if( pBox.positions == 0 )
{
picoPolyShape::createBox( &pBox, 1 );
picoPolyShape::createShpere( &pSphere, 5 );
picoPolyShape::createCapsule( &pCapsule, pCapsuleSurfaces, 5 );
}




picoPolyShape::bxPolyShape* shape = &pCapsule;
const int begin = pCapsuleSurfaces[4] / 3;
const int end = pCapsuleSurfaces[5] / 3;

for( int i = begin; i < begin + end; ++i )
{
const unsigned* tri = picoPolyShape::getTriangle( *shape, i );

const float* point0 = picoPolyShape::getPosition( *shape, tri[0] );
const float* point1 = picoPolyShape::getPosition( *shape, tri[1] );
const float* point2 = picoPolyShape::getPosition( *shape, tri[2] );

Vector3 pv0, pv1, pv2;
load_xyz( pv0, point0 );
load_xyz( pv1, point1 );
load_xyz( pv2, point2 );

picoDebugDraw::addLine( pv0, pv1, 0xff00ff00, 1.f, picoDebugDraw::eSpace_world, true );
picoDebugDraw::addLine( pv0, pv2, 0xff00ff00, 1.f, picoDebugDraw::eSpace_world, true );
picoDebugDraw::addLine( pv1, pv2, 0xff00ff00, 1.f, picoDebugDraw::eSpace_world, true );
}
*/

//#include "marching_cubes/CIsoSurface.h"
//
//inline __m128 vec_floor_sse2( __m128 x )
//{
//	const __m128i as_int = _mm_cvttps_epi32( x );
//	return _mm_cvtepi32_ps( as_int );
//}
//
//void scalar_field_create( ScalarField* sfield, const Vector3* points, int n_points, int n_cells_x, int n_cells_y, int n_cells_z )
//{
//	const int n_samples = ( n_cells_x ) * ( n_cells_y ) * ( n_cells_z );
//
//    util::AABB bbox = util::aabb::prepare();
//    for( int ipoint = 0; ipoint < n_points; ++ipoint )
//    {
//        bbox = util::aabb::extend( bbox, points[ipoint] );
//    }
//
//    const Vector3 n_cells_v3 = Vector3( float(n_cells_x), float(n_cells_y), float(n_cells_z) );
//    Vector3 bbox_size = util::aabb::size( bbox );
//	Vector3 cell_size = divPerElem( bbox_size, n_cells_v3 );
//	bbox.min -= cell_size;
//	bbox.max += cell_size;
//
//	bbox_size = util::aabb::size( bbox );
//	cell_size = divPerElem( bbox_size, n_cells_v3 );
//
//    float* samples = (float*)util::default_allocator()->allocate( n_samples * sizeof(float), 4 );
//    memset( samples, 0, n_samples * sizeof(float) );
//
//    Grid3D grid( n_cells_x, n_cells_y, n_cells_z );
//    float min_value = TYPE_FLT_MAX;
//    float max_value =-TYPE_FLT_MAX;
//    for( int ipoint = 0; ipoint < n_points; ++ipoint )
//    {
//        const Vector3& point = points[ipoint];
//        const Vector3 local_point = point - bbox.min + cell_size * half_vec;
//		const Vector3 cell_coords_v3f = divPerElem( local_point, cell_size );
//		const Vector3 cell_coords_v3( vec_floor_sse2( cell_coords_v3f.get128() ) );
//
//		const util::SSEScalar cell_coords_scalar( cell_coords_v3.get128() );
//
//        int cell_coords[] = 
//        {
//            (int)cell_coords_scalar.as_float[0],
//            (int)cell_coords_scalar.as_float[1],
//            (int)cell_coords_scalar.as_float[2],
//        };
//        SYS_ASSERT( cell_coords[0] >= 0 && cell_coords[0] < n_cells_x );
//        SYS_ASSERT( cell_coords[1] >= 0 && cell_coords[1] < n_cells_y );
//        SYS_ASSERT( cell_coords[2] >= 0 && cell_coords[2] < n_cells_z );
//
//		//const Vector3 local_point_uvt = cell_coords_v3f - cell_coords_v3;
//		//const Vector3 local_point_uvt_m11 = local_point_uvt*2.f - Vector3( 1.f );
//		//const float value = length( Vector3( 1.f ) - local_point_uvt ).getAsFloat();
//		const float value = point.getElem( 3 ).getAsFloat();
// 		const int sample_index = grid.index( cell_coords[0], cell_coords[1], cell_coords[2] );
//		SYS_ASSERT( sample_index < n_samples );
//		samples[sample_index] += value;
//
//		min_value = util::min_of_pair( samples[sample_index], min_value );
//		max_value = util::max_of_pair( samples[sample_index], max_value );
//
//        //const int sample_indices[] = 
//        //{
//        //    grid.index( cell_coords[0]  , cell_coords[1]  , cell_coords[2] ),
//        //    grid.index( cell_coords[0]+1, cell_coords[1]  , cell_coords[2] ),
//        //    grid.index( cell_coords[0]+1, cell_coords[1]+1, cell_coords[2] ),
//        //    grid.index( cell_coords[0]  , cell_coords[1]+1, cell_coords[2] ),
//
//        //    grid.index( cell_coords[0]  , cell_coords[1]  , cell_coords[2]+1 ),
//        //    grid.index( cell_coords[0]+1, cell_coords[1]  , cell_coords[2]+1 ),
//        //    grid.index( cell_coords[0]+1, cell_coords[1]+1, cell_coords[2]+1 ),
//        //    grid.index( cell_coords[0]  , cell_coords[1]+1, cell_coords[2]+1 ),
//        //};
//
//        //Vector3 cell_corners[8] = 
//        //{
//        //    Vector3( cell_coords[0]  , cell_coords[1]  , cell_coords[2] ),
//        //    Vector3( cell_coords[0]+1, cell_coords[1]  , cell_coords[2] ),
//        //    Vector3( cell_coords[0]+1, cell_coords[1]+1, cell_coords[2] ),
//        //    Vector3( cell_coords[0]  , cell_coords[1]+1, cell_coords[2] ),
//
//        //    Vector3( cell_coords[0]  , cell_coords[1]  , cell_coords[2]+1 ),
//        //    Vector3( cell_coords[0]+1, cell_coords[1]  , cell_coords[2]+1 ),
//        //    Vector3( cell_coords[0]+1, cell_coords[1]+1, cell_coords[2]+1 ),
//        //    Vector3( cell_coords[0]  , cell_coords[1]+1, cell_coords[2]+1 ),
//        //};
//
//        //util::AABB cell_bbox = util::aabb::prepare();
//        //for( int icorner = 0; icorner < 8; ++icorner )
//        //{
//        //    cell_corners[icorner] = mulPerElem( cell_corners[icorner], cell_size );
//        //    cell_bbox = util::aabb::extend( cell_bbox, cell_corners[icorner] );
//        //}
//
//        //
//        //const Vector3 point_uvt = local_point_uvt; //divPerElem( local_point - cell_bbox.min, cell_size );
//        //const Vector3 point_1m_uvt = Vector3(1.f) - point_uvt;
//
//        //const Vector3 corner_points[] = 
//        //{
//        //    Vector3( point_uvt.getX()   , point_uvt.getY()   , point_uvt.getZ() ),
//        //    Vector3( point_1m_uvt.getX(), point_uvt.getY()   , point_uvt.getZ() ),
//        //    Vector3( point_1m_uvt.getX(), point_1m_uvt.getY(), point_uvt.getZ() ),
//        //    Vector3( point_uvt.getX()   , point_1m_uvt.getY(), point_uvt.getZ() ),
//
//        //    Vector3( point_uvt.getX()   , point_uvt.getY()   , point_1m_uvt.getZ() ),
//        //    Vector3( point_1m_uvt.getX(), point_uvt.getY()   , point_1m_uvt.getZ() ),
//        //    Vector3( point_1m_uvt.getX(), point_1m_uvt.getY(), point_1m_uvt.getZ() ),
//        //    Vector3( point_uvt.getX()   , point_1m_uvt.getY(), point_1m_uvt.getZ() ),
//
//        //};
//
//        //for( int iidex = 0; iidex < 8; ++iidex )
//        //{
//        //    SYS_ASSERT( sample_indices[iidex] < n_samples );
//        //    const int sample_index = sample_indices[iidex];
//        //    const float sample_value = length( corner_points[iidex] ).getAsFloat();
//        //    samples[sample_index] += sample_value;
//        //    
//        //    min_value = util::min_of_pair( samples[sample_index], min_value );
//        //    max_value = util::max_of_pair( samples[sample_index], max_value );
//
//        //}
//    }
//
//    sfield->n_samples = n_samples;
//    sfield->samples = samples;
//    sfield->bbox = bbox;
//    sfield->grid = grid;
//    sfield->min_value = min_value;
//    sfield->max_value = max_value;
//    util::m128_to_xyz( sfield->cell_size, cell_size.get128() );
//}
//void scalar_field_release( ScalarField* sfield )
//{
//	DEALLOCATE0( util::default_allocator(), sfield->samples );
//	memset( sfield, 0, sizeof( ScalarField ) );
//}
//void scalar_field_normalize( ScalarField* sfield, normalize_func* nrm_function, float minimum /*= 0.f*/, float maximum /*= 1.f */ )
//{
//    const float a = 0.f;
//    const float b = sfield->max_value;
//    for( int isample = 0; isample < sfield->n_samples; ++isample )
//    {
//        const float value = sfield->samples[isample];
//        sfield->samples[isample] = util::clamp( (*nrm_function)( a, b, value ), minimum, maximum );
//    }
//}
//
//void iso_surface_create( IsoSurface* isosurf, const ScalarField& sfield, float iso_level )
//{
//	CIsoSurface<float> ciso;
//	ciso.GenerateSurface( sfield.samples, iso_level, sfield.grid.width-1, sfield.grid.height-1, sfield.grid.depth-1, sfield.cell_size[0], sfield.cell_size[1], sfield.cell_size[2] );
//	SYS_STATIC_ASSERT( sizeof( POINT3D ) == 12 );
//
//	const int n_points = ciso.m_nVertices;
//	const int n_indices = ciso.m_nTriangles * 3;
//
//	const int point_stride = 3 * sizeof( f32 );
//
//	int mem_size = 0;
//	mem_size += n_points * point_stride;
//	mem_size += n_points * point_stride;
//	mem_size += n_indices * sizeof( unsigned );
//
//	void* mem_handle = util::default_allocator()->allocate( mem_size, 4 );
//	memset( mem_handle, 0, mem_size );
//
//	isosurf->points = (f32*)mem_handle;
//	isosurf->normals = isosurf->points + n_points * 3;
//	isosurf->indices = (unsigned*)(isosurf->normals + n_points * 3);
//	SYS_ASSERT( uptr( (u8*)mem_handle + mem_size ) == uptr( isosurf->indices + n_indices ) );
//
//	isosurf->n_points = n_points;
//	isosurf->n_indices = n_indices;
//
//	memcpy( isosurf->points, ciso.m_ppt3dVertices, n_points * point_stride );
//	memcpy( isosurf->normals, ciso.m_pvec3dNormals, n_points * point_stride );
//	memcpy( isosurf->indices, ciso.m_piTriangleIndices, n_indices * sizeof( unsigned ) );
//}
//void iso_surface_release( IsoSurface* isosurf )
//{
//	DEALLOCATE0( util::default_allocator(), isosurf->points );
//	memset( isosurf, 0, sizeof( IsoSurface ) );
//}

