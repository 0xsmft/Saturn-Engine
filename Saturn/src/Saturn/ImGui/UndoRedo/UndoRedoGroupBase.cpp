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

#include "GlobalUndoRedoGroup.h"

namespace Saturn {

	UndoRedoGroupBase::UndoRedoGroupBase()
	{
		GlobalUndoRedoGroup::Get().AddGroup( this );
	}

	UndoRedoGroupBase::~UndoRedoGroupBase()
	{
	}

	void UndoRedoGroupBase::UndoMostRecent()
	{
		if( !m_UndoActions.empty() )
		{
			auto& rAction = m_UndoActions.top();

			rAction->Undo();

			m_RedoActions.push( rAction );
			m_UndoActions.pop();
		}
	}

	void UndoRedoGroupBase::RedoMostRecent()
	{
		if( !m_RedoActions.empty() )
		{
			auto& rAction = m_RedoActions.top();

			rAction->Redo();
			
			m_UndoActions.push( rAction );
			m_RedoActions.pop();
		}
	}

	void UndoRedoGroupBase::AddAction( Ref<UndoRedoActionBase> action )
	{
		m_UndoActions.push( action );
	}
}