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

	void SNodeEditorGetVariableTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		SAT_CORE_ASSERT( pHandler );
		Super::InitialiseTaskWithOther( pHandler, pOther );
	
		SNodeEditorGetVariableTask* pThisOther = dynamic_cast< SNodeEditorGetVariableTask* >( pOther );
		if( pThisOther )
		{
			m_VariableID = pThisOther->m_VariableID;

			RefreshLocators();
		}
	}

#define SAT_GETVARTASK_REFRESHLOCATOR( CppType )								\
m_pHandler->RegisterLocator<CppType>( m_NodeID, 0llu, var->GetPtr<CppType>() )  \

	void SNodeEditorGetVariableTask::RefreshLocators() 
	{
		SAT_CORE_ASSERT( m_pHandler );

		auto var = m_pHandler->GetVariable( m_VariableID );
		switch( var->GetType() )
		{
			case NodeEditorVariableDataType::Float:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( float );
			} break;

			case NodeEditorVariableDataType::Int:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( int );
			} break;

			case NodeEditorVariableDataType::ID:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( uint64_t );
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( bool );
			} break;

			case NodeEditorVariableDataType::Vec2:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( glm::vec2 );
			} break;

			case NodeEditorVariableDataType::Vec3:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( glm::vec3 );
			} break;

			case NodeEditorVariableDataType::Vec4:
			{
				SAT_GETVARTASK_REFRESHLOCATOR( glm::vec4 );
			} break;

			default:
				Core::BreakDebug();
				break;
		}
	}

	NodeEditorTaskState SNodeEditorGetVariableTask::Tick( Timestep ts )
	{
		RefreshLocators();
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

	//////////////////////////////////////////////////////////////////////////
	// SNodeEditorSetVariableTask

	SNodeEditorSetVariableTask::SNodeEditorSetVariableTask()
	{
	}

	SNodeEditorSetVariableTask::~SNodeEditorSetVariableTask()
	{
	}

#if !defined(SAT_DIST)
	void SNodeEditorSetVariableTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		const NodeEditorSetVariableNode* pOtherNode = dynamic_cast< const NodeEditorSetVariableNode* >( pNode );
		if( pOtherNode )
		{
			auto var = pOtherNode->GetVariable();
			
			// TODO: Need to properly handle this in the future...
			SAT_CORE_ASSERT( var );

			m_VariableID = var->GetUUID();

			// Not enough inputs for this node!
			// we need at least two.
			SAT_CORE_ASSERT( pOtherNode->Inputs.size() >= 2 );
			
			// because the node looks like this:
			/*
			* -----------------------------------------------
			* [				   SET MY VARIABLE				]
			* -----------------------------------------------
			* | -> IN EXEC (0)					OUT EXEC -> |
			* |											    |
			* | -> IN VAL (1)					 OUT VAL -> |
			* -----------------------------------------------
			* 
			* NB: In value does not have to be linked! We must account for that.
			*	  If it is linked then because inputs do not have locators we must find what node is linked to it 
			*     and get the locator.
			*	  However, if it's not linked then it's much simpler as all we need to do is read the pin data and save
			*	  it, and that will be used when we Tick.
			*/

			if( const auto link = pEditor->FindLinkByPin( pNode->Inputs[ 1 ]->ID ) )
			{
				m_IsNewValueLinked = true;
				m_InNewValueOtherPinID = link->StartPinID;
			}
			else
			{
				// if we aren't linked, then it's much much harder to deal with...
				// because we now need to store the default value, sounds easy!
				// but no, because at compile time we don't know what the type
				// of the default value, we only know that at runtime time,
				// we must store the variable in a type-safe union.
				// i.e. std::variant, which is annoying and I would prefer
				// if we didn't have to use it.

				m_VariableType = var->GetType();

				switch( m_VariableType )
				{
					case NodeEditorVariableDataType::Float:
					{
						Ref<FloatPin> dataPin = pNode->Inputs[ 1 ].As<FloatPin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					case NodeEditorVariableDataType::Int:
					{
						Ref<IntPin> dataPin = pNode->Inputs[ 1 ].As<IntPin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					case NodeEditorVariableDataType::ID:
					{
						Ref<UInt64Pin> dataPin = pNode->Inputs[ 1 ].As<UInt64Pin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					case NodeEditorVariableDataType::Bool:
					{
						Ref<BoolPin> dataPin = pNode->Inputs[ 1 ].As<BoolPin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					case NodeEditorVariableDataType::Vec2:
					{
						Ref<Vec2Pin> dataPin = pNode->Inputs[ 1 ].As<Vec2Pin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					case NodeEditorVariableDataType::Vec3:
					{
						Ref<Vec3Pin> dataPin = pNode->Inputs[ 1 ].As<Vec3Pin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					case NodeEditorVariableDataType::Vec4:
					{
						Ref<Vec4Pin> dataPin = pNode->Inputs[ 1 ].As<Vec4Pin>();
						m_DefaultValueUnion = dataPin->Data;
					} break;

					default:
						break;
				}
			}
		}
	}
#endif

	void SNodeEditorSetVariableTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		const SNodeEditorSetVariableTask* pThisOther = dynamic_cast< const SNodeEditorSetVariableTask* >( pOther );
		if( pThisOther )
		{
			m_VariableID		   = pThisOther->m_VariableID;
			m_InNewValueOtherPinID = pThisOther->m_InNewValueOtherPinID;
			m_DefaultValueUnion    = pThisOther->m_DefaultValueUnion;
			m_IsNewValueLinked     = pThisOther->m_IsNewValueLinked;
			m_VariableType         = pThisOther->m_VariableType;
		}
	}

#define SAT_SETVARTASK_SETVALUE( CppType )																   \
const auto* const pValueToSet = m_pHandler->AccessLocator<const CppType>( m_InNewValueOtherPinID, 0llu );  \
if( pValueToSet )																						   \
{																										   \
	var->Set<CppType>( *pValueToSet );																	   \
}																										   \
else																									   \
{																										   \
	var->Set( std::get<CppType>( m_DefaultValueUnion ) );												   \
}

	NodeEditorTaskState SNodeEditorSetVariableTask::Tick( Timestep ts )
	{
		// Update variable with value specified in locator m_InNewValuePinID
		// we need to do this every time we tick because the value in m_InNewValuePinID may change
		// TODO: Constant expression check if pin is not linked.

		auto var = m_pHandler->GetVariable( m_VariableID );
		SAT_CORE_ASSERT( var );

		switch( var->GetType() )
		{
			case NodeEditorVariableDataType::Float:
			{
				SAT_SETVARTASK_SETVALUE( float )
			} break;

			case NodeEditorVariableDataType::Int:
			{
				SAT_SETVARTASK_SETVALUE( int )
			} break;

			case NodeEditorVariableDataType::ID:
			{
				SAT_SETVARTASK_SETVALUE( uint64_t )
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				SAT_SETVARTASK_SETVALUE( bool )
			} break;

			case NodeEditorVariableDataType::Vec2:
			{
				SAT_SETVARTASK_SETVALUE( glm::vec2 )
			} break;

			case NodeEditorVariableDataType::Vec3:
			{
				SAT_SETVARTASK_SETVALUE( glm::vec3 )
			} break;

			case NodeEditorVariableDataType::Vec4:
			{
				SAT_SETVARTASK_SETVALUE( glm::vec4 )
			} break;

			default:
				Core::BreakDebug();
				break;
		}

		return NodeEditorTaskState::Completed;
	}

	void SNodeEditorSetVariableTask::Reset()
	{
	}

#define SAT_SETVALUETASK_SERIALISE_DATA( CppType )			\
const auto val = std::get<CppType>( m_DefaultValueUnion );	\
RawSerialisation::WriteObject( val, rStream )

	void SNodeEditorSetVariableTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_VariableID, rStream );
		RawSerialisation::WriteObject( m_InNewValueOtherPinID, rStream );
		RawSerialisation::WriteObject( m_IsNewValueLinked, rStream );
		RawSerialisation::WriteObject( m_VariableType, rStream );

		switch( m_VariableType )
		{
			case NodeEditorVariableDataType::Float:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( float );
			} break;

			case NodeEditorVariableDataType::Int:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( int );
			} break;
			
			case NodeEditorVariableDataType::ID:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( uint64_t );
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( bool );
			} break;

			case NodeEditorVariableDataType::Vec2:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( glm::vec2 );
			} break;

			case NodeEditorVariableDataType::Vec3:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( glm::vec3 );
			} break;

			case NodeEditorVariableDataType::Vec4:
			{
				SAT_SETVALUETASK_SERIALISE_DATA( glm::vec4 );
			} break;

			default:
				break;
		}
	}

