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
#include "NodeEditorVariableTasks.h"

#include "Link.h"

#include "NodeEditorTaskHandler.h"

#if !defined(SAT_DIST)
#include "NodeEditorNodeBase.h"
#include "NodeEditorBase.h"
#include "NodeEditorVariableNode.h"
#endif

namespace Saturn {

	SNodeEditorGetVariableTask::SNodeEditorGetVariableTask()
	{
	}

	SNodeEditorGetVariableTask::~SNodeEditorGetVariableTask()
	{
	}

#if !defined(SAT_DIST)
	void SNodeEditorGetVariableTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		NodeEditorVariableNode* pOtherNode = dynamic_cast< NodeEditorVariableNode* >( pNode );
		if( pOtherNode )
		{
			if( auto var = pOtherNode->GetVariable(); var ) 
			{
				m_VariableID = var->GetUUID();
			}
		}
	}
#endif

	/*
	void SNodeEditorGetVariableTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		m_pHandler = pHandler;

		NodeEditorVariableNode* pVariableNode = dynamic_cast< NodeEditorVariableNode* >( pNode );
		if( pVariableNode )
		{
			m_Variable = pVariableNode->GetVariable();

			// For each link that this node servers, we will create a new data handle for,
			// Then when we Tick, we'll carry across the value of m_Variable into the data handle.
			//
			// TODO: This system is not great, it may be better for a data handle to just have a pointer to the actual data instead of own a copy of it.
			const auto links = pEditor->FindLinksByPin( pNode->Outputs[ 0 ]->ID );

			Outgoings.reserve( links.size() );
			for( const auto& rLink : links )
			{
				DataLine dl;

				// Init with default value
				switch( m_Variable->GetType() )
				{
					case NodeEditorVariableDataType::Float:
					{
						dl.WriteValue( 0.0f );
					} break;

					case NodeEditorVariableDataType::Int:
					{
						dl.WriteValue( 0 );
					} break;

					case NodeEditorVariableDataType::ID:
					{
						dl.WriteValue( 0llu );
					} break;

					case NodeEditorVariableDataType::Bool:
					{
						dl.WriteValue( false );
					} break;

					case NodeEditorVariableDataType::Vec2:
					{
						dl.WriteValue( glm::vec2{} );
					} break;

					case NodeEditorVariableDataType::Vec3:
					{
						dl.WriteValue( glm::vec3{} );
					} break;

					case NodeEditorVariableDataType::Vec4:
					{
						dl.WriteValue( glm::vec4{} );
					} break;

					case NodeEditorVariableDataType::Class:
					{
						dl.WriteValue( ( SClass* ) 0 );
					} break;

					case NodeEditorVariableDataType::String:
					{
						dl.WriteValue( std::string{} );
					} break;

					default:
						break;
				}

				Outgoings.push_back( rLink->ID );
				// Add new data line
				pHandler->InsertDataLine( rLink->ID, dl );
			}

			m_NodeID = pNode->ID;
		}
	}
	*/

	void SNodeEditorGetVariableTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );
	
		SNodeEditorGetVariableTask* pThisOther = dynamic_cast< SNodeEditorGetVariableTask* >( pOther );
		if( pThisOther )
		{
			m_VariableID = pThisOther->m_VariableID;

			auto var = pHandler->GetVariable( m_VariableID );

			switch( var->GetType() )
			{
				case NodeEditorVariableDataType::Bool:
				{
					pHandler->RegisterLocator<bool>( m_NodeID, 0llu, var->GetPtr<bool>() );
				} break;

				default:
					break;
			}
		}
	}

	NodeEditorTaskState SNodeEditorGetVariableTask::Tick( Timestep ts )
	{
		return NodeEditorTaskState::Completed;
	}

	void SNodeEditorGetVariableTask::Reset()
	{
	}

	void SNodeEditorGetVariableTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObjectChecked( m_VariableID, rStream );
	}

	void SNodeEditorGetVariableTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObjectChecked( m_VariableID, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SNodeEditorGetVariableTask );
