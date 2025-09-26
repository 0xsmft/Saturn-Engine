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
#include "NodeEditorBase.h"

#include "Runtime/NodeEditorRuntime.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

#include "Saturn/GameFramework/SClass.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// NODE EDITOR BASE

	NodeEditorBase::NodeEditorBase() 
		: m_Runtime( Ref<NodeEditorRuntime>::Create() )
	{
	}

	NodeEditorBase::NodeEditorBase( AssetID id )
		: m_AssetID( id ), m_Runtime( Ref<NodeEditorRuntime>::Create() )
	{
	}

	NodeEditorBase::~NodeEditorBase()
	{
		m_Runtime = nullptr;
		ed::DestroyEditor( m_Editor );
		ed::SetCurrentEditor( nullptr );

		for( auto& [id, rNode] : m_Nodes )
		{
			rNode->Destroy();
		}

		m_Links.clear();
		m_Nodes.clear();
	}

	void NodeEditorBase::SaveSettings()
	{
		NodeCacheSettings::WriteEditorSettings( SharedFromThis() );
	}

	static void BuildNode( SharedPtr<NodeEditorNodeBase>& rNode )
	{
		for( auto& input : rNode->Inputs )
		{
			input->Node = rNode;
		}

		for( auto& output : rNode->Outputs )
		{
			output->Node = rNode;
		}
	}

	void NodeEditorBase::SerialiseData( std::ofstream& rStream, bool isForDist )
	{
		RawSerialisation::WriteString( m_Name, rStream );

		size_t mapSize = m_Nodes.size();
		rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( const auto& [key, value] : m_Nodes )
		{
			RawSerialisation::WriteUUID( key, rStream );

			RawSerialisation::WriteObject( value->GetClass()->GetHash(), rStream );

			value->Serialise( rStream, isForDist );
		}

		mapSize = m_Links.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( auto& rLinks : m_Links )
		{
			Link::Serialise( rLinks, rStream );
		}

		mapSize = m_DataHandles.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( const auto& [id, rHandle] : m_DataHandles )
		{
			RawSerialisation::WriteObjectChecked( id, rStream );
			NodeEditorVariable::Serialise( rHandle, rStream );
		}
	}

#if defined(SAT_DIST)
	void NodeEditorBase::DeserialiseData( std::istream& rStream )
	{
		m_State = NodeEditorState::Loading;

		NodeCacheSettings::ReadEditorSettings( SharedFromThis() );

		m_Name = RawSerialisation::ReadString( rStream );

		size_t mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		for( size_t i = 0; i < mapSize; i++ )
		{
			UUID key = 0;
			UUID::Deserialise( key, rStream );

			uint64_t targetClassHash = 0;
			RawSerialisation::ReadObject( targetClassHash, rStream );

			NodeEditorNodeBase* pNode = dynamic_cast< NodeEditorNodeBase* >( ClassMetadataHandler::Get().CreateClassObject( targetClassHash ) );

			SharedPtr<NodeEditorNodeBase> node = pNode;
			if( node )
			{
				AddNode( node );
			}
			else
			{
				node = SharedPtr<NodeEditorBlueprintNode>::Create();
			}

			node->Deserialise( rStream );

			m_Nodes[ key ] = node;
			BuildNode( node );
		}

		mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		m_Links.resize( mapSize );

		for( size_t i = 0; i < mapSize; i++ )
		{
			Ref<Link> link = Ref<Link>::Create();

			Link::Deserialise( link, rStream );

			m_Links[ i ] = link;
		}

		m_State = NodeEditorState::Editing;
	}
