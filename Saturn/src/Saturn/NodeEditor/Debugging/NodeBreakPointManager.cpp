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

#include "sppch.h"
#include "NodeBreakPointManager.h"

namespace Saturn {

	NodeBreakPointManager::NodeBreakPointManager()
	{
	}

	NodeBreakPointManager::~NodeBreakPointManager()
	{
		m_Breakpoints.clear();
	}

	NodeBreakPoint& NodeBreakPointManager::AddBreakPoint( UUID nodeID, BreakPointType type )
	{
		m_Breakpoints.emplace( nodeID, NodeBreakPoint{ 0, type } );
		return m_Breakpoints[ nodeID ];
	}

	void NodeBreakPointManager::Break( UUID nodeID )
	{
		// Check if we have a break point...
		if( m_Breakpoints.contains( nodeID ) )
		{
			// ... if not we can add one and set it to be single fire.

			AddBreakPoint( nodeID, BreakPointType::SingleFire );
		}
		else
		{
			// ... if we have one active it.
			m_Breakpoints[ nodeID ].Active = true;
		}
	}

	void NodeBreakPointManager::Remove( UUID nodeID )
	{
		auto itr = m_Breakpoints.find( nodeID );
		if( itr != m_Breakpoints.end() )
		{
			m_Breakpoints.erase( itr );
		}
	}

	void NodeBreakPointManager::Deactivate( UUID nodeID )
	{
		auto itr = m_Breakpoints.find( nodeID );
		if( itr != m_Breakpoints.end() )
		{
			auto& rBreakPoint = itr->second;
			rBreakPoint.Active = false;
		}
	}

	bool NodeBreakPointManager::ShouldBreak( UUID nodeID )
	{
		auto itr = m_Breakpoints.find( nodeID );
		if( itr != m_Breakpoints.end() )
		{
			auto& rBreakPoint = itr->second;

			if( !rBreakPoint.Active )
				return false;

			++rBreakPoint.HitCount;
			
			if( rBreakPoint.Type == BreakPointType::SingleFire )
			{
				m_Breakpoints.erase( itr );
			}

			return true;
		}

		return false;
	}

}
