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
#include "UndoRedoNodeEditorActions.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// MODIFY NODE POSITION

	UndoRedoActionModifyNodePosition::UndoRedoActionModifyNodePosition( SharedPtr<NodeEditor> nodeEditor, SharedPtr<NodeEditorNodeBase> originalNode, const ImVec2& rOldPosition )
		: UndoRedoActionBase( "Modify Node Position" ), m_NodeEditor( nodeEditor )
	{
#if !defined(SAT_DIST)
		m_NodeID = originalNode->ID;
		m_OldPosition = rOldPosition;
		m_NewPosition = ed::GetNodePosition( ed::NodeId( originalNode->ID ) );
#endif
	}

	UndoRedoActionModifyNodePosition::~UndoRedoActionModifyNodePosition()
	{
	}

	void UndoRedoActionModifyNodePosition::Undo()
	{
		if( auto nodeEditor = m_NodeEditor.Access() )
		{
			nodeEditor->SetNodePosition( m_NodeID, m_OldPosition );
		}
	}

	void UndoRedoActionModifyNodePosition::Redo()
	{
		if( auto nodeEditor = m_NodeEditor.Access() )
		{
			nodeEditor->SetNodePosition( m_NodeID, m_NewPosition );
		}
	}

}