#endif

	bool NodeEditorBase::IsLinked( UUID pinID )
	{
		if( !pinID )
			return false;

		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[pinID]( const auto& rLink )
			{
				return rLink->StartPinID == pinID || rLink->EndPinID == pinID;
			} );

		if( Itr != m_Links.end() )
			return true;

		return false;
	}

	Ref<Pin> NodeEditorBase::FindPin( UUID id )
	{
		if( !id )
			return nullptr;

		for( const auto& [ nodeId, rNode ] : m_Nodes )
		{
			for( const auto& pin : rNode->Inputs )
			{
				if( pin->ID == id )
				{
					return pin;
				}
			}

			for( const auto& pin : rNode->Outputs )
			{
				if( pin->ID == id )
				{
					return pin;
				}
			}
		}

		return nullptr;
	}

	Ref<Link> NodeEditorBase::FindLink( UUID id )
	{
		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[id]( const auto& rLink )
			{
				return rLink->ID == id;
			} );

		if( Itr != m_Links.end() )
			return *Itr;

		return nullptr;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditorBase::FindNode( UUID id )
	{
		const auto Itr = m_Nodes.find( id );

		if( Itr != m_Nodes.end() )
			return Itr->second;

		return nullptr;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditorBase::FindNode( const std::string& rName )
	{
		for( auto& [ id, node ]: m_Nodes )
		{
			if( node->Name == rName )
				return node;
		}

		return nullptr;
	}

	Ref<Link> NodeEditorBase::FindLinkByPin( UUID id )
	{
		if( id == 0 )
			return nullptr;

		if( !IsLinked( id ) )
			return nullptr;

		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[ id ]( const auto& rLink )
		{
			return rLink->StartPinID == id || rLink->EndPinID == id;
		} );

		if( Itr != m_Links.end() )
			return *Itr;

		return nullptr;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditorBase::FindNodeByPin( UUID id )
	{
		if( auto rPin = FindPin( id ) ) 
			return rPin->Node;
		
		return nullptr;
	}

	std::vector<Ref<Link>> NodeEditorBase::FindLinksByPin( UUID id )
	{
		std::vector<Ref<Link>> result;

		if( id == 0 )
			return result;

		for( const auto& rLink : m_Links )
		{
			if( rLink->StartPinID == id || rLink->EndPinID == id )
			{
				result.push_back( rLink );
			}
		}

		return result;
	}

	void NodeEditorBase::SetRuntime( Ref<NodeEditorRuntime> runtime )
	{
		m_Runtime = runtime;
	}

	NodeEditorCompilationStatus NodeEditorBase::Evaluate()
	{
		if( !m_Runtime || m_State == NodeEditorState::Loading || !HasPrivilege( NodeEditorUserAuthority::Evaluation ) )
			return NodeEditorCompilationStatus::Failed;

		m_State = NodeEditorState::Evaluating;

		return m_Runtime->EvaluateEditor();
	}

	std::vector<UUID> NodeEditorBase::FindNeighborsRight( SharedPtr<NodeEditorNodeBase> node )
	{
		std::vector<UUID> ids;

		for( const auto& rInput : node->Inputs )
		{
			if( !IsLinked( rInput->ID ) )
				continue;

			// If the pin is linked find the other end of it and add it to our list.
			const auto links = FindLinksByPin( rInput->ID );
			for( const auto& rLink : links )
			{
				const bool isStart = rLink->StartPinID == rInput->ID;
				SharedPtr<NodeEditorNodeBase> otherNode = FindNodeByPin( isStart ? rLink->EndPinID : rLink->StartPinID );

				ids.push_back( otherNode->ID );
			}
		}

		return ids;
	}

	std::vector<UUID> NodeEditorBase::FindNeighborsLeft( SharedPtr<NodeEditorNodeBase> node )
	{
		std::vector<UUID> ids;

		for( const auto& rOutput : node->Outputs )
		{
			if( !IsLinked( rOutput->ID ) )
				continue;

			// If the pin is linked find the other end of it and add it to our list.
			const auto links = FindLinksByPin( rOutput->ID );

			for( const auto& rLink : links )
			{
				const bool isStart = rLink->StartPinID == rOutput->ID;
				SharedPtr<NodeEditorNodeBase> otherNode = FindNodeByPin( isStart ? rLink->EndPinID : rLink->StartPinID );

				ids.push_back( otherNode->ID );
			}
		}

		return ids;
	}

	void NodeEditorBase::CreateLink( const Ref<Pin>& rStart, const Ref<Pin>& rEnd )
	{
		m_Links.push_back( Ref<Link>::Create( UUID(), rStart->ID, rEnd->ID, rStart->GetPinColor() ) );
	}

	void NodeEditorBase::CreateLinkWithID( UUID linkID, const Ref<Pin>& rStart, const Ref<Pin>& rEnd )
	{
		m_Links.push_back( Ref<Link>::Create( linkID, rStart->ID, rEnd->ID, rStart->GetPinColor() ) );
	}

	void NodeEditorBase::ShowFlow()
	{
		for( const auto& rLink : m_Links )
			ed::Flow( ed::LinkId( rLink->ID ) );
	}

	void NodeEditorBase::ShowFlow( const std::vector<Ref<Link>>& rLinks )
	{
		for( const auto& rLink : rLinks )
			ed::Flow( ed::LinkId( rLink->ID ) );
	}

	void NodeEditorBase::ShowFlow( const Ref<Link>& rLink )
	{
		ed::Flow( ed::LinkId( rLink->ID ) );
	}

	void NodeEditorBase::ShowFlow( UUID linkID )
	{
		ed::Flow( ed::LinkId( linkID ) );
	}

	bool NodeEditorBase::HasPrivilege( NodeEditorUserAuthority privilege ) const
	{
		return ( m_Privileges & privilege ) == privilege;
	}

	void NodeEditorBase::SetPrivileges( NodeEditorUserAuthority privilege, bool value )
	{
		if( value )
			m_Privileges = m_Privileges | privilege;
		else
			m_Privileges = m_Privileges & ~privilege;
	}

	void NodeEditorBase::AddNode( SharedPtr<NodeEditorNodeBase> node )
	{
		if( m_State == NodeEditorState::Loading )
			return;

		m_Nodes[ node->ID ] = node;

		BuildNode( node );

#if !defined(SAT_DIST)
		VariableGuard<ed::EditorContext*> guard( m_Editor );

		// TODO: add this back
//		if( node->Position.x != 0.0f && node->Position.y != 0.0f )
//			ed::SetNodePosition( ed::NodeId( node->ID ), node->Position );
#endif

		node->pOuter = this;
	}

}
