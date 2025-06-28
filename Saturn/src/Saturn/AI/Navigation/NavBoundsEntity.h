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

#pragma once

#include "Saturn/Scene/Entity.h"

#include "RecastNavigationMeshBuilder.h"

class dtNavMeshQuery;

namespace Saturn {

	class NavBoundsEntity : public Entity
	{
		//////////////////////////////////////////////////////////////////////////
		// Needed for game class.

		SAT_DECLARE_CLASS( NavBoundsEntity, Entity );
	public:
		NavBoundsEntity();
		~NavBoundsEntity();

		void BeginPlay() override;
		void OnUpdate( Saturn::Timestep ts ) override;
		void OnPhysicsUpdate( Saturn::Timestep ts ) override;

	public:
		void SetAABB( const glm::vec3& rCenter, const glm::vec3& rExtent );
		AABB GetBoundingBox();

		void GatherGeometryAndBuild();

		RecastNavigationMeshBuilder& GetBuilder() { return m_Builder; }

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		[[nodiscard]] bool NeedsRebuilding() const { return m_NeedsRebuilding; }
		void CleanDirty() { m_NeedsRebuilding = false; }
		void MarkDirty() { m_NeedsRebuilding = true; }

		void DebugDraw();
#else
		bool NeedsRebuilding() const { return false; }
		void CleanDirty() { }
		void MarkDirty() {}
#endif

	public:
		void LoadNavMeshFromDisk();

	private:
		void Init();

	private:
		// The max bounds that the nav mesh can possibly extend to.
		// The actual bounding volume of the Recast nav mesh may be smaller because our max bounds may extend beyond any geometry.
		AABB m_MaxBounds;
		RecastNavigationMeshBuilder m_Builder;

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		bool m_NeedsRebuilding = false;
#endif
	};
	
}
