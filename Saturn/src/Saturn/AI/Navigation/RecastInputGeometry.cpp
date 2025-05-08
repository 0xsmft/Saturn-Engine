/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#include "sppch.h"
#include "RecastInputGeometry.h"

#include "Recast.h"

#include "Saturn/Scene/Scene.h"
#include "Saturn/Physics/PhysicsScene.h"

namespace Saturn {

	void RecastInputGeometry::BeginImport()
	{
		if( m_pChunkyTriMesh )
		{
			delete m_pChunkyTriMesh;
			m_pChunkyTriMesh = nullptr;
		}

		m_pChunkyTriMesh = new rcChunkyTriMesh();
	}

	void RecastInputGeometry::Add( Ref<StaticMesh> mesh, const glm::mat4& rModel )
	{
		auto& rVertices = mesh->Vertices();
		auto& rIndices = mesh->Indices();

		m_VertexBuffer.reserve( rVertices.size() );
		m_IndexBuffer.reserve( rIndices.size() );

		for( const auto& rVertex : rVertices )
		{
			glm::vec3 vertexPos = rVertex.Position;
			glm::vec3 worldPos = glm::vec3( rModel * glm::vec4( vertexPos, 1.0f ) );

			m_VertexBuffer.push_back( worldPos.x );
			m_VertexBuffer.push_back( worldPos.y );
			m_VertexBuffer.push_back( worldPos.z );
		}

		for( const auto& rIndex : rIndices )
		{
			m_IndexBuffer.push_back( rIndex.V1 );
			m_IndexBuffer.push_back( rIndex.V2 );
			m_IndexBuffer.push_back( rIndex.V3 );
		}

		m_FaceCount += mesh->GetIndexCount();
	}

	void RecastInputGeometry::EndImport()
	{
		float min[ 3 ], max[ 3 ];
		rcCalcBounds( m_VertexBuffer.data(), m_VertexBuffer.size() / 3, min, max );

		m_MinBounds = glm::vec3( min[ 0 ], min[ 1 ], min[ 2 ] );
		m_MaxBounds = glm::vec3( max[ 0 ], max[ 1 ], max[ 2 ] );

		rcCreateChunkyTriMesh( m_VertexBuffer.data(), m_IndexBuffer.data(), m_IndexBuffer.size() / 3, 256, m_pChunkyTriMesh );
	}

}