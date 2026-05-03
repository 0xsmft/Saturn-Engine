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

	class SMaths2LessThanFloatNode;
	class SMaths2LessThanIntNode;
	class SMaths2LessThanUIntNode;

	class SMaths2LessThanOrEquFloatNode;
	class SMaths2LessThanOrEquIntNode;
	class SMaths2LessThanOrEquUIntNode;
	
	class SMaths2GreaterThanFloatNode;
	class SMaths2GreaterThanIntNode;
	class SMaths2GreaterThanUIntNode;

	class SMaths2GreaterThanOrEquFloatNode;
	class SMaths2GreaterThanOrEquIntNode;
	class SMaths2GreaterThanOrEquUIntNode;

	class SMaths2EquToBoolNode;
	class SMaths2EquToFloatNode;
	class SMaths2EquToIntNode;
	class SMaths2EquToUIntNode;

	class SMaths2NotEqualToBoolNode;
	class SMaths2NotEqualToFloatNode;
	class SMaths2NotEqualToIntNode;
	class SMaths2NotEqualToUIntNode;

	class SMaths2AddFloatNode;
	class SMaths2AddIntNode;
	class SMaths2AddUIntNode;

	class Maths2BoolNodeLibrary
	{
	public:
		static SharedPtr<SMaths2LessThanFloatNode>      SpawnMathsLTFlts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2LessThanIntNode>        SpawnMathsLTInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2LessThanUIntNode>       SpawnMathsLTUInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2LessThanOrEquFloatNode> SpawnMathsLTOrEquFlts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2LessThanOrEquIntNode>   SpawnMathsLTOrEquInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2LessThanOrEquUIntNode>  SpawnMathsLTOrEquUInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2GreaterThanFloatNode>   SpawnMathsGTFlts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2GreaterThanIntNode>     SpawnMathsGTInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2GreaterThanUIntNode>    SpawnMathsGTUInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2GreaterThanOrEquFloatNode> SpawnMathsGTOrEquFlts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2GreaterThanOrEquIntNode>   SpawnMathsGTOrEquInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2GreaterThanOrEquUIntNode>  SpawnMathsGTOrEquUInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2EquToBoolNode>   SpawnMathsEquToBool( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2EquToFloatNode>  SpawnMathsEquToFlt( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2EquToIntNode>    SpawnMathsEquToInt( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2EquToUIntNode>   SpawnMathsEquToUInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2NotEqualToBoolNode>   SpawnMathsNotEquToBool( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2NotEqualToFloatNode>  SpawnMathsNotEquToFlt( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2NotEqualToIntNode>    SpawnMathsNotEquToInt( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2NotEqualToUIntNode>   SpawnMathsNotEquToUInts( SharedPtr<NodeEditorBase> nodeEditor );

		static SharedPtr<SMaths2AddFloatNode> SpawnMathsAddFlts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2AddIntNode>   SpawnMathsAddInts( SharedPtr<NodeEditorBase> nodeEditor );
		static SharedPtr<SMaths2AddUIntNode>  SpawnMathsAddUInts( SharedPtr<NodeEditorBase> nodeEditor );

		static SharedPtr<NodeEditorNodeBase> DrawImGuiSelectionMenu( SharedPtr<NodeEditorBase> nodeEditor );
	};
}
