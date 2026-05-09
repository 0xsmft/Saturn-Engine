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
#include "MaterialGraphTasks.h"

#include "MaterialGraphTaskHandler.h"

#include "MaterialGraphNodes.h"
#include "MaterialGraphColorPin.h"

namespace Saturn {

	SMaterialGraphColorPickerTask::SMaterialGraphColorPickerTask()
	{
	}

	SMaterialGraphColorPickerTask::~SMaterialGraphColorPickerTask()
	{
	}

#if !defined(SAT_DIST)
	void SMaterialGraphColorPickerTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		MaterialColorPickerNode* pOtherNode = dynamic_cast< MaterialColorPickerNode* >( pNode );
		if( pOtherNode )
		{
			m_Color = pOtherNode->Outputs[ 0 ].As<MaterialViewerColorPin>()->Data;
		}
	}
#endif

	void SMaterialGraphColorPickerTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		SMaterialGraphColorPickerTask* pThisOther = dynamic_cast< SMaterialGraphColorPickerTask* >( pOther );
		if( pThisOther )
		{
			m_Color = pThisOther->m_Color;
		
			pHandler->RegisterLocator( m_NodeID, 0, &m_Color );
		}
	}

	NodeEditorTaskState SMaterialGraphColorPickerTask::Tick( Timestep ts )
	{
		return NodeEditorTaskState::Completed;
	}

	void SMaterialGraphColorPickerTask::Reset()
	{
	}

	void SMaterialGraphColorPickerTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_Color, rStream );
	}

	void SMaterialGraphColorPickerTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_Color, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// SMaterialGraphOutputNodeTask

	SMaterialGraphOutputNodeTask::SMaterialGraphOutputNodeTask()
	{
	}

	SMaterialGraphOutputNodeTask::~SMaterialGraphOutputNodeTask()
	{
	}

#if !defined(SAT_DIST)
	void SMaterialGraphOutputNodeTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		// Find out if albedo is a color node.
		auto link = pEditor->FindLinkByPin( pNode->Inputs[ 0 ]->ID );
		if( link )
		{
			auto node = pEditor->FindNodeByPin( link->StartPinID );

			// TEMP:
			m_AlbedoID = node->ID;
			m_AlbedoIsColor = true;
		}
	}
#endif

	void SMaterialGraphOutputNodeTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		SMaterialGraphOutputNodeTask* pThisOther = dynamic_cast< SMaterialGraphOutputNodeTask* >( pOther );
		if( pThisOther )
		{
			m_AlbedoID = pThisOther->m_AlbedoID;
			m_AlbedoIsColor = pThisOther->m_AlbedoIsColor;
		}
	}

	NodeEditorTaskState SMaterialGraphOutputNodeTask::Tick( Timestep ts )
	{
		if( m_CurrentState == NodeEditorTaskState::Unknown )
		{
			MaterialGraphTaskHandler* pMaterialHandler = dynamic_cast< MaterialGraphTaskHandler* >( m_pHandler );
			if( pMaterialHandler )
			{
				if( m_AlbedoIsColor )
				{
					glm::vec3* pCol = m_pHandler->AccessLocator<glm::vec3>( m_AlbedoID, 0 );
				
					pMaterialHandler->GetMaterial()->SetPC<glm::vec3>( "u_Materials.AlbedoColor", *pCol );
				}
			}

			m_CurrentState = NodeEditorTaskState::Completed;
		}

		return NodeEditorTaskState::Completed;
	}

	void SMaterialGraphOutputNodeTask::Reset()
	{
	}

	void SMaterialGraphOutputNodeTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_AlbedoIsColor, rStream );
		RawSerialisation::WriteObjectChecked( m_AlbedoID, rStream );
		RawSerialisation::WriteObjectChecked( m_NormalID, rStream );
		RawSerialisation::WriteObjectChecked( m_MetallicID, rStream );
		RawSerialisation::WriteObjectChecked( m_RoughnessID, rStream );
		RawSerialisation::WriteObjectChecked( m_EmissionID, rStream );
	}

	void SMaterialGraphOutputNodeTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_AlbedoIsColor, rStream );
		RawSerialisation::ReadObjectChecked( m_AlbedoID, rStream );
		RawSerialisation::ReadObjectChecked( m_NormalID, rStream );
		RawSerialisation::ReadObjectChecked( m_MetallicID, rStream );
		RawSerialisation::ReadObjectChecked( m_RoughnessID, rStream );
		RawSerialisation::ReadObjectChecked( m_EmissionID, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SMaterialGraphColorPickerTask );
SAT_X31_CREATE_AUTO_REG( SMaterialGraphOutputNodeTask );
