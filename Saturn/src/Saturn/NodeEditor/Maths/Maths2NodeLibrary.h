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
	
	class SMaths2AndBoolNode;
	class SMaths2OrBoolNode;

	class Maths2BoolNodeLibrary
	{
	public:
		static SharedPtr<SMaths2LessThanFloatNode>      SpawnMathsLTFlts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2LessThanIntNode>        SpawnMathsLTInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2LessThanUIntNode>       SpawnMathsLTUInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2LessThanOrEquFloatNode> SpawnMathsLTOrEquFlts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2LessThanOrEquIntNode>   SpawnMathsLTOrEquInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2LessThanOrEquUIntNode>  SpawnMathsLTOrEquUInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2GreaterThanFloatNode>   SpawnMathsGTFlts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2GreaterThanIntNode>     SpawnMathsGTInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2GreaterThanUIntNode>    SpawnMathsGTUInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2GreaterThanOrEquFloatNode> SpawnMathsGTOrEquFlts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2GreaterThanOrEquIntNode>   SpawnMathsGTOrEquInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2GreaterThanOrEquUIntNode>  SpawnMathsGTOrEquUInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2EquToBoolNode>   SpawnMathsEquToBool( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2EquToFloatNode>  SpawnMathsEquToFlt( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2EquToIntNode>    SpawnMathsEquToInt( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2EquToUIntNode>   SpawnMathsEquToUInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2NotEqualToBoolNode>   SpawnMathsNotEquToBool( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2NotEqualToFloatNode>  SpawnMathsNotEquToFlt( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2NotEqualToIntNode>    SpawnMathsNotEquToInt( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2NotEqualToUIntNode>   SpawnMathsNotEquToUInts( SharedPtr<NodeEditor> nodeEditor );

		static SharedPtr<SMaths2AddFloatNode> SpawnMathsAddFlts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2AddIntNode>   SpawnMathsAddInts( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2AddUIntNode>  SpawnMathsAddUInts( SharedPtr<NodeEditor> nodeEditor );
		
		static SharedPtr<SMaths2AndBoolNode>  SpawnMathsAndBool( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<SMaths2OrBoolNode>  SpawnMathsOrBool( SharedPtr<NodeEditor> nodeEditor );

		static SharedPtr<NodeEditorNodeBase> DrawImGuiSelectionMenu( SharedPtr<NodeEditor> nodeEditor );
	};
}
