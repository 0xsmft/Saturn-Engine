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

#include "sppch.h"
#include "Maths2NodeLibrary.h"

#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Maths2AllTasks.h"
#include "Maths2AllNodes.h"

namespace Saturn {

	template<typename Ty>
	struct MathsNodeContextMenuCaller;

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION( ClassType, ClassName, FunctionName )			\
SharedPtr<ClassType> ClassName::FunctionName( SharedPtr<NodeEditor> nodeEditor )	\
{																						\
	SharedPtr<ClassType> node = NewObject<ClassType>( nodeEditor.Get() );				\
	nodeEditor->AddNode( node );														\
																						\
	return node;																		\
}																						\
template<> struct MathsNodeContextMenuCaller<ClassType>									\
{																						\
	static SharedPtr<ClassType> DoCall( SharedPtr<NodeEditor> nodeEditor )			\
	{																					\
		SharedPtr<ClassType> node;														\
		if( ImGui::MenuItem( ClassType::M2_GetNodeName() ) )							\
		{																				\
			node = ClassName::FunctionName( nodeEditor );								\
		}																				\
																						\
		return node;																	\
	}																					\
};


#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_LESS_THAN( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2LessThanFloatNode, ClassName, SpawnMathsLTFlts ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2LessThanIntNode, ClassName, SpawnMathsLTInts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2LessThanUIntNode, ClassName, SpawnMathsLTUInts ) \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_LESS_OR_EQU_TO( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2LessThanOrEquFloatNode, ClassName, SpawnMathsLTOrEquFlts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2LessThanOrEquIntNode,   ClassName, SpawnMathsLTOrEquInts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2LessThanOrEquUIntNode,  ClassName, SpawnMathsLTOrEquUInts )  \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_GREATER_THAN( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2GreaterThanFloatNode, ClassName, SpawnMathsGTFlts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2GreaterThanIntNode,   ClassName, SpawnMathsGTInts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2GreaterThanUIntNode,  ClassName, SpawnMathsGTUInts )  \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_GREATER_OR_EQU_TO( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2GreaterThanOrEquFloatNode, ClassName, SpawnMathsGTOrEquFlts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2GreaterThanOrEquIntNode,   ClassName, SpawnMathsGTOrEquInts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2GreaterThanOrEquUIntNode,  ClassName, SpawnMathsGTOrEquUInts )  \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_EQU_TO( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2EquToBoolNode, ClassName, SpawnMathsEquToBool )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2EquToFloatNode, ClassName, SpawnMathsEquToFlt )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2EquToIntNode,   ClassName, SpawnMathsEquToInt )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2EquToUIntNode,  ClassName, SpawnMathsEquToUInts )  \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_NOT_EQU_TO( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2NotEqualToBoolNode, ClassName, SpawnMathsNotEquToBool )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2NotEqualToFloatNode, ClassName, SpawnMathsNotEquToFlt )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2NotEqualToIntNode,   ClassName, SpawnMathsNotEquToInt )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2NotEqualToUIntNode,  ClassName, SpawnMathsNotEquToUInts )  \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_ADD( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2AddFloatNode, ClassName, SpawnMathsAddFlts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2AddIntNode,   ClassName, SpawnMathsAddInts )   \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2AddUIntNode,  ClassName, SpawnMathsAddUInts )  \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_AND( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2AndBoolNode, ClassName, SpawnMathsAndBool )   \

#define SAT_DECLARE_SPAWN_MATHS_FUNCTION_OR( ClassName ) \
SAT_DECLARE_SPAWN_MATHS_FUNCTION( SMaths2OrBoolNode, ClassName, SpawnMathsOrBool )   \

	SAT_DECLARE_SPAWN_MATHS_FUNCTION_LESS_THAN( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_LESS_OR_EQU_TO( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_GREATER_THAN( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_GREATER_OR_EQU_TO( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_EQU_TO( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_NOT_EQU_TO( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_ADD( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_AND( Maths2BoolNodeLibrary );
	SAT_DECLARE_SPAWN_MATHS_FUNCTION_OR( Maths2BoolNodeLibrary );

	//////////////////////////////////////////////////////////////////////////

	SharedPtr<NodeEditorNodeBase> Maths2BoolNodeLibrary::DrawImGuiSelectionMenu( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<NodeEditorNodeBase> node;

		if( node = MathsNodeContextMenuCaller<SMaths2LessThanFloatNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2LessThanIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2LessThanUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2LessThanOrEquFloatNode>::DoCall( nodeEditor ) )
			return node;
		
		if( node = MathsNodeContextMenuCaller<SMaths2LessThanOrEquIntNode>::DoCall( nodeEditor ) )
			return node;
			
		if( node = MathsNodeContextMenuCaller<SMaths2LessThanOrEquUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2GreaterThanFloatNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2GreaterThanIntNode>::DoCall( nodeEditor ) )
			return node;
		
		if( node = MathsNodeContextMenuCaller<SMaths2GreaterThanUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2GreaterThanOrEquFloatNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2GreaterThanOrEquIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2GreaterThanOrEquUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2EquToBoolNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2EquToFloatNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2EquToIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2EquToUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2NotEqualToBoolNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2NotEqualToFloatNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2NotEqualToIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2NotEqualToUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2AddFloatNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2AddIntNode>::DoCall( nodeEditor ) )
			return node;
		
		if( node = MathsNodeContextMenuCaller<SMaths2AddUIntNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2AndBoolNode>::DoCall( nodeEditor ) )
			return node;

		if( node = MathsNodeContextMenuCaller<SMaths2OrBoolNode>::DoCall( nodeEditor ) )
			return node;

		return node;
	}

}
