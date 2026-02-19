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

	//
	// Runtime operation of two floats added together.
	//
	SCLASS()
	class SMathsAddFloatsTask :	public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMathsAddFloatsTask, NodeEditorTaskBase );
	public:
		SMathsAddFloatsTask();
		virtual ~SMathsAddFloatsTask();

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode ) override;
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	public:
		float GetResult() const { return m_Result; }

	private:
		float* m_A = nullptr;
		float* m_B = nullptr;
		float m_Result = 0.0f;
		bool m_OperationCompleted = false;
	};

	//////////////////////////////////////////////////////////////////////////
	class BoolPin;
	class FloatPin;

	//
	// Runtime operation of a logical greater than (A > B).
	//
	SCLASS()
	class SMathsGreaterThanFloatsTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMathsGreaterThanFloatsTask, NodeEditorTaskBase );
	public:
		SMathsGreaterThanFloatsTask();
		virtual ~SMathsGreaterThanFloatsTask();

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode ) override;
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	public:
		[[nodiscard]] bool Succeeded() const { return m_Result; }

	private:
		// TODO: #FixTasksOutgoings
		NodeEditorTaskHandler* m_pHandler = nullptr;
		std::vector<UUID> Outgoings;

		float* m_ValueToTest = nullptr;
		float* m_Threshold = nullptr;
		bool m_Result = false;
	};

	//////////////////////////////////////////////////////////////////////////

	//
	// Runtime operation of a logical less than (A < B).
	//
	SCLASS()
	class SMathsLessThanFloatsTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMathsLessThanFloatsTask, NodeEditorTaskBase );
	public:
		SMathsLessThanFloatsTask();
		virtual ~SMathsLessThanFloatsTask();

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode ) override;
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	public:
		[[nodiscard]] bool Succeeded() const { return m_Result; }

	private:
		// TODO: #FixTasksOutgoings
		NodeEditorTaskHandler* m_pHandler = nullptr;
		std::vector<UUID> Outgoings;

		float* m_ValueToTest = nullptr;
		float* m_Threshold = nullptr;
		bool m_Result = false;
	};

	//////////////////////////////////////////////////////////////////////////

	//
	// Runtime operation of a NOT gate.
	//
	SCLASS()
	class SMathsNotTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMathsNotTask, NodeEditorTaskBase );
	public:
		SMathsNotTask();
		virtual ~SMathsNotTask();

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode ) override;
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	private:
		// TODO: #FixTasksOutgoings
		NodeEditorTaskHandler* m_pHandler = nullptr;
		std::vector<UUID> Outgoings;

		bool* m_pValueToTest = nullptr;
		bool m_Result = false;
	};

	//
	// Runtime operation of a logical OR gate.
	//
	SCLASS()
	class SMathsOrTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMathsOrTask, NodeEditorTaskBase );
	public:
		SMathsOrTask();
		virtual ~SMathsOrTask();

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pBase, NodeEditorNodeBase* pNode ) override;
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	private:
		// TODO: #FixTasksOutgoings
		NodeEditorTaskHandler* m_pHandler = nullptr;
		std::vector<UUID> Outgoings;

		bool* m_pA = nullptr;
		bool* m_pB = nullptr;
		bool m_Result = false;
	};
}
