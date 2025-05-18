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

#include "RecastNavigationMeshBuilder.h"
#include "RecastInputGeometry.h"
#include "RecastNavigationMeshCache.h"
#include "NavigationOctree.h"

#include "Detour/DetourNavMeshQuery.h"

#include "Saturn/Vulkan/VertexBuffer.h"
#include "Saturn/Vulkan/IndexBuffer.h"

namespace Saturn {

	NavBoundsEntity::NavBoundsEntity()
		: Entity()
	{
		AddComponent<NavigationMeshSpecificationComponent>();

		SetAABB( GetComponent<TransformComponent>().Position, glm::vec3( 10.0f ) );
		GetComponent<TransformComponent>().Scale = glm::vec3( 10.0f );

		std::filesystem::path path = Project::GetActiveProject()->GetFullCachePath();
		path /= std::format( "NavMesh{0}.{1}.srnc", m_Scene->Name, ( uint64_t ) GetUUID() );
		m_Builder.TryLoadFromCache( path );
	}

	NavBoundsEntity::NavBoundsEntity( const std::string& rName, UUID id )
		: Entity( rName, id )
	{
		AddComponent<NavigationMeshSpecificationComponent>();

		SetAABB( GetComponent<TransformComponent>().Position, glm::vec3( 10.0f ) );
		GetComponent<TransformComponent>().Scale = glm::vec3( 10.0f );
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
		m_MaxBounds.Max = rCenter + rExtent;
		m_MaxBounds.Min = rCenter - rExtent;
	}

	Saturn::AABB NavBoundsEntity::GetBoundingBox()
	{
		TransformComponent tc = m_Scene->GetWorldSpaceTransform( this );

		auto& comp = tc.Scale;
		auto pos = tc.Position;

		m_MaxBounds.Max = pos + comp;
		m_MaxBounds.Min = pos - comp;

		return m_MaxBounds;
	}

	void NavBoundsEntity::GatherGeometryAndBuild()
	{
		GetBoundingBox();

		m_Scene->PrepareForNavMeshBuilding();

		RecastInputGeometry input;
		input.BeginImport();
		input.GetExportData().Bounds = m_MaxBounds;

		AABB navMeshBounds;
		navMeshBounds.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		navMeshBounds.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

#if defined(SAT_X31_VERT_INTERSECT_X21)
		for( auto& rEntity : m_Scene->GetAllEntitiesWith<StaticMeshComponent>() )
		{
			auto& mesh = rEntity->GetComponent<StaticMeshComponent>();

			if( mesh.Mesh )
			{
				glm::mat4 entityTransform = GActiveScene->GetTransformRelativeToParent( rEntity );
				
				auto& rVertices = mesh.Mesh->Vertices();
				auto& rIndices = mesh.Mesh->Indices();
				auto& rSubmeshes = mesh.Mesh->Submeshes();

				for( const auto& rSubmesh : rSubmeshes )
				{
					glm::mat4 model = entityTransform * rSubmesh.Transform;

					uint32_t count = rSubmesh.IndexCount / 3;
					uint32_t start = rSubmesh.BaseIndex;

					uint32_t vertOffset = input.GetVertexBuffer().size();
					for( uint32_t i = 0; i < count; i++ )
					{
						bool inBounds = false;

						glm::vec3 triWorld[ 3 ]{};

						for( uint32_t vert = 0; vert < 3; vert++ )
						{
							uint32_t vertexIndex = ( vert == 0 ) ? rIndices[ i ].V1 : ( vert == 1 ) ? rIndices[ i ].V2 : rIndices[ i ].V3;

							glm::vec3 localVertexPos = rVertices[ vertexIndex ].Position;
							glm::vec3 worldSpace = glm::vec3( model * glm::vec4( localVertexPos, 1.0f ) );

							triWorld[ vert ] = worldSpace;

							if( m_MaxBounds.Contains( worldSpace ) )
							{
								inBounds = true;
							}
						}

						if( inBounds )
						{
							for( int j = 0; j < 3; j++ )
							{
								input.AddVert( triWorld[ j ] );
							}

							input.AddSingleIndex( vertOffset/*+0*/ );
							input.AddSingleIndex( vertOffset + 1 );
							input.AddSingleIndex( vertOffset + 2 );
						
							for( int j = 0; j < 3; j++ )
							{
								navMeshBounds.Min.x = glm::min( triWorld[ j ].x, navMeshBounds.Min.x );
								navMeshBounds.Min.y = glm::min( triWorld[ j ].y, navMeshBounds.Min.y );
								navMeshBounds.Min.z = glm::min( triWorld[ j ].z, navMeshBounds.Min.z );

								navMeshBounds.Max.x = glm::max( triWorld[ j ].x, navMeshBounds.Max.x );
								navMeshBounds.Max.y = glm::max( triWorld[ j ].y, navMeshBounds.Max.y );
								navMeshBounds.Max.z = glm::max( triWorld[ j ].z, navMeshBounds.Max.z );

//								navMeshBounds.Min = glm::min( navMeshBounds.Min, triWorld[ j ] );
//								navMeshBounds.Max = glm::max( navMeshBounds.Max, triWorld[ j ] );
							}

							vertOffset += 3;
						}
					}
				}
			}
		}
#else
		PhysXSceneExporter exp;
		exp.Export( input, navMeshBounds );
#endif

		SAT_CORE_ASSERT( input.GetVertexBuffer().size() % 3 == 0 );
		SAT_CORE_ASSERT( input.GetIndexBuffer().size() % 3 == 0 );

		input.EndImport( navMeshBounds );

		m_Builder.Init();
		m_Builder.Build( input );

		GetComponent<NavigationMeshSpecificationComponent>().HasBuilt = true;
	
		m_Scene->OnNavMeshBuildCompleted( GetHandle() );

		std::filesystem::path path = Project::GetActiveProject()->GetFullCachePath();
		path /= std::format( "NavMesh{0}.{1}.srnc", m_Scene->Name, ( uint64_t ) GetUUID() );
		RecastNavigationMeshCache::SaveNavMesh( path, m_Builder.GetNavMesh() );
	}

}