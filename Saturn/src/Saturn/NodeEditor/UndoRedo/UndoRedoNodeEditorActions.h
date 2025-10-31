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

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"
#include "Saturn/NodeEditor/Link.h"

#include "Saturn/NodeEditor/UI/NodeEditor.h"

#include "Saturn/ImGui/UndoRedo/UndoRedoActionBase.h"

namespace Saturn {

	enum class UndoRedoActionNodeEditorLinkOp
	{
		Create,
		Delete
	};

	template<UndoRedoActionNodeEditorLinkOp Operation>
	class UndoRedoActionNodeEditorLink : public UndoRedoActionBase
	{
	public:
		UndoRedoActionNodeEditorLink( SharedPtr<NodeEditor> nodeEditor, Ref<Link> originalLink )
			: UndoRedoActionBase( "Create Link" ), m_NodeEditor( nodeEditor )
		{
			m_LinkCopy = Ref<Link>::Create( originalLink->ID, originalLink->StartPinID, originalLink->EndPinID, originalLink->Color );
		}

		~UndoRedoActionNodeEditorLink() 
		{
		}

	public:
		void Undo() override 
		{
			if( auto nodeEditor = m_NodeEditor.Access() )
			{
				if constexpr( Operation == UndoRedoActionNodeEditorLinkOp::Create )
				{
					// TODO: Set Pin color
					nodeEditor->CreateLinkWithID( m_LinkCopy->ID, nodeEditor->FindPin( m_LinkCopy->StartPinID ), nodeEditor->FindPin( m_LinkCopy->EndPinID ), ImColor( 255, 255, 255 ) );
				}
				else
				{
					nodeEditor->DeleteLink( m_LinkCopy->ID, true );
				}
			}
		}

		void Redo() override
		{
			if( auto nodeEditor = m_NodeEditor.Access() )
			{
				if constexpr( Operation == UndoRedoActionNodeEditorLinkOp::Create )
				{
					nodeEditor->DeleteLink( m_LinkCopy->ID, true );
				}
				else
				{
					// TODO: Set Pin color
					nodeEditor->CreateLinkWithID( m_LinkCopy->ID, nodeEditor->FindPin( m_LinkCopy->StartPinID ), nodeEditor->FindPin( m_LinkCopy->EndPinID ), ImColor( 255, 255, 255 ) );
				}
			}
		}

	private:
		WeakRef<NodeEditor> m_NodeEditor;
		Ref<Link> m_LinkCopy;
	};

	using UndoRedoActionCreateLink = UndoRedoActionNodeEditorLink<UndoRedoActionNodeEditorLinkOp::Create>;
	using UndoRedoActionDeleteLink = UndoRedoActionNodeEditorLink<UndoRedoActionNodeEditorLinkOp::Delete>;

	//////////////////////////////////////////////////////////////////////////
	// MODIFY NODE POSITON

	// NOTE: This action will be submitted to the global list BEFORE it has been executed, Undo and Redo function will still work the same though
	class UndoRedoActionModifyNodePosition : public UndoRedoActionBase
	{
	public:
		UndoRedoActionModifyNodePosition( SharedPtr<NodeEditor> nodeEditor, SharedPtr<NodeEditorNodeBase> originalNode, const ImVec2& rOldPosition );
		~UndoRedoActionModifyNodePosition();

	public:
		void Undo() override;
		void Redo() override;

	private:
		WeakRef<NodeEditor> m_NodeEditor;
		UUID m_NodeID = 0;

		ImVec2 m_OldPosition{};
		ImVec2 m_NewPosition{};
	};

}
