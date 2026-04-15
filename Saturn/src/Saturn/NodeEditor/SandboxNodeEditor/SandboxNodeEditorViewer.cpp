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
#include "SandboxNodeEditorViewer.h"

#include "SandboxNodeLibrary.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

namespace Saturn {

	SandboxNodeEditorViewer::SandboxNodeEditorViewer()
	{
		m_NodeEditor = SharedPtr<SandboxNodeEditor>::Create();

		const std::string customName = std::format( "SndbxNE.nce" );
		if( !NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, 0, customName ) )
		{
			SandboxNodeLibrary::SpawnOutputNode( m_NodeEditor );
		}

#if !defined(SAT_DIST)
		m_NodeEditor->SetWindowName( "SandboxNe" );
		m_NodeEditor->NcSetCustomName( customName );
		m_NodeEditor->OpenWindow( true );
#endif

		m_Name = "SndboxVwr";
		m_Open = true;

		SetupNodeEditorCallbacks();
	}

	SandboxNodeEditorViewer::~SandboxNodeEditorViewer()
	{
		if( m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveAndMarkClean();
		}
	}

	void SandboxNodeEditorViewer::OnImGuiRender()
	{
		if( m_NodeEditor->IsOpen() )
		{
			m_NodeEditor->OnImGuiRender();
		}
		else
		{
			m_NodeEditor->BuildTaskCache();
			m_NodeEditor->OpenWindow( false );
			m_Open = false;
		}
	}

	void SandboxNodeEditorViewer::OnUpdate( Timestep ts )
	{
		if( m_NodeEditor )
		{
			m_NodeEditor->OnUpdate( ts );
		}
	}

	void SandboxNodeEditorViewer::OnEvent( Event& rEvent )
	{
		if( m_NodeEditor )
		{
			m_NodeEditor->OnEvent( rEvent );
		}
	}

	void SandboxNodeEditorViewer::ForceOpenWindow()
	{
		m_NodeEditor->OpenWindow( true );
		m_Open = true;
	}

	void SandboxNodeEditorViewer::SetupNodeEditorCallbacks()
	{
		m_NodeEditor->SetCreateNewNodeFunction(
			[ & ]() -> SharedPtr<NodeEditorNodeBase>
		{
			SharedPtr<NodeEditorNodeBase> result; 

			if( ImGui::MenuItem( "Example node" ) )
			{
				result = ( SharedPtr<NodeEditorNodeBase> )SandboxNodeLibrary::SpawnExampleNode( m_NodeEditor );
			}
		
			if( ImGui::MenuItem( "Base class node" ) )
			{
				result = ( SharedPtr<NodeEditorNodeBase> )SandboxNodeLibrary::SpawnBaseClassNode( m_NodeEditor );
			}

			return result;
		} );
	}

}
