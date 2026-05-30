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
#include "SoundGraph.h"

#include "Nodes/SoundGraphNodes.h"
#include "SoundNodeLibrary.h"

namespace Saturn {
	
	SoundGraph::SoundGraph( AssetID assetID )
		: NodeEditor( assetID )
	{
	}

	SoundGraph::~SoundGraph()
	{
	}

	SharedPtr<NodeEditorNodeBase> SoundGraph::SetupNewNodeEditor()
	{
		ed::SetCurrentEditor( m_Editor );

		auto node = SoundNodeLibrary::SpawnOutputNode( SharedFromThis() );
		ed::SetNodePosition( ed::NodeId( node->ID ), { 0.0f, 0.0f } );

		return node;
	}

#if !defined(SAT_DIST)
	void SoundGraph::OnImGuiRender()
	{
		NodeEditor::OnImGuiRender();
	}

	void SoundGraph::OnTopBarRender()
	{
	}

	void SoundGraph::OnNodeEditorEvent( NodeEditorAction action )
	{
		switch( action )
		{
			case NodeEditorAction::PostLoad: 
			{
				const auto node = FindNode( "Sound Output" );
				if( node )
				{
					m_OutputID = node->ID;
				}
			} break;

			case NodeEditorAction::PreEvaluate:
			{
				std::vector< SharedPtr<NodeEditorNodeBase> > ids;
				TraverseFromStart( FindNode( m_OutputID ), NodeEditorFlowDirection::GoToRootNode, [ & ]( const auto id )
				{
					ids.push_back( FindNode( id ) );
				} );

				std::reverse( ids.begin(), ids.end() );

				m_PreCompiler->Init( ids );
			} break;

			case NodeEditorAction::PostEvaluateSuccess: 
			{
				BuildTaskCache();
				SaveAndMarkClean();
			} break;

			default:
				break;
		}
	}
#endif

	void SoundGraph::BuildTaskCache()
	{
#if !defined(SAT_DIST)
		std::vector< SharedPtr<NodeEditorNodeBase> > ids;
		TraverseFromStart( FindNode( m_OutputID ), NodeEditorFlowDirection::GoToRootNode, [ & ]( const auto id )
		{
			ids.push_back( FindNode( id ) );
		} );

		std::reverse( ids.begin(), ids.end() );

		m_TaskCache.BuildMasterList( ids );
#endif
	}

}
