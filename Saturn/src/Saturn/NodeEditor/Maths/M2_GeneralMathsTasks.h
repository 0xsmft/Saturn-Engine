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
	class Maths2AddTTask : public NodeEditorTaskBase
	{
	public:
		Maths2AddTTask() = default;
		virtual ~Maths2AddTTask() = default;

	public:
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override
		{
			if( !pNode || !pNode->Inputs.size() )
				return;

			// Save node flags
			m_NodeFlags = ( NodeEditorNodeFlags ) pNode->Flags;
			m_NodeID = pNode->ID;

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

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override
		{
			// Init super.
			NodeEditorTaskBase::InitialiseTaskWithOther( pHandler, pOther );

			Maths2AddTTask<Ty>* pThisOther = dynamic_cast< Maths2AddTTask<Ty>* >( pOther );
			if( pThisOther )
			{
				m_PinA = pThisOther->m_PinA;
				m_PinB = pThisOther->m_PinB;

				m_pA = pHandler->template AccessLocator<Ty>( m_PinA, 0 );
				m_pB = pHandler->template AccessLocator<Ty>( m_PinB, 0 );

				// Register output as a locator.
				pHandler->RegisterLocator<Ty>( m_NodeID, 0, &m_OutValue );
			}
		}

		virtual NodeEditorTaskState Tick( Timestep ts ) override
		{
			if( !m_pA || !m_pB )
				return NodeEditorTaskState::Failed;

			return NodeEditorTaskState::Running;
		}

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	protected:
		UUID m_PinA = 0, m_PinB = 0;

		Ty* m_pA = nullptr;
		Ty* m_pB = nullptr;

		Ty m_OutValue{};
	};

	//////////////////////////////////////////////////////////////////////////
	// Add

	SCLASS()
	class Maths2AddFloatTask : public Maths2AddTTask<float>
	{
		SAT_DECLARE_CLASS( Maths2AddFloatTask, Maths2AddTTask<float> );
	public:
		Maths2AddFloatTask() = default;
		virtual ~Maths2AddFloatTask() = default;

	public:
		virtual NodeEditorTaskState Tick( Timestep ts ) override
		{
			if( m_CurrentState = Super::Tick( ts ); m_CurrentState != NodeEditorTaskState::Failed )
			{
				m_OutValue = ( *m_pA ) + ( *m_pB );
				m_CurrentState = NodeEditorTaskState::Completed;
			}

			return m_CurrentState;
		}
	};

	SCLASS()
	class Maths2AddIntTask : public Maths2AddTTask<int>
	{
		SAT_DECLARE_CLASS( Maths2AddIntTask, Maths2AddTTask<int> );
	public:
		Maths2AddIntTask() = default;
		virtual ~Maths2AddIntTask() = default;

	public:
		virtual NodeEditorTaskState Tick( Timestep ts ) override
		{
			if( m_CurrentState = Super::Tick( ts ); m_CurrentState != NodeEditorTaskState::Failed )
			{
				m_OutValue = ( *m_pA ) + ( *m_pB );
				m_CurrentState = NodeEditorTaskState::Completed;
			}

			return m_CurrentState;
		}
	};

	SCLASS()
	class Maths2AddUIntTask : public Maths2AddTTask<uint32_t>
	{
		SAT_DECLARE_CLASS( Maths2AddUIntTask, Maths2AddTTask<uint32_t> );
	public:
		Maths2AddUIntTask() = default;
		virtual ~Maths2AddUIntTask() = default;

	public:
		virtual NodeEditorTaskState Tick( Timestep ts ) override
		{
			if( m_CurrentState = Super::Tick( ts ); m_CurrentState != NodeEditorTaskState::Failed )
			{
				m_OutValue = ( *m_pA ) + ( *m_pB );
				m_CurrentState = NodeEditorTaskState::Completed;
			}

			return m_CurrentState;
		}
	};

}
