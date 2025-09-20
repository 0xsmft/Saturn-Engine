/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

namespace Saturn {

	SCLASS()
	class MathsAddFloats : public NodeEditorBlueprintNode
	{
	public:
		MathsAddFloats();
		MathsAddFloats( const std::string& rName );

		virtual ~MathsAddFloats();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsSubFloats : public NodeEditorBlueprintNode
	{
	public:
		MathsSubFloats();
		MathsSubFloats( const std::string& rName );

		virtual ~MathsSubFloats();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsMulFloats : public NodeEditorBlueprintNode
	{
	public:
		MathsMulFloats();
		MathsMulFloats( const std::string& rName );

		virtual ~MathsMulFloats();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsDivideFloats : public NodeEditorBlueprintNode
	{
	public:
		MathsDivideFloats();
		MathsDivideFloats( const std::string& rName );

		virtual ~MathsDivideFloats();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};
}
