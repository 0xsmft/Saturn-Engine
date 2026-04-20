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

#include "Maths2Tasks.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

namespace Saturn {
	
	//////////////////////////////////////////////////////////////////////////
	// Maths2GeneralBoolAlgebra

	template<typename Ty>
	class Maths2GeneralBoolAlgebra : public NodeEditorBlueprintNode
	{
	public:
		Maths2GeneralBoolAlgebra();
		Maths2GeneralBoolAlgebra( const std::string& rName );
		virtual ~Maths2GeneralBoolAlgebra();

	protected:
		void CreateNode();
	};

	template<typename Ty>
	Maths2GeneralBoolAlgebra<Ty>::Maths2GeneralBoolAlgebra()
		: NodeEditorBlueprintNode( "Maths2GeneralBoolAlgebra" )
	{
		CreateNode();
	}

	template<typename Ty>
	Maths2GeneralBoolAlgebra<Ty>::Maths2GeneralBoolAlgebra( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	template<typename Ty>
	void Maths2GeneralBoolAlgebra<Ty>::CreateNode()
	{
		if constexpr( std::is_same<Ty, int>() ) 
		{
			Inputs.push_back( Ref<IntPin>::Create( "A", PinKind::Input ) );
			Inputs.push_back( Ref<IntPin>::Create( "B", PinKind::Input ) );
		}
		
		if constexpr( std::is_same<Ty, float>() )
		{
			Inputs.push_back( Ref<IntPin>::Create( "A", PinKind::Input ) );
			Inputs.push_back( Ref<IntPin>::Create( "B", PinKind::Input ) );
		}

		Outputs.push_back( Ref<BoolPin>::Create( "Result", PinKind::Output ) );
	}

	template<typename Ty>
	Maths2GeneralBoolAlgebra<Ty>::~Maths2GeneralBoolAlgebra()
	{
	}


	//////////////////////////////////////////////////////////////////////////
	// LESS THAN

#define SAT_DECLARE_MATHS_LESS_THAN_CLASS( x, name, type ) \
SCLASS() \
class x : public Maths2GeneralBoolAlgebra<type> \
{ \
SAT_DECLARE_CLASS( x, NodeEditorBlueprintNode ); \
public: \
	x() \
		: Maths2GeneralBoolAlgebra( name ) \
	{ \
	} \
	x( const std::string& rName ) \
		: Maths2GeneralBoolAlgebra( rName ) \
	{ \
	} \
	\
	virtual ~x() = default; \
}

	SAT_DECLARE_MATHS_LESS_THAN_CLASS( SMaths2LessThanFloats, "Less Than (Floats)", float );
	SAT_DECLARE_MATHS_LESS_THAN_CLASS( SMaths2LessThanInts, "Less Than (Ints)", int );
	SAT_DECLARE_MATHS_LESS_THAN_CLASS( SMaths2LessThanUInts,"Less Than (U32)", uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// LESS THAN OR EQU

#define SAT_DECLARE_MATHS_LESS_THAN_OR_EQU_CLASS( x, name, type ) \
SCLASS() \
class x : public Maths2GeneralBoolAlgebra<type> \
{ \
SAT_DECLARE_CLASS( x, NodeEditorBlueprintNode ); \
public: \
	x() \
		: Maths2GeneralBoolAlgebra( name ) \
	{ \
	} \
	x( const std::string& rName ) \
		: Maths2GeneralBoolAlgebra( rName ) \
	{ \
	} \
	\
	virtual ~x() = default; \
}

	SAT_DECLARE_MATHS_LESS_THAN_OR_EQU_CLASS( SMaths2LessThanOrEquFloats, "Less Than or Equal (Floats)", float );
	SAT_DECLARE_MATHS_LESS_THAN_OR_EQU_CLASS( SMaths2LessThanOrEquInts, "Less Than or Equal (Ints)", int );
	SAT_DECLARE_MATHS_LESS_THAN_OR_EQU_CLASS( SMaths2LessThanOrEquUInts, "Less Than or Equal (U32)", uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// GREATER THAN

#define SAT_DECLARE_MATHS_GREATER_THAN_CLASS( x, name, type ) \
SCLASS() \
class x : public Maths2GeneralBoolAlgebra<type> \
{ \
SAT_DECLARE_CLASS( x, NodeEditorBlueprintNode ); \
public: \
	x() \
		: Maths2GeneralBoolAlgebra( name ) \
	{ \
	} \
	x( const std::string& rName ) \
		: Maths2GeneralBoolAlgebra( rName ) \
	{ \
	} \
	\
	virtual ~x() = default; \
}

	SAT_DECLARE_MATHS_GREATER_THAN_CLASS( SMaths2GreaterThanFloats, "Greater Than (Floats)", float );
	SAT_DECLARE_MATHS_GREATER_THAN_CLASS( SMaths2GreaterThanInts, "Greater Than (Ints)", int );
	SAT_DECLARE_MATHS_GREATER_THAN_CLASS( SMaths2GreaterThanUInts, "Greater Than (U32)", uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// GREATER THAN OR EQU

#define SAT_DECLARE_MATHS_GREATER_THAN_OR_EQU_CLASS( x, name, type ) \
SCLASS() \
class x : public Maths2GeneralBoolAlgebra<type> \
{ \
SAT_DECLARE_CLASS( x, NodeEditorBlueprintNode ); \
public: \
	x() \
		: Maths2GeneralBoolAlgebra( name ) \
	{ \
	} \
	x( const std::string& rName ) \
		: Maths2GeneralBoolAlgebra( rName ) \
	{ \
	} \
	\
	virtual ~x() = default; \
}

	SAT_DECLARE_MATHS_GREATER_THAN_OR_EQU_CLASS( SMaths2GreaterThanOrEquFloats, "Greater Than or Equal (Floats)", float );
	SAT_DECLARE_MATHS_GREATER_THAN_OR_EQU_CLASS( SMaths2GreaterThanOrEquInts, "Greater Than or Equal (Ints)", int );
	SAT_DECLARE_MATHS_GREATER_THAN_OR_EQU_CLASS( SMaths2GreaterThanOrEquUInts, "Greater Than or Equal (U32)", uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// EQU TO

#define SAT_DECLARE_MATHS_EQU_TO_CLASS( x, name, type ) \
SCLASS() \
class x : public Maths2GeneralBoolAlgebra<type> \
{ \
SAT_DECLARE_CLASS( x, NodeEditorBlueprintNode ); \
public: \
	x() \
		: Maths2GeneralBoolAlgebra( name ) \
	{ \
	} \
	x( const std::string& rName ) \
		: Maths2GeneralBoolAlgebra( rName ) \
	{ \
	} \
	\
	virtual ~x() = default; \
}

	SAT_DECLARE_MATHS_EQU_TO_CLASS( SMaths2EqualsBool, "Equals (Bool)", bool );
	SAT_DECLARE_MATHS_EQU_TO_CLASS( SMaths2EqualsFloat, "Equals (Floats)", float );
	SAT_DECLARE_MATHS_EQU_TO_CLASS( SMaths2EqualsInt, "Equals (Ints)", int );
	SAT_DECLARE_MATHS_EQU_TO_CLASS( SMaths2EqualsUInt, "Equals (U32)", uint32_t );

	//////////////////////////////////////////////////////////////////////////
	// NOT EQU TO

#define SAT_DECLARE_MATHS_NEQU_TO_CLASS( x, name, type ) \
SCLASS() \
class x : public Maths2GeneralBoolAlgebra<type> \
{ \
SAT_DECLARE_CLASS( x, NodeEditorBlueprintNode ); \
public: \
	x() \
		: Maths2GeneralBoolAlgebra( name ) \
	{ \
	} \
	x( const std::string& rName ) \
		: Maths2GeneralBoolAlgebra( rName ) \
	{ \
	} \
	\
	virtual ~x() = default; \
}

	SAT_DECLARE_MATHS_NEQU_TO_CLASS( SMaths2NotEqualToBool, "Not Equal To (Bool)", bool );
	SAT_DECLARE_MATHS_NEQU_TO_CLASS( SMaths2NotEqualToFloat, "Not Equal To (Floats)", float );
	SAT_DECLARE_MATHS_NEQU_TO_CLASS( SMaths2NotEqualToInt, "Not Equal To (Ints)", int );
	SAT_DECLARE_MATHS_NEQU_TO_CLASS( SMaths2NotEqualToUInt, "Not Equal To (U32)", uint32_t );
}
