// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2013-2016 NVIDIA Corporation. All rights reserved.

#include "aabbtree.h"

#include <assert.h>
#include <algorithm>
#include <iostream>

using namespace std;

AABBTree::AABBTree( array_span_t<const Vec3> vertices, array_span_t<const u16> indices, uint32_t numFaces )
    : m_vertices(vertices)
    , m_indices(indices)
    , m_numFaces(numFaces)
{
    // build stats
    m_treeDepth = 0;
    m_innerNodes = 0;
    m_leafNodes = 0;

    Build();
}

namespace
{

	struct FaceSorter
	{
		FaceSorter(const Vec3* positions, const uint16_t* indices, uint32_t n, uint32_t axis) 
			: m_vertices(positions)
			, m_indices(indices)
			, m_numIndices(n)
			, m_axis(axis)
		{        
		}

		inline bool operator()(uint32_t lhs, uint32_t rhs) const
		{
			float a = GetCentroid(lhs);
			float b = GetCentroid(rhs);

			if (a == b)
				return lhs < rhs;
			else
				return a < b;
		}

		inline float GetCentroid(uint32_t face) const
		{
			const Vec3& a = m_vertices[m_indices[face*3+0]];
			const Vec3& b = m_vertices[m_indices[face*3+1]];
			const Vec3& c = m_vertices[m_indices[face*3+2]];

			return (a[m_axis] + b[m_axis] + c[m_axis])/3.0f;
		}

		const Vec3* m_vertices;
		const uint16_t* m_indices;
		uint32_t m_numIndices;
		uint32_t m_axis;
	};
	
	inline uint32_t LongestAxis(const Vector3& v)
	{    
		if (v.x > v.y && v.x > v.z)
			return 0;
		else
			return (v.y > v.z) ? 1 : 2;
	}

} // anonymous namespace

void AABBTree::CalculateFaceBounds( const uint16_t* faces, uint32_t numFaces, Vector3& outMinExtents, Vector3& outMaxExtents)
{
    Vector3 minExtents(FLT_MAX);
    Vector3 maxExtents(-FLT_MAX);

    // calculate face bounds
    for (uint32_t i=0; i < numFaces; ++i)
    {
        Vector3 a = Vector3(m_vertices[m_indices[faces[i]*3+0]]);
        Vector3 b = Vector3(m_vertices[m_indices[faces[i]*3+1]]);
        Vector3 c = Vector3(m_vertices[m_indices[faces[i]*3+2]]);

        minExtents = Min(a, minExtents);
        maxExtents = Max(a, maxExtents);

        minExtents = Min(b, minExtents);
        maxExtents = Max(b, maxExtents);

        minExtents = Min(c, minExtents);
        maxExtents = Max(c, maxExtents);
    }

    outMinExtents = minExtents;
    outMaxExtents = maxExtents;
}

// track current tree depth
static uint32_t s_depth = 0;

void AABBTree::Build()
{
    assert(m_numFaces*3);

    const uint32_t numFaces = m_numFaces;

    // build initial list of faces
    m_faces.reserve(numFaces);

    // calculate bounds of each face and store
    m_faceBounds.reserve(numFaces);   
    
	std::vector<Bounds> stack;
	for (uint16_t i=0; i < numFaces; ++i)
    {
		Bounds top;
        CalculateFaceBounds(&i, 1, top.m_min, top.m_max);
		
		m_faces.push_back(i);
		m_faceBounds.push_back(top);
    }

	m_nodes.reserve(uint32_t(numFaces*1.5f));

    // allocate space for all the nodes
	m_freeNode = 1;

    // start building
    BuildRecursive(0, &m_faces[0], numFaces);

    assert(s_depth == 0);

    FaceBoundsArray f;
    m_faceBounds.swap(f);
}

// partion faces around the median face
uint32_t AABBTree::PartitionMedian(Node& n, uint16_t* faces, uint32_t numFaces)
{
	FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces*3, LongestAxis(n.m_maxExtents-n.m_minExtents));
    std::nth_element(faces, faces+numFaces/2, faces+numFaces, predicate);

	return numFaces/2;
}

