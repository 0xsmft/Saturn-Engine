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
#include "NodeEditorTaskBase.h"

#include "NodeEditorBase.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

#if !defined(SAT_DIST)
	void NodeEditorTaskBase::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		if( pNode )
		{
			m_NodeFlags = ( NodeEditorNodeFlags ) pNode->Flags;
			m_NodeID = pNode->ID;
			
			if( m_DebugName.empty() )
				m_DebugName = pNode->Name;
		}
	}
#endif

	void NodeEditorTaskBase::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		m_pHandler = pHandler;
		m_NodeID = pOther->m_NodeID;
		m_NodeFlags = pOther->m_NodeFlags;
#if !defined(SAT_DIST)
		m_DebugName = pOther->m_DebugName;
#endif
	}

	void NodeEditorTaskBase::Serialise( std::ofstream& rStream ) const
	{
#if !defined(SAT_DIST)
		RawSerialisation::WriteString( m_DebugName, rStream );
#endif

		RawSerialisation::WriteObject( m_NodeID, rStream );
		RawSerialisation::WriteObject( m_NodeFlags, rStream );
	}

	void NodeEditorTaskBase::Deserialise( FDependentIStream& rStream )
	{
#if !defined(SAT_DIST)
		m_DebugName = RawSerialisation::ReadString( rStream );
#else
		// TODO: Remove this on dist.
		auto _ = RawSerialisation::ReadString( rStream );
#endif

		RawSerialisation::ReadObject( m_NodeID, rStream );
		RawSerialisation::ReadObject( m_NodeFlags, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( NodeEditorTaskBase );
