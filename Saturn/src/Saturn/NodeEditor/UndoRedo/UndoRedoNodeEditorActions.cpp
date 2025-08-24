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
#include "UndoRedoNodeEditorActions.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// TODO: USE WEAK REFS!
	// CREATE NODE/DELETE NODE
	// TODO: USE GLOBAL NODE LIST FOR EVAL
	// TODO: CREATE COPY OPERATOR

	UndoRedoActionCreateNode::UndoRedoActionCreateNode( SharedPtr<NodeEditor> nodeEditor, SharedPtr<NodeEditorNodeBase> originalNode )
		: UndoRedoActionBase( "Create Node" ), m_NodeEditor( nodeEditor )
	{
		/*
		m_NodeCopy = Ref<NodeEditorNodeBase>::Create( originalNode->Name );
		m_NodeCopy->ID = originalNode->ID;
		m_NodeCopy->Position = originalNode->Position;
		m_NodeCopy->Size = originalNode->Size;
		m_NodeCopy->Inputs = originalNode->Inputs;
		m_NodeCopy->Outputs = originalNode->Outputs;
		m_NodeCopy->Color = originalNode->Color;
		m_NodeCopy->ExecutionType = originalNode->ExecutionType;
		m_NodeCopy->Type = originalNode->Type;
		m_NodeCopy->CanBeDeleted = originalNode->CanBeDeleted;
		m_NodeCopy->ActiveState = originalNode->ActiveState;
		m_NodeCopy->SavedState = originalNode->SavedState;
		m_NodeCopy->Name = originalNode->Name;

#if defined(SAT_DEBUG)
		m_NodeCopy->EvaluationOrder = originalNode->EvaluationOrder;
#endif
*/
	}

	UndoRedoActionCreateNode::~UndoRedoActionCreateNode()
	{
	}

	void UndoRedoActionCreateNode::Undo()
	{
		m_NodeEditor->DeleteNode( m_NodeCopy->ID, true );
	}

	void UndoRedoActionCreateNode::Redo()
	{
/*
		// create a new copy of the node and add to the editor
		Ref<NodeEditorNodeBase> newNode = Ref<NodeEditorNodeBase>::Create( m_NodeCopy->Name );
		newNode->ID = m_NodeCopy->ID;
		newNode->Position = m_NodeCopy->Position;
		newNode->Size = m_NodeCopy->Size;
		newNode->Inputs = m_NodeCopy->Inputs;
		newNode->Outputs = m_NodeCopy->Outputs;
		newNode->Color = m_NodeCopy->Color;
		newNode->ExecutionType = m_NodeCopy->ExecutionType;
		newNode->Type = m_NodeCopy->Type;
		newNode->CanBeDeleted = m_NodeCopy->CanBeDeleted;
		newNode->ActiveState = m_NodeCopy->ActiveState;
		newNode->SavedState = m_NodeCopy->SavedState;
		newNode->Name = m_NodeCopy->Name;
#if defined(SAT_DEBUG)
		newNode->EvaluationOrder = m_NodeCopy->EvaluationOrder;
#endif

		m_NodeEditor->AddNode( newNode );
		*/
	}

	UndoRedoActionDeleteNode::UndoRedoActionDeleteNode( SharedPtr<NodeEditor> nodeEditor, SharedPtr<NodeEditorNodeBase> originalNode )
		: UndoRedoActionBase( "Delete Node" ), m_NodeEditor( nodeEditor )
	{
		/*
		m_NodeCopy = Ref<NodeEditorNodeBase>::Create( originalNode->Name );
		m_NodeCopy->ID = originalNode->ID;
		m_NodeCopy->Position = originalNode->Position;
		m_NodeCopy->Size = originalNode->Size;
		m_NodeCopy->Inputs = originalNode->Inputs;
		m_NodeCopy->Outputs = originalNode->Outputs;
		m_NodeCopy->Color = originalNode->Color;
		m_NodeCopy->ExecutionType = originalNode->ExecutionType;
		m_NodeCopy->Type = originalNode->Type;
		m_NodeCopy->CanBeDeleted = originalNode->CanBeDeleted;
		m_NodeCopy->ActiveState = originalNode->ActiveState;
		m_NodeCopy->SavedState = originalNode->SavedState;
		m_NodeCopy->Name = originalNode->Name;

#if defined(SAT_DEBUG)
		m_NodeCopy->EvaluationOrder = originalNode->EvaluationOrder;
#endif
*/
	}

	UndoRedoActionDeleteNode::~UndoRedoActionDeleteNode()
	{
	}

	void UndoRedoActionDeleteNode::Undo()
	{
		/*
		// create a new copy of the node and add to the editor
		Ref<NodeEditorNodeBase> newNode = Ref<NodeEditorNodeBase>::Create( m_NodeCopy->Name );
		newNode->ID = m_NodeCopy->ID;
		newNode->Position = m_NodeCopy->Position;
		newNode->Size = m_NodeCopy->Size;
		newNode->Inputs = m_NodeCopy->Inputs;
		newNode->Outputs = m_NodeCopy->Outputs;
		newNode->Color = m_NodeCopy->Color;
		newNode->ExecutionType = m_NodeCopy->ExecutionType;
		newNode->Type = m_NodeCopy->Type;
		newNode->CanBeDeleted = m_NodeCopy->CanBeDeleted;
		newNode->ActiveState = m_NodeCopy->ActiveState;
		newNode->SavedState = m_NodeCopy->SavedState;
		newNode->Name = m_NodeCopy->Name;
#if defined(SAT_DEBUG)
		newNode->EvaluationOrder = m_NodeCopy->EvaluationOrder;
#endif

		m_NodeEditor->AddNode( newNode );
*/
	}

	void UndoRedoActionDeleteNode::Redo()
	{
		m_NodeEditor->DeleteNode( m_NodeCopy->ID, true );
	}

	//////////////////////////////////////////////////////////////////////////
	// MODIFY NODE POSITION

	UndoRedoActionModifyNodePosition::UndoRedoActionModifyNodePosition( SharedPtr<NodeEditor> nodeEditor, SharedPtr<NodeEditorNodeBase> originalNode, const ImVec2& rOldPosition )
		: UndoRedoActionBase( "Modify Node Position" ), m_NodeEditor( nodeEditor )
	{
#if !defined(SAT_DIST)
		m_NodeCopy = originalNode;
		m_NewPosition = rOldPosition;
		m_OldPosition = originalNode->Position;
#endif
	}

	UndoRedoActionModifyNodePosition::~UndoRedoActionModifyNodePosition()
	{
	}

	void UndoRedoActionModifyNodePosition::Undo()
	{
		if( auto nodeEditor = m_NodeEditor.Access() )
		{
			nodeEditor->SetNodePosition( m_NodeCopy->ID, m_OldPosition );
		}
	}

	void UndoRedoActionModifyNodePosition::Redo()
	{
		if( auto nodeEditor = m_NodeEditor.Access() )
		{
			nodeEditor->SetNodePosition( m_NodeCopy->ID, m_NewPosition );
		}
	}

}
