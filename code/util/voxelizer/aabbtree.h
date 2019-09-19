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

#pragma once

#include <vector>

#include "../foundation/math/vmath_type.h"
#include "../foundation/math/vec3.h"
#include "foundation/containers.h"

using Vector3 = vec3_t;
using Vec3 = vec3_t;

template<typename Type>
inline Type Min( const Type& a, const Type& b ) { return (a < b) ? a : b; }

template<typename Type>
inline Type Max( const Type& a, const Type& b ) { return (a < b) ? b : a; }

template<>
inline vec3_t Min( const vec3_t& a, const vec3_t& b ) { return min_per_elem( a, b ); }

template<>
inline vec3_t Max( const vec3_t& a, const vec3_t& b ) { return max_per_elem( a, b ); }

class AABBTree
{
	AABBTree(const AABBTree&);
	AABBTree& operator=(const AABBTree&);

public:

    AABBTree( array_span_t<const Vec3> vertices, array_span_t<const u16> indices, uint32_t numFaces );

	bool TraceRaySlow(const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const;
    bool TraceRay(const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const;

    Vector3 GetCenter() const { return (m_nodes[0].m_minExtents+m_nodes[0].m_maxExtents)*0.5f; }
    Vector3 GetMinExtents() const { return m_nodes[0].m_minExtents; }
    Vector3 GetMaxExtents() const { return m_nodes[0].m_maxExtents; }
	
private:

    struct Node
    {
        Node() 	
            : m_numFaces(0)
            , m_faces(NULL)
            , m_minExtents(0.0f)
            , m_maxExtents(0.0f)
        {
        }

		union
		{
			uint32_t m_children;
			uint32_t m_numFaces;			
		};

		const uint16_t* m_faces;        
        Vector3 m_minExtents;
        Vector3 m_maxExtents;
    };


    struct Bounds
    {
        Bounds() : m_min(0.0f), m_max(0.0f)
        {
        }

        Bounds(const Vector3& min, const Vector3& max) : m_min(min), m_max(max)
        {
        }

        inline float GetVolume() const
        {
            Vector3 e = m_max-m_min;
            return (e.x*e.y*e.z);
        }

        inline float GetSurfaceArea() const
        {
            Vector3 e = m_max-m_min;
            return 2.0f*(e.x*e.y + e.x*e.z + e.y*e.z);
        }

        inline void Union(const Bounds& b)
        {
            m_min = Min(m_min, b.m_min);
            m_max = Max(m_max, b.m_max);
        }

        Vector3 m_min;
        Vector3 m_max;
    };

    typedef std::vector<Vec3> PositionArray;
    typedef std::vector<Node> NodeArray;
    typedef std::vector<uint16_t> FaceArray;
    typedef std::vector<Bounds> FaceBoundsArray;

	// partition the objects and return the number of objects in the lower partition
	uint32_t PartitionMedian(Node& n, uint16_t* faces, uint32_t numFaces);
	uint32_t PartitionSAH   (Node& n, uint16_t* faces, uint32_t numFaces);

    void Build();
    void BuildRecursive(uint32_t nodeIndex, uint16_t* faces, uint32_t numFaces);
    void TraceRecursive(uint32_t nodeIndex, const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const;
 
    void CalculateFaceBounds(const uint16_t* faces, uint32_t numFaces, Vector3& outMinExtents, Vector3& outMaxExtents);
    uint32_t GetNumFaces() const { return m_numFaces; }
	uint32_t GetNumNodes() const { return uint32_t(m_nodes.size()); }

	// track the next free node
	uint32_t m_freeNode;

    array_span_t<const Vec3> m_vertices;
    array_span_t<const u16> m_indices;
    const uint32_t m_numFaces;

    FaceArray m_faces;
    NodeArray m_nodes;
    FaceBoundsArray m_faceBounds;    

    // stats
    uint32_t m_treeDepth;
    uint32_t m_innerNodes;
    uint32_t m_leafNodes; 
};
