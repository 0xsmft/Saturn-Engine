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
#include "MatGraph2_Graph.h"

#include "Nodes/MatGraph2_Nodes.h"
#include "MatGraph2_NodeLibrary.h"

namespace Saturn {

	MatGraph2_Graph::MatGraph2_Graph( AssetID id )
		: NodeEditor( id )
	{
	}

	MatGraph2_Graph::~MatGraph2_Graph()
	{
	}

	SharedPtr<NodeEditorNodeBase> MatGraph2_Graph::SetupNewNodeEditor()
	{
		ed::SetCurrentEditor( m_Editor );

		auto node = MatGraph2_NodeLibrary::SpawnOutputNode( SharedFromThis() );
		ed::SetNodePosition( ed::NodeId( node->ID ), { 0.0f, 0.0f } );

		return node;
	}

#if !defined(SAT_DIST)
	void MatGraph2_Graph::OnImGuiRender()
	{
		NodeEditor::OnImGuiRender();
	}

	void MatGraph2_Graph::OnTopBarRender()
	{

	}

	void MatGraph2_Graph::OnNodeEditorEvent( NodeEditorAction action )
	{

	}
#endif

}
