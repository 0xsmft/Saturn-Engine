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

#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	// Standard set of errors
	enum NodeEditorPreCompileStdErrors : uint32_t
	{
		NodeEdPreCompError_InternalError	   = 0,
		NodeEdPreCompError_MissingRequiredLink = 1,
		NodeEdPreCompError_MissingRequiredData = 2,
	};

	// Standard errors category i.e. what node editor.
	enum NodeEditorPreCompileCategory : uint32_t
	{
		NodeEdPreCompCategory_Standard		   = BIT( 0 ),
		NodeEdPreCompCategory_MaterialGraph    = BIT( 1 ),
		NodeEdPreCompCategory_SoundGraph	   = BIT( 2 ),
		NodeEdPreCompCategory_AnimationGraph   = BIT( 3 ),
		NodeEdPreCompCategory_Sandbox		   = BIT( 4 ),
		NodeEdPreCompCategory_BehaviourTree	   = BIT( 5 ),
	};

	struct NodeEditorPreCompileError
	{
		NodeEditorPreCompileCategory Category = NodeEdPreCompCategory_Standard;
		uint32_t ErrorCode = 0u;
	};

	//
	// NodeEditorPreCompilerBase
	// 
	// The pre-compiler runs before simulation. It is the first stage in compilation stage.
	// 
	// The pre-compile must have zero errors in other for the TaskCache to be built.
	// 
	// NB: In some NodeEditors (namely Materials) the pre-compilation IS the compilation stage itself.
	//	   In such a case no task cache will be built nor will there be any simulation stage.
	//
	class NodeEditorPreCompilerBase : public RefTarget
	{
	public:
		NodeEditorPreCompilerBase() = default;
		NodeEditorPreCompilerBase( SharedPtr<NodeEditorBase> nodeEditor ) 
			: m_NodeEditor( nodeEditor )
		{
		}

		virtual ~NodeEditorPreCompilerBase() = default;

		virtual void Init( const std::vector<SharedPtr<NodeEditorNodeBase>>& rOrder )
		{
			m_Order = rOrder;
		}

		virtual std::vector<NodeEditorPreCompileError> PreCompile() = 0;

	protected:
		SharedPtr<NodeEditorBase> m_NodeEditor;
		std::vector<SharedPtr<NodeEditorNodeBase>> m_Order;
	};
	
}
