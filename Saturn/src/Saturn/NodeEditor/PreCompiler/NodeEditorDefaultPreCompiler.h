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

#include "NodeEditorPreCompilerBase.h"

namespace Saturn {

	//
	// NodeEditorDefaultPreCompiler
	// 
	// A NodeEditorDefaultPreCompiler is a default pre-compiler.
	// It's behaviour is to walk through all the nodes and check if they need to be linked.
	// It will also warn if a node is connected to anything.
	// 
	// Other pre-compilers can be created by simply inhering NodeEditorPreCompilerBase.
	//
	class NodeEditorDefaultPreCompiler : public NodeEditorPreCompilerBase
	{
	public:
		NodeEditorDefaultPreCompiler() = default;
		NodeEditorDefaultPreCompiler( SharedPtr<NodeEditorBase> nodeEditor ) 
			: NodeEditorPreCompilerBase( nodeEditor )
		{
		}

		virtual ~NodeEditorDefaultPreCompiler() = default;

		virtual NodeEditorPreCompileResult PreCompile() override;
	};
	
}
