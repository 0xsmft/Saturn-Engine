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

#include "Saturn/ImGui/AssetViewer.h"

#include "NodeEditorCompilationStatus.h"

#include "Node.h"
#include "Link.h"

#include "imgui_node_editor.h"

#include <stack>

namespace ed = ax::NodeEditor;

namespace Saturn {

	enum class NodeEditorType
	{
		Unknown,
		Default,
		Material,
		Sound
	};

	class NodeEditorRuntime;
	class Texture2D;

	class NodeEditorBase : public RefTarget
	{
	public:
		NodeEditorBase();
		NodeEditorBase( AssetID id );
		virtual ~NodeEditorBase();

#if !defined(SAT_DIST)
		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate( Timestep ts ) = 0;
		virtual void OnEvent( RubyEvent& rEvent ) = 0;
#else
		virtual void OnImGuiRender() {}
		virtual void OnUpdate( Timestep ts ) {}
		virtual void OnEvent( RubyEvent& rEvent ) {}
#endif

		static Ref<Texture2D> GetBlueprintBackground();

		bool IsLinked( UUID pinID );
		Ref<Pin> FindPin( UUID id );
		Ref<Link> FindLink( UUID id );
		Ref<Node> FindNode( UUID id );
		Ref<Node> FindNode( const std::string& rName );
		Ref<Link> FindLinkByPin( UUID id );
		Ref<Node> FindNodeByPin( UUID id );

		void SetRuntime( Ref<NodeEditorRuntime> runtime );
		NodeEditorCompilationStatus Evaluate();

		std::vector<UUID> FindNeighbors( Ref<Node> node );

		template<typename Function>
		void TraverseFromStart( const Ref<Node>& rRootNode, Function func ) 
		{
			// Last in first out
			// So add our output node then add neighbors and continue as we want the last node with no more neighbors to be evaluated first.

			std::stack<UUID> stack;
			stack.push( rRootNode->ID );

			while( !stack.empty() )
			{
				const auto currentID = stack.top();
				stack.pop();

				func( currentID );

				// Find neighbors from inputs and continue until there is no neighbors
				Ref<Node> nextNode = FindNode( currentID );
				for( const auto& rNeighbor : FindNeighbors( nextNode ) )
				{
					stack.push( rNeighbor );
				}
			}
		}

		void CreateLink( const Ref<Pin>& rStart, const Ref<Pin>& rEnd );
		void ShowFlow();

	public:
		AssetID GetAssetID() const { return m_AssetID; }

		const std::map<UUID, Ref<Node>>& GetNodes() const { return m_Nodes; }
		std::map<UUID, Ref<Node>>& GetNodes() { return m_Nodes; }

		const std::vector<Ref<Link>>& GetLinks() const { return m_Links; }
		std::vector<Ref<Link>>& GetLinks() { return m_Links; }

		void AddNode( Ref<Node> node );

		bool IsOpen() const { return m_WindowOpen; }

		void SaveSettings();

	public:
#if !defined(SAT_DIST)
		virtual void SerialiseData( std::ofstream& rStream ) = 0;
		virtual void DeserialiseData( std::ifstream& rStream ) = 0;
#else
		virtual void SerialiseData( std::ofstream& rStream );
		virtual void DeserialiseData( std::ifstream& rStream );
#endif

	protected:
		std::string m_Name;
		bool m_WindowOpen = false;

		ed::EditorContext* m_Editor = nullptr;
		std::string m_ActiveNodeEditorState;

		std::map<UUID, Ref<Node>> m_Nodes;
		std::vector<Ref<Link>> m_Links;

		Ref<NodeEditorRuntime> m_Runtime;

		AssetID m_AssetID = 0;

		bool m_Loading = false;

	private:
		friend class NodeEditorCache;
		friend class NodeCacheEditor;
		friend class NodeCacheSettings;
	};
}