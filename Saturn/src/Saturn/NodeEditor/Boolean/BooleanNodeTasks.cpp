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
#include "BooleanNodeTasks.h"

#include "BooleanNodes.h"

#include "Saturn/NodeEditor/DataLine.h"
#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	SNotBoolTask::SNotBoolTask()
	{
	}

	void SNotBoolTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode )
	{
		m_pHandler = pHandler;

		SNotNode* pNotNode = dynamic_cast< SNotNode* >( pNode );
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
						m_ValueToTest = pInputData->GetIf<bool>();
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

	SNotBoolTask::~SNotBoolTask()
	{
	}

	NodeEditorTaskState SNotBoolTask::Tick( Timestep ts )
	{
		m_Result = !*m_ValueToTest;

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

	void SNotBoolTask::Reset()
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

SAT_X31_CREATE_AUTO_REG( SNotBoolTask );
