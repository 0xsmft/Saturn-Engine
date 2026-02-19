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
#include "MathsTasks.h"

#include "MathsNodes.h"

#include "Saturn/NodeEditor/DataLine.h"
#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"
#include "Saturn/NodeEditor/Link.h"

namespace Saturn {

	SMathsAddFloatsTask::SMathsAddFloatsTask()
	{
	}

	SMathsAddFloatsTask::~SMathsAddFloatsTask()
	{
	}

	void SMathsAddFloatsTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode )
	{
		/*
		MathsAddFloats* pMathsNode = dynamic_cast< MathsAddFloats* >( pNode );
		if( pMathsNode )
		{
			Ref<FloatPin> fpin = pMathsNode->Inputs[ 0 ].As<FloatPin>();
			if( fpin )
			{
				m_A = fpin->Data;
			}

			fpin = pMathsNode->Inputs[ 1 ].As<FloatPin>();
			if( fpin )
			{
				m_B = fpin->Data;
			}

			m_NodeID = pMathsNode->ID;
		}
		*/
	}

	NodeEditorTaskState SMathsAddFloatsTask::Tick( Timestep ts )
	{
		if( !m_OperationCompleted ) 
		{
			m_Result = ( *m_A ) + ( *m_B );
			m_OperationCompleted = true;
		}

		return NodeEditorTaskState::Completed;
	}

	void SMathsAddFloatsTask::Reset()
	{
		m_Result = 0.0f;
	}
	
	//////////////////////////////////////////////////////////////////////////

	SMathsGreaterThanFloatsTask::SMathsGreaterThanFloatsTask()
	{
	}

	SMathsGreaterThanFloatsTask::~SMathsGreaterThanFloatsTask()
	{
	}

	void SMathsGreaterThanFloatsTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode )
	{
		m_pHandler = pHandler;

		// Get the real C++ float values
		MathsGreaterThanFloats* pMathsNode = dynamic_cast< MathsGreaterThanFloats* >( pNode );
		if( pMathsNode )
		{	
			auto links = pBase->FindLinksByPin( pMathsNode->Inputs[ 0 ]->ID );
			for( const auto& rLink : links )
			{
				// Get data line
				if( m_pHandler->DoesDataLineExist( rLink->ID ) ) 
				{
					auto* pInputData = m_pHandler->GetDataLine( rLink->ID );
					if( pInputData )
					{
						m_ValueToTest = pInputData->GetIf<float>();
					}
				}
			}

			auto fpin = pMathsNode->Inputs[ 1 ].As<FloatPin>();
			if( fpin )
			{
				if( pBase->IsLinked( fpin->ID ) )
				{
					links = pBase->FindLinksByPin( fpin->ID );

					// Get data line
					if( m_pHandler->DoesDataLineExist( links[ 0 ]->ID ) )
					{
						auto* pInputData = m_pHandler->GetDataLine( links[ 0 ]->ID );
						if( pInputData )
						{
							m_ValueToTest = pInputData->GetIf<float>();
						}
					}
				}
				else
					m_Threshold = &fpin->Data;
			}

			links = pBase->FindLinksByPin( pMathsNode->Outputs[ 0 ]->ID );
			Outgoings.reserve( links.size() );

			// We output a bool.
			for( const auto& rLink : links )
			{
				DataLine dl;
				dl.WriteValue<bool>( false );
				m_pHandler->InsertDataLine( rLink->ID, dl );
			
				Outgoings.push_back( rLink->ID );
			}

			m_NodeID = pNode->ID;
		}
	}

	NodeEditorTaskState SMathsGreaterThanFloatsTask::Tick( Timestep ts )
	{
		if( *m_ValueToTest > *m_Threshold )
		{
			m_Result = true;
		}

		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}

		return NodeEditorTaskState::Completed;
	}

	void SMathsGreaterThanFloatsTask::Reset()
	{
		m_Result = false;
		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	SMathsLessThanFloatsTask::SMathsLessThanFloatsTask()
	{
	}

	SMathsLessThanFloatsTask::~SMathsLessThanFloatsTask()
	{
	}

	void SMathsLessThanFloatsTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode )
	{
		m_pHandler = pHandler;

		// Get the real C++ float values
		MathsLessThanFloats* pMathsNode = dynamic_cast< MathsLessThanFloats* >( pNode );
		if( pMathsNode )
		{
			auto links = pBase->FindLinksByPin( pMathsNode->Inputs[ 0 ]->ID );
			for( const auto& rLink : links )
			{
				// Get data line
				if( m_pHandler->DoesDataLineExist( rLink->ID ) )
				{
					auto* pInputData = m_pHandler->GetDataLine( rLink->ID );
					if( pInputData )
					{
						m_ValueToTest = pInputData->GetIf<float>();
					}
				}
			}

			auto fpin = pMathsNode->Inputs[ 1 ].As<FloatPin>();
			if( fpin )
			{
				if( pBase->IsLinked( fpin->ID ) )
				{
					links = pBase->FindLinksByPin( fpin->ID );

					// Get data line
					if( m_pHandler->DoesDataLineExist( links[ 0 ]->ID ) )
					{
						auto* pInputData = m_pHandler->GetDataLine( links[ 0 ]->ID );
						if( pInputData )
						{
							m_ValueToTest = pInputData->GetIf<float>();
						}
					}
				}
				else
					m_Threshold = &fpin->Data;
			}

			links = pBase->FindLinksByPin( pMathsNode->Outputs[ 0 ]->ID );
			Outgoings.reserve( links.size() );

			// We output a bool.
			for( const auto& rLink : links )
			{
				DataLine dl;
				dl.WriteValue<bool>( false );
				m_pHandler->InsertDataLine( rLink->ID, dl );

				Outgoings.push_back( rLink->ID );
			}

			m_NodeID = pNode->ID;
		}
	}

	NodeEditorTaskState SMathsLessThanFloatsTask::Tick( Timestep ts )
	{
		if( *m_ValueToTest < *m_Threshold )
		{
			m_Result = true;
		}

		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}

		return NodeEditorTaskState::Completed;
	}

	void SMathsLessThanFloatsTask::Reset()
	{
		m_Result = false;
		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	SMathsNotTask::SMathsNotTask()
		: Super()
	{

	}

	SMathsNotTask::~SMathsNotTask()
	{
	}

	void SMathsNotTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode )
	{
		m_pHandler = pHandler;

		MathsNot* pNotNode = dynamic_cast< MathsNot* >( pNode );
		if( pNotNode )
		{
			auto links = pBase->FindLinksByPin( pNotNode->Inputs[ 0 ]->ID );
			for( const auto& rLink : links )
			{
				// Get data line
				if( m_pHandler->DoesDataLineExist( rLink->ID ) )
				{
					auto* pInputData = m_pHandler->GetDataLine( rLink->ID );
					if( pInputData )
					{
						m_pValueToTest = pInputData->GetIf<bool>();
					}
				}
			}

			links = pBase->FindLinksByPin( pNotNode->Outputs[ 0 ]->ID );
			Outgoings.reserve( links.size() );

			// We output a bool.
			for( const auto& rLink : links )
			{
				DataLine dl;
				dl.WriteValue<bool>( false );
				m_pHandler->InsertDataLine( rLink->ID, dl );

				Outgoings.push_back( rLink->ID );
			}

			m_NodeID = pNode->ID;
		}
	}

	NodeEditorTaskState SMathsNotTask::Tick( Timestep ts )
	{
		m_Result = !*m_pValueToTest;

		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}

		return NodeEditorTaskState::Completed;
	}

	void SMathsNotTask::Reset()
	{
		m_Result = false;
		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	SMathsOrTask::SMathsOrTask()
		: Super()
	{
	}

	SMathsOrTask::~SMathsOrTask()
	{
	}

	void SMathsOrTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode )
	{
		m_pHandler = pHandler;

		MathsNot* pNotNode = dynamic_cast< MathsNot* >( pNode );
		if( pNotNode )
		{
			auto links = pBase->FindLinksByPin( pNotNode->Inputs[ 0 ]->ID );
			for( const auto& rLink : links )
			{
				// Get data line
				if( m_pHandler->DoesDataLineExist( rLink->ID ) )
				{
					auto* pInputData = m_pHandler->GetDataLine( rLink->ID );
					if( pInputData )
					{
						m_pA = pInputData->GetIf<bool>();
					}
				}
			}

			links = pBase->FindLinksByPin( pNotNode->Inputs[ 1 ]->ID );
			for( const auto& rLink : links )
			{
				// Get data line
				if( m_pHandler->DoesDataLineExist( rLink->ID ) )
				{
					auto* pInputData = m_pHandler->GetDataLine( rLink->ID );
					if( pInputData )
					{
						m_pB = pInputData->GetIf<bool>();
					}
				}
			}

			links = pBase->FindLinksByPin( pNotNode->Outputs[ 0 ]->ID );
			Outgoings.reserve( links.size() );

			// We output a bool.
			for( const auto& rLink : links )
			{
				DataLine dl;
				dl.WriteValue<bool>( false );
				m_pHandler->InsertDataLine( rLink->ID, dl );

				Outgoings.push_back( rLink->ID );
			}

			m_NodeID = pNode->ID;
		}
	}

	NodeEditorTaskState SMathsOrTask::Tick( Timestep ts )
	{
		m_Result = ( *m_pA ) || ( *m_pB );

		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}

		return NodeEditorTaskState::Completed;
	}

	void SMathsOrTask::Reset()
	{
		m_Result = false;
		for( const auto& rID : Outgoings )
		{
			auto* pLine = m_pHandler->GetDataLine( rID );
			if( pLine )
			{
				pLine->WriteValue<bool>( m_Result );
			}
		}
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SMathsAddFloatsTask );
SAT_X31_CREATE_AUTO_REG( SMathsGreaterThanFloatsTask );
SAT_X31_CREATE_AUTO_REG( SMathsLessThanFloatsTask );
SAT_X31_CREATE_AUTO_REG( SMathsNotTask );
SAT_X31_CREATE_AUTO_REG( SMathsOrTask );
