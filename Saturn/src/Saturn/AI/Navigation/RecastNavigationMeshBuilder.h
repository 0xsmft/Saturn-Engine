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

#include "Saturn/Core/AABB/AABB.h"

#include "RecastInputGeometry.h"
#include "Visualisation/RecastDebugVisualisation.h"

#include <Recast/Recast.h>
#include <Detour/DetourNavMesh.h>

namespace Saturn {

	enum class NavigationMeshArea
	{
		Ground,
		Water,
		Road,
		Door,
		Grass,
		Jump
	};

	enum NavigationMeshPolyFlag_
	{
		NavigationMeshPolyFlag_Walk = BIT( 0 ),
		NavigationMeshPolyFlag_Swim = BIT( 1 ),
		NavigationMeshPolyFlag_Door = BIT( 2 ),
		NavigationMeshPolyFlag_Jump = BIT( 3 ),
		NavigationMeshPolyFlag_Disabled = BIT( 4 ),
		NavigationMeshPolyFlag_All = 0xFFFF
	};

	// NavigationMeshPolyFlag
	typedef NavigationMeshPolyFlag_ NavigationMeshPolyFlag;

	class RecastContext : public rcContext
	{
	public:
		RecastContext();
		~RecastContext() = default;

	protected:
		void doLog( const rcLogCategory category, const char* pMessage, const int len ) override;
		
#if !defined(SAT_DIST)
		void doResetTimers() override;
		void doStartTimer( const rcTimerLabel label ) override;
		void doStopTimer( const rcTimerLabel label ) override;
		int doGetAccumulatedTime( const rcTimerLabel label ) const override;

	private:
		std::map<rcTimerLabel, Timer> m_Timers;
#endif
	};

	//
	// RecastNavigationMeshBuilder
	// This class is responsible for building tiles into the navmesh, it is owned by the navmesh it self.
	// 
	// Furthermore, it owns the the dtNavMesh, which is the pointer to detour's navmesh. 
	//
	class RecastNavigationMeshBuilder
	{
	public:
		RecastNavigationMeshBuilder();
		~RecastNavigationMeshBuilder();

		void Init();
		void Build( const RecastInputGeometry& rInputGeometry );
		bool TryLoadFromCache( const std::filesystem::path& rPath );

		void DebugDrawNavMesh();

		dtNavMesh* GetNavMesh() const { return m_pNavMesh; }

	private:
		dtNavMesh* m_pNavMesh = nullptr;

		rcConfig m_Config{};
		RecastContext m_Context;
		RecastDebugVisualisation m_DebugDrawer{};

		glm::vec3 m_LastTileMin{}, m_LastTileMax{};
	};

	struct RecastNavigationTileBuilderData
	{
		glm::vec3 LastTileMin{}, LastTileMax{};
		bool KeepResults = false;
	};

	//
	// RecastNavigationTileBuilder
	// This class is responsible for building a single tile into a navmesh.
	//
	class RecastNavigationTileBuilder
	{
	public:
		RecastNavigationTileBuilder( RecastNavigationTileBuilderData& rData, int x, int y );
		~RecastNavigationTileBuilder();

		[[nodiscard]] bool Build( dtNavMesh* pParentMesh, RecastContext& rContext, const RecastInputGeometry& rInputGeometry );
		void RemoveTile( dtNavMesh* pParentMesh );

		void FreeAll();

		[[nodiscard]] bool IsDirty() const { return m_Dirty; }
		void MarkDirty() { m_Dirty = true; }

	private:
		void CleanDirty() { m_Dirty = false; }

	private:
		rcPolyMesh* m_pPolyMesh = nullptr;
		rcPolyMeshDetail* m_pPolyMeshDetail = nullptr;
		rcHeightfield* m_pSolidHeightfield = nullptr;
		rcContourSet* m_pContourSet = nullptr;
		rcCompactHeightfield* m_pCompactHeightfield = nullptr;

		unsigned char* m_AreaFlags = nullptr;

	private:
		int m_X{}, m_Y{};
		RecastNavigationTileBuilderData m_Data;
		bool m_Dirty = false;
	};
}
