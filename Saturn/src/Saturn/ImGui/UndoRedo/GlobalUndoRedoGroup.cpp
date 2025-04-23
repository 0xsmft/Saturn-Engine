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
#include "GlobalUndoRedoGroup.h"

namespace Saturn {

	GlobalUndoRedoGroup::GlobalUndoRedoGroup()
	{
		SingletonStorage::AddSingleton( this );
	}

	void GlobalUndoRedoGroup::ClearGroups()
	{
		m_UndoRedoGroups.clear();

		SingletonStorage::RemoveSingleton( this );
	}

	GlobalUndoRedoGroup::~GlobalUndoRedoGroup()
	{
		ClearGroups();
	}

	void GlobalUndoRedoGroup::GlobalUndoRecent()
	{
		for( auto& rGroup : m_UndoRedoGroups )
			rGroup->UndoMostRecent();
	}

	void GlobalUndoRedoGroup::GlobalRedoRecent()
	{
		for( auto& rGroup : m_UndoRedoGroups )
			rGroup->RedoMostRecent();
	}

	void GlobalUndoRedoGroup::GlobalUndoTo( size_t amount /*= 0 */ )
	{
		for( auto& rGroup : m_UndoRedoGroups )
			rGroup->UndoTo( amount );
	}

	void GlobalUndoRedoGroup::GlobalRedoTo( size_t amount /*= 0 */ )
	{
		//for( auto& rGroup : m_UndoRedoGroups )
		//	rGroup->RedoTo( amount );
	}

	void GlobalUndoRedoGroup::AddGroup( Ref<UndoRedoGroupBase> group )
	{
		m_UndoRedoGroups.push_back( group );
	}

	void GlobalUndoRedoGroup::RemoveGroup( Ref<UndoRedoGroupBase> group )
	{
		auto it = std::remove( m_UndoRedoGroups.begin(), m_UndoRedoGroups.end(), group );
		m_UndoRedoGroups.erase( it, m_UndoRedoGroups.end() );
	}

}