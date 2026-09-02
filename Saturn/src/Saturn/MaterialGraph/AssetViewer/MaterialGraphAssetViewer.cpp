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
#include "MaterialGraphAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/MaterialGraph/Graph/MatGraph2_NodeLibrary.h"

namespace Saturn {

	MaterialGraph2AssetViewer::MaterialGraph2AssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::MaterialGraph;
		AddMaterialGraph();
	}

	MaterialGraph2AssetViewer::~MaterialGraph2AssetViewer()
	{
	}

	void MaterialGraph2AssetViewer::OnImGuiRender()
	{
		if( m_MaterialGraph->IsOpen() )
		{
			m_MaterialGraph->OnImGuiRender();
		}
		else
		{
			m_MaterialGraph->OpenWindow( false );
			m_Open = false;
		}
	}

	void MaterialGraph2AssetViewer::OnUpdate( Timestep ts )
	{
	}

	void MaterialGraph2AssetViewer::OnEvent( Event& rEvent )
	{
	}

	void MaterialGraph2AssetViewer::AddMaterialGraph()
	{
		m_Asset = AssetManager::Get()->FindAsset( m_AssetID );
		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		const std::string filename = std::format( "{0}.smg", m_Asset->Name );

		m_MaterialGraph = SharedPtr<MatGraph2_Graph>::Create( m_AssetID );
		m_MaterialGraph->SetupNewNodeEditor();
		m_MaterialGraph->NcSetCustomName( filename );
		m_MaterialGraph->SetWindowName( m_Name );
		m_MaterialGraph->OpenWindow( true );

		m_Open = true;

		SetupNodeEditorCallbacks();
	}

	void MaterialGraph2AssetViewer::SetupNewNodeEditor()
	{
		m_OutputNodeID = m_MaterialGraph->SetupNewNodeEditor()->ID;
		m_MaterialGraph->OnNodeEditorEvent( NodeEditorAction::PostLoad );
		MarkDirty();
	}

	void MaterialGraph2AssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_MaterialGraph->SetCreateNewNodeFunction(
			[ & ]() -> SharedPtr<NodeEditorNodeBase>
		{
			SharedPtr<NodeEditorNodeBase> result = nullptr;

			ImGui::SeparatorText( "Basic" );

			if( ImGui::MenuItem( "Constant Colour" ) )
				result = ( SharedPtr<NodeEditorNodeBase> ) MatGraph2_NodeLibrary::SpawnConstantColourNode( m_MaterialGraph );

			if( ImGui::MenuItem( "Sample Texture" ) )
				result = ( SharedPtr<NodeEditorNodeBase> ) MatGraph2_NodeLibrary::SpawnTextureSampleNode( m_MaterialGraph );

			return result;
		} );
#endif
	}

#if !defined(SAT_DIST)
	void MaterialGraph2AssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
	{
	}
#endif

}
