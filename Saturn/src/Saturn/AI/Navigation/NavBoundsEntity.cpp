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

#include "Saturn/Project/Project.h"

#include "Saturn/Vulkan/VertexBuffer.h"
#include "Saturn/Vulkan/IndexBuffer.h"

#if !defined(SAT_DIST)
#include "Saturn/Vulkan/Renderer2D.h"
#endif

namespace Saturn {

	NavBoundsEntity::NavBoundsEntity()
		: Entity()
	{
		Init();
	}

	void NavBoundsEntity::Init()
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
		const TransformComponent tc = GetScene()->GetWorldSpaceTransform( SharedFromThis() );
		const auto& comp = tc.Scale;
		const auto pos = tc.Position;

		m_MaxBounds.Max = pos + comp;
		m_MaxBounds.Min = pos - comp;

		return m_MaxBounds;
	}

	void NavBoundsEntity::GatherGeometryAndBuild()
	{
		GetBoundingBox();

		GetScene()->PrepareForNavMeshBuilding();

		RecastInputGeometry input;
		input.BeginImport();
		input.GetExportData().Bounds = m_MaxBounds;

		AABB navMeshBounds;
		navMeshBounds.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		navMeshBounds.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		PhysXSceneExporter::Export( input, navMeshBounds );

		SAT_CORE_ASSERT( input.GetVertexBuffer().size() % 3 == 0 );
		SAT_CORE_ASSERT( input.GetIndexBuffer().size() % 3 == 0 );

		input.EndImport( navMeshBounds );

		m_Builder.Init();
		m_Builder.Build( input );

		GetComponent<NavigationMeshSpecificationComponent>().HasBuilt = true;
	
		std::filesystem::path path = Project::GetActiveProject()->GetFullCachePath();
		path /= std::format( "NavMesh{0}.{1}.srnc", GetScene()->Name, ( uint64_t ) GetUUID() );
		RecastNavigationMeshCache::SaveNavMesh( path, m_Builder.GetNavMesh() );

		CleanDirty();
		GetScene()->DestroyPhysicsScene();
	}

	void NavBoundsEntity::LoadNavMeshFromDisk()
	{
		std::filesystem::path path = Project::GetActiveProject()->GetFullCachePath();
		path /= std::format( "NavMesh{0}.{1}.srnc", GetScene()->Name, ( uint64_t ) GetUUID() );

		GetComponent<NavigationMeshSpecificationComponent>().HasBuilt = m_Builder.TryLoadFromCache( path );
	}

#if !defined(SAT_DIST)
	void NavBoundsEntity::DebugDraw( Renderer2D* pRenderer2D )
	{
		m_Builder.DebugDrawNavMesh( pRenderer2D );
	}
#endif

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG_SPWN( NavBoundsEntity );
