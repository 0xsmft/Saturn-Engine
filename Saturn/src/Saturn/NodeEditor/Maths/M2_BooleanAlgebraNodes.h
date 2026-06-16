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
	// Maths2GeneralBoolAlgebra
	// 
	// Boolean Algebra node
	// 
	// Takes in 2 of Ty and outputs a bool.
	//
	template<typename Ty>
	class Maths2GeneralBoolAlgebraNode : public NodeEditorBlueprintNode
	{
	public:
		Maths2GeneralBoolAlgebraNode()
			: NodeEditorBlueprintNode( "Maths2GeneralBoolAlgebra" )
		{
			CreateNode();
		}

		Maths2GeneralBoolAlgebraNode( const std::string& rName )
			: NodeEditorBlueprintNode( rName )
		{
			CreateNode();
		}

		virtual ~Maths2GeneralBoolAlgebraNode() = default;

	protected:
		void CreateNode()
		{
			// Use PinTypeTraits to get the PinClass of CppType Ty
			Inputs.push_back( Ref<SAT_CLANG_TYPENAME PinTypeTraits<Ty>::PinType>::Create( "A", PinKind::Input ) );
			Inputs.push_back( Ref<SAT_CLANG_TYPENAME PinTypeTraits<Ty>::PinType>::Create( "B", PinKind::Input ) );

			Outputs.push_back( Ref<BoolPin>::Create( "Result", PinKind::Output ) );
		}
	};

	//
	// Maths2BoolOnlyAlgebraNode
	// 
	// Takes in bool and outputs a bool.
	//
	class Maths2BoolOnlyAlgebraNode : public NodeEditorBlueprintNode
	{
	public:
		Maths2BoolOnlyAlgebraNode()
			: NodeEditorBlueprintNode( "Maths2BoolOnlyAlgebraNode" )
		{
			CreateNode();
		}

		Maths2BoolOnlyAlgebraNode( const std::string& rName )
			: NodeEditorBlueprintNode( rName )
		{
			CreateNode();
		}

		virtual ~Maths2BoolOnlyAlgebraNode() = default;

	protected:
		void CreateNode()
		{
			Inputs.push_back( Ref<BoolPin>::Create( "In Value", PinKind::Input ) );
			Outputs.push_back( Ref<BoolPin>::Create( "Result", PinKind::Output ) );
		}
	};
	
/**
 * Register a boolean algebra node.
 * 
 * ClassName: The class name e.g. SMaths2LessThanFloats
 * NodeName:  The node name e.g. float < float
 * TaskNamePlusFriendlyName:  The task name + friendly name e.g. SMaths2LessThan + Float
 * CppType:   e.g. float
 * 
*/
#define SAT_DECLARE_MATHS2_NODE( ClassName, NodeName, TaskNamePlusFriendlyName, CppType ) \
SCLASS() \
class ClassName : public Maths2GeneralBoolAlgebraNode<CppType> \
{ \
	/* NB: Super class as NodeEditorBlueprintNode is intentional, we cannot use Maths2GeneralBoolAlgebraNode
	as a super class, due to it being a templated class.*/ \
	SAT_DECLARE_CLASS( ClassName, NodeEditorBlueprintNode ) \
public: \
	ClassName() \
		: Maths2GeneralBoolAlgebraNode( NodeName ) \
	{ \
	} \
	ClassName( const std::string& rName ) \
		: Maths2GeneralBoolAlgebraNode( NodeName ) \
	{ \
	} \
	\
	virtual ~ClassName() = default; \
	\
	virtual NodeEditorTaskBase* ConvertToTask() override \
	{ \
		return NewObject<TaskNamePlusFriendlyName##Task>( nullptr ); \
	}\
	\
	static const char* M2_GetNodeName() \
	{\
		return NodeName; \
	}\
}

/**
 * Register a boolean algebra node only taking in a bool.
 * Useful for the NOT node
 * 
 * ClassName: The class name e.g. SMaths2NotBool
 * NodeName:  The node name e.g. boolean NOT
 * TaskNamePlusFriendlyName:  The task name + friendly name e.g. SMaths2Not + Bool
 * CppType:   e.g. bool
 * 
*/
#define SAT_DECLARE_MATHS2_BOOL_ONLY_NODE( ClassName, NodeName, TaskNamePlusFriendlyName, CppType ) \
SCLASS() \
class ClassName : public Maths2BoolOnlyAlgebraNode \
{ \
	SAT_DECLARE_CLASS( ClassName, Maths2BoolOnlyAlgebraNode ) \
public: \
	ClassName() \
		: Maths2BoolOnlyAlgebraNode( NodeName ) \
	{ \
	} \
	ClassName( const std::string& rName ) \
		: Maths2BoolOnlyAlgebraNode( NodeName ) \
	{ \
	} \
	\
	virtual ~ClassName() = default; \
	\
	virtual NodeEditorTaskBase* ConvertToTask() override \
	{ \
		return NewObject<TaskNamePlusFriendlyName##Task>( nullptr ); \
	}\
	\
	static const char* M2_GetNodeName() \
	{\
		return NodeName; \
	}\
}

	//////////////////////////////////////////////////////////////////////////
	// LESS THAN (<)