// partion faces based on the surface area heuristic
uint32_t AABBTree::PartitionSAH(Node& n, uint16_t* faces, uint32_t numFaces)
{
	uint32_t bestAxis = 0;
	uint32_t bestIndex = 0;
	float bestCost = FLT_MAX;

	for (uint32_t a=0; a < 3; ++a)	
	//uint32_t a = bestAxis;
	{
		// sort faces by centroids
		FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces*3, a);
		std::sort(faces, faces+numFaces, predicate);

		// two passes over data to calculate upper and lower bounds
		vector<float> cumulativeLower(numFaces);
		vector<float> cumulativeUpper(numFaces);

		Bounds lower;
		Bounds upper;

		for (uint32_t i=0; i < numFaces; ++i)
		{
			lower.Union(m_faceBounds[faces[i]]);
			upper.Union(m_faceBounds[faces[numFaces-i-1]]);

			cumulativeLower[i] = lower.GetSurfaceArea();        
			cumulativeUpper[numFaces-i-1] = upper.GetSurfaceArea();
		}

		float invTotalSA = 1.0f / cumulativeUpper[0];

		// test all split positions
		for (uint32_t i=0; i < numFaces-1; ++i)
		{
			float pBelow = cumulativeLower[i] * invTotalSA;
			float pAbove = cumulativeUpper[i] * invTotalSA;

			float cost = 0.125f + (pBelow*i + pAbove*(numFaces-i));
			if (cost <= bestCost)
			{
				bestCost = cost;
				bestIndex = i;
				bestAxis = a;
			}
		}
	}

	// re-sort by best axis
	FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces*3, bestAxis);
	std::sort(faces, faces+numFaces, predicate);

	return bestIndex+1;
}

void AABBTree::BuildRecursive(uint32_t nodeIndex, uint16_t* faces, uint32_t numFaces)
{
    const uint32_t kMaxFacesPerLeaf = 6;
    
    // if we've run out of nodes allocate some more
    if (nodeIndex >= m_nodes.size())
    {
		uint32_t s = std::max(uint32_t(1.5f*m_nodes.size()), 512U);

		//cout << "Resizing tree, current size: " << m_nodes.size()*sizeof(Node) << " new size: " << s*sizeof(Node) << endl;

        m_nodes.resize(s);
    }

    // a reference to the current node, need to be careful here as this reference may become invalid if array is resized
	Node& n = m_nodes[nodeIndex];

	// track max tree depth
    ++s_depth;
    m_treeDepth = max(m_treeDepth, s_depth);

	CalculateFaceBounds(faces, numFaces, n.m_minExtents, n.m_maxExtents);

	// calculate bounds of faces and add node  
    if (numFaces <= kMaxFacesPerLeaf)
    {
        n.m_faces = faces;
        n.m_numFaces = numFaces;		

        ++m_leafNodes;
    }
    else
    {
        ++m_innerNodes;        

        // face counts for each branch
        //const uint32_t leftCount = PartitionMedian(n, faces, numFaces);
        const uint32_t leftCount = PartitionSAH(n, faces, numFaces);
        const uint32_t rightCount = numFaces-leftCount;

		// alloc 2 nodes
		m_nodes[nodeIndex].m_children = m_freeNode;

		// allocate two nodes
		m_freeNode += 2;
  
        // split faces in half and build each side recursively
        BuildRecursive(m_nodes[nodeIndex].m_children+0, faces, leftCount);
        BuildRecursive(m_nodes[nodeIndex].m_children+1, faces+leftCount, rightCount);
    }

    --s_depth;
}

struct StackEntry
{
    uint32_t m_node;   
    float m_dist;
};


