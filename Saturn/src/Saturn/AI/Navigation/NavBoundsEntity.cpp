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
#include "NavBoundsEntity.h"

#include "RecastNavigationMesh.h"
#include "RecastInputGeometry.h"

#include "Saturn/Vulkan/VertexBuffer.h"
#include "Saturn/Vulkan/IndexBuffer.h"
#include "Saturn/Core/Octree.h"

namespace Saturn {

	NavBoundsEntity::NavBoundsEntity()
		: Entity()
	{
		auto& comp = AddComponent<NavigationMeshSpecificationComponent>().Extent = { 1000.0f, 10000.0f, 1000.0f };

		SetAABB( GetComponent<TransformComponent>().Position, comp );
	}

	NavBoundsEntity::~NavBoundsEntity()
	{
	}

	void NavBoundsEntity::BeginPlay()
	{
	}

	void NavBoundsEntity::OnUpdate( Saturn::Timestep ts )
	{
	}

	void NavBoundsEntity::OnPhysicsUpdate( Saturn::Timestep ts )
	{
	}

	void NavBoundsEntity::SetAABB( const glm::vec3& rCenter, const glm::vec3& rExtent )
	{
		m_Bounds.Max = rCenter + rExtent;
		m_Bounds.Min = rCenter - rExtent;
	}

	Saturn::AABB NavBoundsEntity::GetBoundingBox()
	{
		auto& comp = GetComponent<NavigationMeshSpecificationComponent>().Extent;
		auto& pos = GetComponent<TransformComponent>().Position;

		m_Bounds.Max = pos + comp;
		m_Bounds.Min = pos - comp;

		return m_Bounds;
	}

	glm::vec3 NavBoundsEntity::GetRandomPoint()
	{
		return {};
	}

	static AABB TransformAABB( const AABB& rAABB, const glm::mat4& rTransform )
	{
		// Get all 8 corners of the AABB
		glm::vec4 corners[ 8 ] = {
			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Min.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Min.y, rAABB.Max.z, 1.0f },

			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Min.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Min.y, rAABB.Min.z, 1.0f }
		};

		glm::vec3 newMin( corners[ 0 ] );
		glm::vec3 newMax( corners[ 0 ] );

		for( int i = 0; i < 8; ++i )
		{
			glm::vec3 transformed = glm::vec3( corners[ i ] );
			newMin = glm::min( newMin, transformed );
			newMax = glm::max( newMax, transformed );
		}

		return { newMin, newMax };
	}

	void NavBoundsEntity::GatherGeometry()
	{
		std::vector<float> rOutVertices;
		std::vector<int> rOutIndices;

		GetBoundingBox();

#if SAT_X31_OCTREE
		Octree<Ref<Entity>> navOctree( m_Bounds );

		for( auto& rEntity : m_Scene->GetAllEntitiesWith<StaticMeshComponent>() )
		{
			auto& tc = rEntity->GetComponent<TransformComponent>();
			auto& mesh = rEntity->GetComponent<StaticMeshComponent>();

			if( mesh.Mesh )
			{
				navOctree.Insert( tc.Position, rEntity );
			}
		}

		std::vector<Ref<Entity>> result;
		navOctree.Query( m_Bounds, result );
#elif SAT_X31_VERT_INTERSECT
		for( auto& rEntity : m_Scene->GetAllEntitiesWith<StaticMeshComponent>() )
		{
			auto& mesh = rEntity->GetComponent<StaticMeshComponent>();
			glm::mat4 entityTransform = GActiveScene->GetTransformRelativeToParent( rEntity );

			if( mesh.Mesh )
			{
				auto& rVertices = mesh.Mesh->Vertices();
				auto& rIndices = mesh.Mesh->Indices();
				auto& rSubmeshes = mesh.Mesh->Submeshes();

				for( const auto& rSubmesh : rSubmeshes )
				{
					glm::mat4 model = entityTransform * rSubmesh.Transform;

					std::unordered_map<uint32_t, uint32_t> indexMap;
					uint32_t vertOffset = rVertices.size() / 3;
					
					for( uint32_t i = 0; i < rSubmesh.VertexCount; i++ )
					{
						uint32_t vertexIndex = i + rSubmesh.BaseVertex;
						glm::vec3 vertexPos = rVertices[ vertexIndex ].Position;
						glm::vec3 worldPos = glm::vec3( model * glm::vec4( vertexPos, 1.0f ) );

						if( m_Bounds.Contains( worldPos ) )
						{
							rOutVertices.push_back( worldPos.x );
							rOutVertices.push_back( worldPos.y );
							rOutVertices.push_back( worldPos.z );
							indexMap[ vertexIndex ] = vertOffset++;
						}
					}

					uint32_t start = rSubmesh.BaseIndex / 3;
					uint32_t count = rSubmesh.IndexCount / 3;

					for( uint32_t i = 0; i < count; i++ )
					{
						const Index& rIndex = rIndices[ start + i ];

						// Check if all three vertices were submitted
						if( indexMap.count( rIndex.V1 ) && indexMap.count( rIndex.V2 ) && indexMap.count( rIndex.V3 ) )
						{
							rOutIndices.push_back( rIndex.V1 );
							rOutIndices.push_back( rIndex.V2 );
							rOutIndices.push_back( rIndex.V3 );
						}
					}
				}
			}
		}
#else
		RecastInputGeometry input;
		input.BeginImport();

		for( auto& rEntity : m_Scene->GetAllEntitiesWith<StaticMeshComponent>() )
		{
			auto& mesh = rEntity->GetComponent<StaticMeshComponent>();
			glm::mat4 entityTransform = GActiveScene->GetTransformRelativeToParent( rEntity );

			if( mesh.Mesh )
			{
				bool moveOn = false;
	
				auto& rVertices = mesh.Mesh->Vertices();
				auto& rIndices = mesh.Mesh->Indices();
				auto& rSubmeshes = mesh.Mesh->Submeshes();

				for( const auto& rSubmesh : rSubmeshes )
				{
					if( moveOn ) break;

					glm::mat4 model = entityTransform * rSubmesh.Transform;

					for( uint32_t i = 0; i < rSubmesh.VertexCount; i++ )
					{
						uint32_t vertexIndex = i + rSubmesh.BaseVertex;
						glm::vec3 vertexPos = rVertices[ vertexIndex ].Position;
						glm::vec3 worldPos = glm::vec3( model * glm::vec4( vertexPos, 1.0f ) );

						// If one vertex is in the bounds submit the whole mesh
						// Recast will clip it
						if( m_Bounds.Contains( worldPos ) )
						{
							input.Add( mesh.Mesh, model );

							moveOn = true;
							break;
						}
					}
				}
			}
		}

		input.EndImport();
#endif

		m_Builder.Init();
		m_Builder.SetBounds( m_Bounds );
		m_Builder.Build( input );
	}

}