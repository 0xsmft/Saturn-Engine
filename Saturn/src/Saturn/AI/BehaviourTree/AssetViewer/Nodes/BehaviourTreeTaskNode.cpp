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
#include "BehaviourTreeTaskNode.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"

namespace Saturn {

	BehaviourTreeTaskNode::BehaviourTreeTaskNode()
		: BehaviourTreeNodeBase( "<NULL TASK>" )
	{
		SetTaskInstance( nullptr );
	}

	BehaviourTreeTaskNode::BehaviourTreeTaskNode( BehaviourTreeBaseTask* pTaskInstance )
#if !defined(SAT_DIST)
		: BehaviourTreeNodeBase( pTaskInstance->GetTaskName() ),
#else
		: BehaviourTreeNodeBase(),
#endif
		m_TaskInstance( pTaskInstance )
	{
		SetTaskInstance( pTaskInstance );
	}

	void BehaviourTreeTaskNode::SetTaskInstance( BehaviourTreeBaseTask* pTaskInstance )
	{
		m_TaskInstance = pTaskInstance;

		CreateNode();

#if !defined(SAT_DIST)
		m_MemVariable = Ref<BlackboardVaraibleSpec>::Create();
#endif
	}

	void BehaviourTreeTaskNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeGeneralTaskNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Tree;
#endif

		Inputs.push_back( Ref<Pin>::Create( "In", PinType::Flow, PinKind::Input ) );

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeTaskNode::~BehaviourTreeTaskNode()
	{
	}

	void BehaviourTreeTaskNode::Serialise( std::ofstream& rStream ) const
	{
		BehaviourTreeNodeBase::Serialise( rStream );

		RawSerialisation::WriteString( m_TaskInstance->GetClass()->GetName(), rStream );

		m_TaskInstance->Serialise( rStream );
	}

	void BehaviourTreeTaskNode::Deserialise( FDependentIStream& rStream )
	{
		BehaviourTreeNodeBase::Deserialise( rStream );

		const std::string& className = RawSerialisation::ReadString( rStream );
	
		BehaviourTreeBaseTask* pObject = dynamic_cast< BehaviourTreeBaseTask* >( ClassMetadataHandler::Get().CreateClassObject( className ) );

		if( pObject )
		{
			// Promote to strong ref.
			m_TaskInstance = pObject;
#if !defined(SAT_DIST)
			Name = m_TaskInstance->GetTaskName();
#endif
		}
		else
			delete ( SObject* )pObject;

		m_TaskInstance->Deserialise( rStream );
	}

	void BehaviourTreeTaskNode::PostDeserialise()
	{
	}

#if !defined( SAT_DIST )
	void BehaviourTreeTaskNode::RenderDetails()
	{
		m_TaskInstance->RenderDetails();

		const std::string dbgText = std::format( "{0} ~ {1}", m_TaskInstance->GetTaskName(), m_TaskInstance->GetClass()->GetName() );
		ImGui::TextDisabled( dbgText.c_str() );
	}

	void BehaviourTreeTaskNode::OnRenderExtra()
	{
		m_TaskInstance->OnRenderExtra();
	}

#endif

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeTaskNode );
