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

#pragma once

#include "Saturn/NodeEditor/NodeEditorTaskBase.h"

namespace Saturn {

	template<typename Ty>
	class Maths2LessThanTask : public NodeEditorTaskBase
	{
	public:
		Maths2LessThanTask() = default;
		virtual ~Maths2LessThanTask() = default;

	public:
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	private:
		UUID m_PinA = 0, m_PinB = 0;

		Ty* m_pA = nullptr;
		Ty* m_pB = nullptr;
	};

#if !defined(SAT_DIST)
	template<typename Ty>
	void Maths2LessThanTask<Ty>::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		if( !pNode || !pNode->Inputs.size() )
			return;

		m_NodeFlags = ( NodeEditorNodeFlags ) pNode->Flags;
		m_NodeID = pNode->ID;

		// NB: FindLinkByPin is OK here, Pin does not have PinFlag_AcceptMultipleLinks flag.
		auto link = pEditor->FindLinkByPin( pNode->Inputs[ 0 ]->ID );
		if( link && link->StartPinID )
		{
			auto otherPin = pEditor->FindPin( link->StartPinID );
			if( otherPin )
			{
				m_PinA = otherPin->Node->ID;
			}
		}

		link = pEditor->FindLinkByPin( pNode->Inputs[ 1 ]->ID );
		if( link && link->StartPinID )
		{
			auto otherPin = pEditor->FindPin( link->StartPinID );
			if( otherPin )
			{
				m_PinB = otherPin->Node->ID;
			}
		}
	}
#endif

	template<typename Ty>
	void Maths2LessThanTask<Ty>::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		NodeEditorTaskBase::InitialiseTaskWithOther( pHandler, pOther );

		Maths2LessThanTask<Ty>* pLessThanOther = dynamic_cast< Maths2LessThanTask<Ty>* >( pOther );
		if( pLessThanOther )
		{
			m_PinA = pLessThanOther->m_PinA;
			m_PinB = pLessThanOther->m_PinB;

			m_pA = pHandler->template AccessLocator<Ty>( m_PinA, 0 );
			m_pB = pHandler->template AccessLocator<Ty>( m_PinB, 1 );
		}
	}

	template<typename Ty>
	NodeEditorTaskState Maths2LessThanTask<Ty>::Tick( Timestep ts )
	{
		if( !m_pA || !m_pB )
		{
			m_CurrentState = NodeEditorTaskState::Failed;
			return;
		}

		if( *m_pA < *m_pB )
			m_CurrentState = NodeEditorTaskState::Completed;

		return m_CurrentState;
	}

	template<typename Ty>
	void Maths2LessThanTask<Ty>::Reset()
	{
	}

}
