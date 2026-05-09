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
#include "MaterialGraph.h"

#include "MaterialGraphTaskHandler.h"

#include "Saturn/Asset/MaterialAsset.h"

namespace Saturn {

	MaterialGraph::MaterialGraph()
		: NodeEditor()
	{
	}

	MaterialGraph::MaterialGraph( AssetID id )
		: NodeEditor( id )
	{
	}

	MaterialGraph::~MaterialGraph()
	{
	}

	SharedPtr<NodeEditorNodeBase> MaterialGraph::SetupNewNodeEditor( Ref<Material> material )
	{
		return nullptr;
	}

	void MaterialGraph::SetHostMaterialAsset( Ref<MaterialAsset> asset )
	{
		m_HostMaterialAsset = asset;
		m_EditingMaterial = Ref<Material>( m_HostMaterialAsset->GetMaterial() );

		if( !m_TaskHandler )
			m_TaskHandler = Ref<MaterialGraphTaskHandler>::Create( m_HostMaterialAsset->GetMaterial() );
	}

#if !defined(SAT_DIST)
	void MaterialGraph::BuildTaskCache()
	{
		std::vector<SharedPtr<NodeEditorNodeBase>> order;
		order.reserve( m_Nodes.size() );

		SharedPtr<NodeEditorNodeBase> outputNode = FindNode( "Material Output" );
		TraverseFromStart( outputNode, NodeEditorFlowDirection::GoToRootNode,
			[ & ]( const auto id )
		{
			order.push_back( FindNode( id ) );
		} );

		m_TaskCache.BuildMasterList( order );
	}
#endif

#if !defined(SAT_DIST)
	void MaterialGraph::OnNodeEditorEvent( NodeEditorAction action )
	{
		switch( action )
		{
			case NodeEditorAction::PreEvaluate: 
			{
				std::vector<SharedPtr<NodeEditorNodeBase>> order;
				order.reserve( m_Nodes.size() );

				SharedPtr<NodeEditorNodeBase> outputNode = FindNode( "Material Output" );
				TraverseFromStart( outputNode, NodeEditorFlowDirection::GoToRootNode,
					[ & ]( const auto id )
				{
					order.push_back( FindNode( id ) );
				} );

				m_PreCompiler->Init( order );
			} break;

			case NodeEditorAction::PostEvaluateSuccess:
			{
				BuildTaskCache();
				SimulateChanges();

				SaveAndMarkClean();
			} break;

			default:
				break;
		}
	}
#endif

	void MaterialGraph::SimulateChanges()
	{
		m_TaskHandler->Init( m_TaskCache );

		// Tick twice
		// (1) to get the first task
		// (2) to tick the first task
		m_TaskHandler->Tick( 0.0f );
		m_TaskHandler->Tick( 0.0f );
	}

	void MaterialGraph::ApplyMaterialChanges()
	{

	}

}
