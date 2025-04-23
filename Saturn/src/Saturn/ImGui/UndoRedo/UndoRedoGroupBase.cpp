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
#include "UndoRedoGroupBase.h"

namespace Saturn {

	UndoRedoGroupBase::UndoRedoGroupBase()
	{
	}

	UndoRedoGroupBase::~UndoRedoGroupBase()
	{
		m_Actions.clear();
	}

	void UndoRedoGroupBase::UndoMostRecent()
	{
		if( m_Actions.size() )
		{
			m_Actions.back()->Undo();
		}
	}

	void UndoRedoGroupBase::UndoTo( size_t amount /*= 0 */ )
	{
		if( amount >= m_Actions.size() )
			return;
	
		for( size_t i = m_Actions.size() - 1; i > amount; --i )
		{
			m_Actions[ i ]->Undo();
			m_Actions.pop_back();
		}
	}

	void UndoRedoGroupBase::RedoMostRecent()
	{
		m_Actions.back()->Redo();
	}

	void UndoRedoGroupBase::AddAction( Ref<UndoRedoActionBase> action )
	{
		m_Actions.push_back( action );
	}
}