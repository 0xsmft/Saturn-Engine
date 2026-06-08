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
	class Maths2GeneralBooleanTask : public NodeEditorTaskBase
	{
	public:
		Maths2GeneralBooleanTask() = default;
		virtual ~Maths2GeneralBooleanTask() = default;

	public:
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override
		{
			if( !pNode || !pNode->Inputs.size() )
				return;

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
			// Not linked? Read data from the Pin
			else
			{
				m_DefaultValueA = pNode->Inputs[ 0 ].As<typename PinTypeTraits<Ty>::PinType>()->Data;
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
			// Not linked? Read data from the Pin
			else
			{
				m_DefaultValueB = pNode->Inputs[ 1 ].As<typename PinTypeTraits<Ty>::PinType>()->Data;
			}
		}
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override
		{
			NodeEditorTaskBase::InitialiseTaskWithOther( pHandler, pOther );

			Maths2GeneralBooleanTask<Ty>* pThisOther = dynamic_cast< Maths2GeneralBooleanTask<Ty>* >( pOther );
			if( pThisOther )
			{
				m_PinA = pThisOther->m_PinA;
				m_PinB = pThisOther->m_PinB;

				m_DefaultValueA = pThisOther->m_DefaultValueA;
				m_DefaultValueB = pThisOther->m_DefaultValueB;

				FetchLocators();

				pHandler->RegisterLocator( m_NodeID, 0, &m_OutValue );
			}
		}

		virtual NodeEditorTaskState Tick( Timestep ts ) override
		{
			if( ( m_pA == nullptr || m_pB == nullptr ) && !m_AttemptedFetch )
			{
				FetchLocators();
				m_AttemptedFetch = true;
			}
			else if( m_AttemptedFetch && ( m_pA == nullptr || m_pB == nullptr ) )
			{
				return NodeEditorTaskState::Failed;
			}

			return NodeEditorTaskState::Running;
		}

		virtual void Reset() override {}

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		void FetchLocators() 
		{
			// No locator pointer yet?
			if( !m_pA )
			{
				// So, if m_PinA is valid that means that pin A is linked,
				// so in that case we can try get the locator.
				if( m_PinA )
				{
					m_pA = m_pHandler->template AccessLocator<Ty>( m_PinA, 0 );
				}
				// However, if it's not linked, then we can just use the value that was typed into the node itself.
				else
				{
					// TODO: Const... should not be able to modify a default value.
					m_pA = &m_DefaultValueA;
				}
			}

			// No locator pointer yet?
			if( !m_pB )
			{
				// So, if m_PinB is valid that means that pin B is linked,
				// so in that case we can try get the locator.
				if( m_PinB )
				{
					m_pB = m_pHandler->template AccessLocator<Ty>( m_PinB, 0 );
				}
				// However, if it's not linked, then we can just use the value that was typed into the node itself.
				else
				{
					// TODO: Const... should not be able to modify a default value.
					m_pB = &m_DefaultValueB;
				}
			}
		}

	protected:
		// Link IDs
		UUID m_PinA = 0, m_PinB = 0;
		
		// Default values.
		Ty m_DefaultValueA{}, m_DefaultValueB{};

		// Pointer to locators
		Ty* m_pA = nullptr;
		Ty* m_pB = nullptr;
		
		bool m_OutValue = false;
		bool m_AttemptedFetch = false;
	};

#define SAT_DECLARE_MATHS2_BOOLEAN_TASK( ClassName, BoolOperation, CppType ) \
SCLASS()																	 \
class ClassName##Task : public Maths2GeneralBooleanTask<CppType>			 \
{																			 \
	SAT_DECLARE_CLASS( ClassName##Task, Maths2GeneralBooleanTask<CppType> ); \
public:																		 \
	ClassName##Task() = default;											 \
	virtual ~ClassName##Task() = default;									 \
																			 \
public:																		 \
	virtual NodeEditorTaskState Tick( Timestep ts ) override				 \
	{																		 \
		if( m_CurrentState = Super::Tick( ts ); m_CurrentState != NodeEditorTaskState::Failed ) \
		{																						\
			m_OutValue = ( ( *m_pA ) BoolOperation ( *m_pB ) );									\
																								\
			m_CurrentState = NodeEditorTaskState::Completed;									\
		}																						\
																								\
		return m_CurrentState;																	\
	}																							\
}

	//////////////////////////////////////////////////////////////////////////
	// Less than

#define SAT_DECLARE_MATHS2_TASK_LESS_THAN( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2LessThan##FriendlyName, <, CppType )

	// ~ Maths2LessThan
	SAT_DECLARE_MATHS2_TASK_LESS_THAN( Float, float );
	SAT_DECLARE_MATHS2_TASK_LESS_THAN( UInt,  uint32_t );
	SAT_DECLARE_MATHS2_TASK_LESS_THAN( Int,   int );

	//////////////////////////////////////////////////////////////////////////
	// Less than or equ

#define SAT_DECLARE_MATHS2_TASK_LESS_THAN_OR_EQU( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2LessThanOrEqu##FriendlyName, <=, CppType )

	// ~ Maths2LessThanOrEqu
	SAT_DECLARE_MATHS2_TASK_LESS_THAN_OR_EQU( Float, float );
	SAT_DECLARE_MATHS2_TASK_LESS_THAN_OR_EQU( UInt, uint32_t );
	SAT_DECLARE_MATHS2_TASK_LESS_THAN_OR_EQU( Int, int );

	//////////////////////////////////////////////////////////////////////////
	// Greater than

#define SAT_DECLARE_MATHS2_TASK_GREATER_THAN( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2GreaterThan##FriendlyName, >, CppType )

	// ~ Maths2GreaterThan
	SAT_DECLARE_MATHS2_TASK_GREATER_THAN( Float, float );
	SAT_DECLARE_MATHS2_TASK_GREATER_THAN( UInt, uint32_t );
	SAT_DECLARE_MATHS2_TASK_GREATER_THAN( Int, int );

	//////////////////////////////////////////////////////////////////////////
	// Greater than or equ

#define SAT_DECLARE_MATHS2_TASK_GREATER_THAN_OR_EQU( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2GreaterThanOrEqu##FriendlyName, >=, CppType )

	// ~ Maths2GreaterThanOrEqu
	SAT_DECLARE_MATHS2_TASK_GREATER_THAN_OR_EQU( Float, float );
	SAT_DECLARE_MATHS2_TASK_GREATER_THAN_OR_EQU( UInt, uint32_t );
	SAT_DECLARE_MATHS2_TASK_GREATER_THAN_OR_EQU( Int, int );

	//////////////////////////////////////////////////////////////////////////
	// Equal to

#define SAT_DECLARE_MATHS2_TASK_EQUAL_TO( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2EquTo##FriendlyName, ==, CppType )

	// ~ Maths2EquTo
	SAT_DECLARE_MATHS2_TASK_EQUAL_TO( Bool, bool );
	SAT_DECLARE_MATHS2_TASK_EQUAL_TO( Float, float );
	SAT_DECLARE_MATHS2_TASK_EQUAL_TO( UInt, uint32_t );
	SAT_DECLARE_MATHS2_TASK_EQUAL_TO( Int, int );

	//////////////////////////////////////////////////////////////////////////
	// NOT Equal to

#define SAT_DECLARE_MATHS2_TASK_NOT_EQUAL_TO( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2NotEqualTo##FriendlyName, >, CppType )

	// ~ Maths2NotEqualTo
	SAT_DECLARE_MATHS2_TASK_NOT_EQUAL_TO( Bool, bool );
	SAT_DECLARE_MATHS2_TASK_NOT_EQUAL_TO( Float, float );
	SAT_DECLARE_MATHS2_TASK_NOT_EQUAL_TO( UInt, uint32_t );
	SAT_DECLARE_MATHS2_TASK_NOT_EQUAL_TO( Int, int );

	//////////////////////////////////////////////////////////////////////////
	// AND

#define SAT_DECLARE_MATHS2_TASK_AND( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2And##FriendlyName, &&, CppType )

	// ~ Maths2And
	SAT_DECLARE_MATHS2_TASK_AND( Bool, bool );

	//////////////////////////////////////////////////////////////////////////
	// OR

#define SAT_DECLARE_MATHS2_TASK_OR( FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOLEAN_TASK( Maths2Or##FriendlyName, ||, CppType )

	// ~ Maths2Or
	SAT_DECLARE_MATHS2_TASK_OR( Bool, bool );

}