bool AABBTree::TraceRay(const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const
{   
    Vector3 rcp_dir(1.0f/dir.x, 1.0f/dir.y, 1.0f/dir.z);

    outT = FLT_MAX;
    TraceRecursive(0, start, dir, outT, u, v, w, faceSign, faceIndex);

    return (outT != FLT_MAX);
}

inline bool IntersectRayAABB( const Vec3& start, const Vector3& dir, const Vector3& min, const Vector3& max, float& t, Vector3* normal )
{
    //! calculate candidate plane on each axis
    float tx = -1.0f, ty = -1.0f, tz = -1.0f;
    bool inside = true;

    //! use unrolled loops

    //! x
    if( start.x < min.x )
    {
        if( dir.x != 0.0f )
            tx = (min.x - start.x) / dir.x;
        inside = false;
    }
    else if( start.x > max.x )
    {
        if( dir.x != 0.0f )
            tx = (max.x - start.x) / dir.x;
        inside = false;
    }

    //! y
    if( start.y < min.y )
    {
        if( dir.y != 0.0f )
            ty = (min.y - start.y) / dir.y;
        inside = false;
    }
    else if( start.y > max.y )
    {
        if( dir.y != 0.0f )
            ty = (max.y - start.y) / dir.y;
        inside = false;
    }

    //! z
    if( start.z < min.z )
    {
        if( dir.z != 0.0f )
            tz = (min.z - start.z) / dir.z;
        inside = false;
    }
    else if( start.z > max.z )
    {
        if( dir.z != 0.0f )
            tz = (max.z - start.z) / dir.z;
        inside = false;
    }

    //! if point inside all planes
    if( inside )
    {
        t = 0.0f;
        return true;
    }

    //! we now have t values for each of possible intersection planes
    //! find the maximum to get the intersection point
    float tmax = tx;
    int taxis = 0;

    if( ty > tmax )
    {
        tmax = ty;
        taxis = 1;
    }
    if( tz > tmax )
    {
        tmax = tz;
        taxis = 2;
    }

    if( tmax < 0.0f )
        return false;

    //! check that the intersection point lies on the plane we picked
    //! we don't test the axis of closest intersection for precision reasons

    //! no eps for now
    float eps = 0.0f;

    Vec3 hit = start + dir * tmax;

    if( (hit.x < min.x - eps || hit.x > max.x + eps) && taxis != 0 )
        return false;
    if( (hit.y < min.y - eps || hit.y > max.y + eps) && taxis != 1 )
        return false;
    if( (hit.z < min.z - eps || hit.z > max.z + eps) && taxis != 2 )
        return false;

    //! output results
    t = tmax;

    return true;
}

inline bool IntersectRayTriTwoSided( const Vec3& p, const Vec3& dir, const Vec3& a, const Vec3& b, const Vec3& c, float& t, float& u, float& v, float& w, float& sign )//Vec3* normal)
{
    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 n = cross( ab, ac );

    float d = dot( -dir, n );
    float ood = 1.0f / d; // No need to check for division by zero here as infinity aritmetic will save us...
    Vector3 ap = p - a;

    t = dot( ap, n ) * ood;
    if( t < 0.0f )
        return false;

    Vector3 e = cross( -dir, ap );
    v = dot( ac, e ) * ood;
    if( v < 0.0f || v > 1.0f ) // ...here...
        return false;
    w = -dot( ab, e ) * ood;
    if( w < 0.0f || v + w > 1.0f ) // ...and here
        return false;

    u = 1.0f - v - w;
    //if (normal)
        //*normal = n;
    sign = d;

    return true;
}

void AABBTree::TraceRecursive(uint32_t nodeIndex, const Vec3& start, const Vector3& dir, float& outT, float& outU, float& outV, float& outW, float& faceSign, uint32_t& faceIndex) const
{
	const Node& node = m_nodes[nodeIndex];

    if (node.m_faces == NULL)
    {
        // find closest node
        const Node& leftChild = m_nodes[node.m_children+0];
        const Node& rightChild = m_nodes[node.m_children+1];

        float dist[2] = {FLT_MAX, FLT_MAX};

        IntersectRayAABB(start, dir, leftChild.m_minExtents, leftChild.m_maxExtents, dist[0], NULL);
        IntersectRayAABB(start, dir, rightChild.m_minExtents, rightChild.m_maxExtents, dist[1], NULL);
        
        uint32_t closest = 0;
        uint32_t furthest = 1;
		
        if (dist[1] < dist[0])
        {
            closest = 1;
            furthest = 0;
        }		

        if (dist[closest] < outT)
            TraceRecursive(node.m_children+closest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);

        if (dist[furthest] < outT)
            TraceRecursive(node.m_children+furthest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);

    }
    else
    {
        Vector3 normal;
        float t, u, v, w, s;

        for (uint32_t i=0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i]*3;

            const Vec3& a = m_vertices[m_indices[indexStart+0]];
            const Vec3& b = m_vertices[m_indices[indexStart+1]];
            const Vec3& c = m_vertices[m_indices[indexStart+2]];

            if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s))
            {
                if (t < outT)
                {
                    outT = t;
					outU = u;
					outV = v;
					outW = w;
					faceSign = s;
					faceIndex = node.m_faces[i];
                }
            }
        }
    }
}

bool AABBTree::TraceRaySlow(const Vec3& start, const Vector3& dir, float& outT, float& outU, float& outV, float& outW, float& faceSign, uint32_t& faceIndex) const
{    
    const uint32_t numFaces = GetNumFaces();

    float minT, minU, minV, minW, minS;
	minT = minU = minV = minW = minS = FLT_MAX;

    Vector3 minNormal(0.0f, 1.0f, 0.0f);

    Vector3 n(0.0f, 1.0f, 0.0f);
    float t, u, v, w, s;
    bool hit = false;
	uint32_t minIndex = 0;

    for (uint32_t i=0; i < numFaces; ++i)
    {
        const Vec3& a = m_vertices[m_indices[i*3+0]];
        const Vec3& b = m_vertices[m_indices[i*3+1]];
        const Vec3& c = m_vertices[m_indices[i*3+2]];

        if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s))
        {
            if (t < minT)
            {
                minT = t;
				minU = u;
				minV = v;
				minW = w;
				minS = s;
                minNormal = n;
				minIndex = i;
                hit = true;
            }
        }
    }

    outT = minT;
	outU = minU;
	outV = minV;
	outW = minW;
	faceSign = minS;
	faceIndex = minIndex;

    return hit;
}
