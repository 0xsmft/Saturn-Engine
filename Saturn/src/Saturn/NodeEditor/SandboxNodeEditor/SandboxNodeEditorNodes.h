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

#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

namespace Saturn {

	//
	// SandboxNodeEditorOutputNode
	//
	SCLASS()
	class SandboxNodeEditorOutputNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( SandboxNodeEditorOutputNode, NodeEditorBlueprintNode );
	public:
		SandboxNodeEditorOutputNode();
		virtual ~SandboxNodeEditorOutputNode() = default;

		virtual NodeEditorTaskBase* ConvertToTask();

	private:
		void CreateNode();
	};

	//
	// SandboxNodeEditorNode
	// 
	// Example node implementation (consteval)
	//
	// Stores a number...
	//
	SCLASS()
	class SandboxNodeEditorNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( SandboxNodeEditorNode, NodeEditorBlueprintNode );
	public:
		SandboxNodeEditorNode();
		virtual ~SandboxNodeEditorNode() = default;

		uint64_t GetSpecialValue() const { return m_SpecialSandboxValue; }

		virtual NodeEditorTaskBase* ConvertToTask();

	protected:
		uint64_t m_SpecialSandboxValue = 0xC0DEBABE;

	private:
		void CreateNode();
	};
	
	//
	// SandboxNodeExampleNode
	//
	// Example implementation.
	//
	SCLASS()
	class SandboxNodeExampleNode : public SandboxNodeEditorNode
	{
		SAT_DECLARE_CLASS( SandboxNodeExampleNode, SandboxNodeEditorNode );
	public:
		SandboxNodeExampleNode();
		virtual ~SandboxNodeExampleNode() = default;

		uint64_t GetAnotherNumber() const { return m_AnotherNumber; }

		virtual NodeEditorTaskBase* ConvertToTask();

	protected:
		uint64_t m_AnotherNumber = 0xDEADBEEF;

	private:
		void CreateNode();
	};
}
