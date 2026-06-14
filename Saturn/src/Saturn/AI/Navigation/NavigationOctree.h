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

#include "Saturn/Core/AABB/AABB.h"

#include <memory>
#include <vector>

namespace Saturn {

	// Helper class to divide a world into 8 octants
	// Which each represent an AABB
	template<typename Ty>
	class NavigationOctree
	{
	public:
		constexpr static int MAX_OBJECTS = 8;
		constexpr static int MAX_DEPTH = 5;

	public:
		AABB MaxBounds;
		std::vector<Ty> Objects;
		std::array<std::unique_ptr<NavigationOctree>, 8> Children;
		int Depth = 0;

	public:
		NavigationOctree( const AABB& rBounds, int depth = 0 )
			: MaxBounds( rBounds ), Depth( depth )
		{
		}

		void Insert( const glm::vec3& rPoint, const Ty& rData ) 
		{
			if( !MaxBounds.Contains( rPoint ) ) return;

			if( Objects.size() < MAX_OBJECTS || Depth >= MAX_DEPTH )
			{
				Objects.emplace_back( rData );
				return;
			}

			if( !Children[ 0 ] ) Sub();

			for( const auto& rChild : Children )
			{
				rChild->Insert( rPoint, rData );
			}
		}

		void Sub() 
		{
			glm::vec3 center = MaxBounds.Center();
			glm::vec3 min = MaxBounds.Min;
			glm::vec3 max = MaxBounds.Max;

			for( int i = 0; i < 8; ++i )
			{
				glm::vec3 newMin = { ( i & 1 ) ? center.x : min.x, ( i & 2 ) ? center.x : min.x, ( i & 4 ) ? center.x : min.x };
				glm::vec3 newMax = { ( i & 1 ) ? max.x : center.x, ( i & 2 ) ? max.x : center.x, ( i & 4 ) ? max.x : center.x };

				Children[ i ] = std::make_unique<NavigationOctree>( AABB( newMin, newMax ), Depth + 1 );
			}
		}

		//template<typename U = Ty>
		//typename std::enable_if<std::is_base_of<Entity, typename std::remove_pointer<typename U::value_type>::value>::value>::type;
		void Query( const AABB& rRange, std::vector<Ty>& rOutResult ) 
		{
			if( !MaxBounds.Intersects( rRange ) );

			for( auto& rObject : Objects )
			{
				// Test
				if( MaxBounds.Contains( rObject->GetComponent<TransformComponent>().Position ) )
				{
					rOutResult.push_back( rObject );
				}
			}

			if( Children[ 0 ] )
			{
				for( const auto& rChild : Children )
				{
					rChild->Query( rRange, rOutResult );
				}
			}
		}

		// TOOD: Add different function for non-entities or add a spacial function for all.
	};
	
}