#define SAT_SETVALUETASK_DESERIALSE_DATA( CppType )		\
CppType defaultVal{};									\
RawSerialisation::ReadObject( defaultVal, rStream );	\
m_DefaultValueUnion = defaultVal


	void SNodeEditorSetVariableTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_VariableID, rStream );
		RawSerialisation::ReadObject( m_InNewValueOtherPinID, rStream );
		RawSerialisation::ReadObject( m_IsNewValueLinked, rStream );
		RawSerialisation::ReadObject( m_VariableType, rStream );

		switch( m_VariableType )
		{
			case NodeEditorVariableDataType::Float:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( float );
			} break;

			case NodeEditorVariableDataType::Int:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( int );
			} break;

			case NodeEditorVariableDataType::ID:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( uint64_t );
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( bool );
			} break;

			case NodeEditorVariableDataType::Vec2:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( glm::vec2 );
			} break;

			case NodeEditorVariableDataType::Vec3:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( glm::vec3 );
			} break;

			case NodeEditorVariableDataType::Vec4:
			{
				SAT_SETVALUETASK_DESERIALSE_DATA( glm::vec4 );
			} break;

			default:
				break;
		}
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SNodeEditorGetVariableTask );
SAT_X31_CREATE_AUTO_REG( SNodeEditorSetVariableTask );
