/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
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

#include "Saturn/Core/Ref.h"
#include "StraightNavPath.h"

#include <vector>
#include <expected>

class dtNavMeshQuery;

namespace Saturn {

	class NavBoundsEntity;
	class Renderer2D;

	class NavigationSystem
	{
	public:
		NavigationSystem() = default;
		~NavigationSystem();

		void Initialise();
		
		// RUNTIME ONLY! Call this BEFORE Scene::Empty,
		// the scene calls it in OnRuntimeEnd(), so really there should be 
		// no need to call this yourself outside of that function.
		void ReleaseReferenceToNavBounds();

		// Clear all paths and destroy the detour nav query object.
		void Terminate();

		void DebugDraw( Renderer2D* pRenderer2D );
	
	public:
		// Note that the pointer that gets returned is a Ref<> so this means that you do not
		// have to worry about the lifetime of this object
		// do NOT call delete on it though! shit WILL break!
		StraightNavPath* CreateStraightPath( const glm::vec3& rStart, const glm::vec3& rEnd, uint32_t maxPaths = 256 );

		// Destory a navpath, you do not need to do this as all paths will get cleared when the
		// NavigationSystem cleanups, but if you need to destroy a path then do so with this
		// function.
		void DestoryStraightPath( StraightNavPath* pPath );

		uint32_t FindNearestPoly( const glm::vec3& rPosition, float* pNearestPoint );

	public:
		dtNavMeshQuery* GetNavMeshQuery() const { return m_pNavMeshQuery; }

	public:
		std::expected<glm::vec3, unsigned int> GetRandomPointInNavMesh( float maxRadius ) const;

	private:
		bool m_Initialised = false;

		// #ReplaceRawPtrOrRefWithWeakRef
		WeakRef<NavBoundsEntity> m_NavBoundsEntity;

		dtNavMeshQuery* m_pNavMeshQuery = nullptr;

		std::vector<Ref<StraightNavPath>> m_Paths;
	};
}
