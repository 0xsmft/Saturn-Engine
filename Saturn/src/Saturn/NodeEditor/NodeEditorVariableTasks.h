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

#include "NodeEditorTaskBase.h"
#include "NodeEditorVariableDataType.h"

namespace Saturn {

	class NodeEditorVariable;
	class NodeEditorTaskHandler;

	//
	// SNodeEditorGetVariableTask
	//
	// Register a locator with an address to the variable data.
	//
	SCLASS()
	class SNodeEditorGetVariableTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SNodeEditorGetVariableTask, NodeEditorTaskBase );
	public:
		SNodeEditorGetVariableTask();
		virtual ~SNodeEditorGetVariableTask();
		
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		void RefreshLocators();

	private:
		UUID m_VariableID = 0llu;
	};

	//
	// SNodeEditorSetVariableTask
	//
	// Use a pre-registered locator and set the value to what was specified "In New Value" pin.
	//
	SCLASS()
	class SNodeEditorSetVariableTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SNodeEditorSetVariableTask, NodeEditorTaskBase );
	public:
		SNodeEditorSetVariableTask();
		virtual ~SNodeEditorSetVariableTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		UUID m_VariableID = 0llu;
		// If new value pin is linked this will be the output pin id from the other node.
		UUID m_InNewValueOtherPinID = 0llu;

		NodeEditorVarTypeSafeUnion m_DefaultValueUnion{};
		
		bool m_IsNewValueLinked = false;
		NodeEditorVariableDataType m_VariableType = NodeEditorVariableDataType::Unknown;
	};

}