#define SAT_DECLARE_MATHS2_LESS_THAN_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2LessThan##FriendlyName##Node, NodeName, Maths2LessThan##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_LESS_THAN_CLASS( "float < float", Float, float );
	SAT_DECLARE_MATHS2_LESS_THAN_CLASS( "int < int", Int, int );
	SAT_DECLARE_MATHS2_LESS_THAN_CLASS( "U32 < U32", UInt, uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// LESS THAN OR EQU TO (<=)

#define SAT_DECLARE_MATHS2_LESS_THAN_OR_EQU_TO_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2LessThanOrEqu##FriendlyName##Node, NodeName, Maths2LessThanOrEqu##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_LESS_THAN_OR_EQU_TO_CLASS( "float <= float", Float, float );
	SAT_DECLARE_MATHS2_LESS_THAN_OR_EQU_TO_CLASS( "int <= int", Int, int );
	SAT_DECLARE_MATHS2_LESS_THAN_OR_EQU_TO_CLASS( "U32 <= U32", UInt, uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// GREATER THAN (>)

#define SAT_DECLARE_MATHS2_GREATER_THAN_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2GreaterThan##FriendlyName##Node, NodeName, Maths2GreaterThan##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_GREATER_THAN_CLASS( "float > float", Float, float );
	SAT_DECLARE_MATHS2_GREATER_THAN_CLASS( "int > int", Int, int );
	SAT_DECLARE_MATHS2_GREATER_THAN_CLASS( "U32 > U32", UInt, uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// GREATER THAN OR EQU TO (>=)

#define SAT_DECLARE_MATHS2_GREATER_THAN_OR_EQU_TO_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2GreaterThanOrEqu##FriendlyName##Node, NodeName, Maths2GreaterThanOrEqu##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_GREATER_THAN_OR_EQU_TO_CLASS( "float >= float", Float, float );
	SAT_DECLARE_MATHS2_GREATER_THAN_OR_EQU_TO_CLASS( "int >= int", Int, int );
	SAT_DECLARE_MATHS2_GREATER_THAN_OR_EQU_TO_CLASS( "U32 >= U32", UInt, uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// EQU TO (==)

#define SAT_DECLARE_MATHS2_EQUAL_TO_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2EquTo##FriendlyName##Node, NodeName, Maths2EquTo##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_EQUAL_TO_CLASS( "bool == bool", Bool, bool );
	SAT_DECLARE_MATHS2_EQUAL_TO_CLASS( "float == float", Float, float );
	SAT_DECLARE_MATHS2_EQUAL_TO_CLASS( "int == int", Int, int );
	SAT_DECLARE_MATHS2_EQUAL_TO_CLASS( "U32 == U32", UInt, uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// NOT EQU TO (!=)

#define SAT_DECLARE_MATHS2_NOT_EQUAL_TO_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2NotEqualTo##FriendlyName##Node, NodeName, Maths2NotEqualTo##FriendlyName, CppType )

	// Special case for bool
#define SAT_DECLARE_MATHS2_NOT_BOOL_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_BOOL_ONLY_NODE( SMaths2NotEqualTo##FriendlyName##Node, NodeName, Maths2NotEqualTo##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_NOT_EQUAL_TO_CLASS( "float != float", Float, float );
	SAT_DECLARE_MATHS2_NOT_EQUAL_TO_CLASS( "int != int", Int, int );
	SAT_DECLARE_MATHS2_NOT_EQUAL_TO_CLASS( "U32 != U32", UInt, uint32_t );

	// Special case for bool (inverter)
	SAT_DECLARE_MATHS2_NOT_BOOL_CLASS( "boolean NOT", Bool, bool );

	//////////////////////////////////////////////////////////////////////////
	// AND (&&)

#define SAT_DECLARE_MATHS2_AND_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2And##FriendlyName##Node, NodeName, Maths2And##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_AND_CLASS( "boolean AND", Bool, bool );

	//////////////////////////////////////////////////////////////////////////
	// OR (||)

#define SAT_DECLARE_MATHS2_OR_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS2_NODE( SMaths2Or##FriendlyName##Node, NodeName, Maths2Or##FriendlyName, CppType )

	SAT_DECLARE_MATHS2_OR_CLASS( "boolean OR", Bool, bool );

}
