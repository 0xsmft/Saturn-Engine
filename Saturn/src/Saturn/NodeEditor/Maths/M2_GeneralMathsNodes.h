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

	//////////////////////////////////////////////////////////////////////////
	// Maths2GeneralTyAlgebra

	//
	// Maths2GeneralTyAlgebra 
	// 
	// This node returns Ty after a certain mathematical operation
	// For example, Maths2GeneralTyAlgebra<float> with add would be float + float.
	//
	template<typename Ty>
	class Maths2GeneralTyOutNode : public NodeEditorBlueprintNode
	{
	public:
		Maths2GeneralTyOutNode()
			: NodeEditorBlueprintNode( "Maths2GeneralTyAlgebra" )
		{
			CreateNode();
		}

		Maths2GeneralTyOutNode( const std::string& rName )
			: NodeEditorBlueprintNode( rName )
		{
			CreateNode();
		}

		virtual ~Maths2GeneralTyOutNode() = default;

	protected:
		void CreateNode()
		{
			Inputs.push_back( Ref<PinTypeTraits<Ty>::PinType>::Create( "A", PinKind::Input ) );
			Inputs.push_back( Ref<PinTypeTraits<Ty>::PinType>::Create( "B", PinKind::Input ) );

			Outputs.push_back( Ref<PinTypeTraits<Ty>::PinType>::Create( "Result", PinKind::Output ) );
		}
	};

	//////////////////////////////////////////////////////////////////////////

#define SAT_DECLARE_MATHS_TY_NODE( ClassName, NodeName, TaskNamePlusFriendlyName, CppType ) \
SCLASS() \
class ClassName : public Maths2GeneralTyOutNode<CppType> \
{ \
	/* NB: Super class as NodeEditorBlueprintNode is intentional, we cannot use Maths2GeneralBoolAlgebra
	as a super class.*/ \
	SAT_DECLARE_CLASS( ClassName, NodeEditorBlueprintNode ); \
public: \
	ClassName() \
		: Maths2GeneralTyOutNode( NodeName ) \
	{ \
	} \
	ClassName( const std::string& rName ) \
		: Maths2GeneralTyOutNode( NodeName ) \
	{ \
	} \
	\
	virtual ~ClassName() = default; \
	\
	virtual NodeEditorTaskBase* ConvertToTask() override \
	{ \
		return NewObject<TaskNamePlusFriendlyName##Task>( nullptr ); \
	}\
	static const char* M2_GetNodeName() \
	{\
		return NodeName; \
	}\
}

	//////////////////////////////////////////////////////////////////////////
	// ADD

#define SAT_DECLARE_MATHS_ADD_CLASS( NodeName, FriendlyName, CppType ) SAT_DECLARE_MATHS_TY_NODE( SMaths2Add##FriendlyName##Node, NodeName, Maths2Add##FriendlyName, CppType )

	SAT_DECLARE_MATHS_ADD_CLASS( "float + float", Float, float );
	SAT_DECLARE_MATHS_ADD_CLASS( "int + int", Int, int );
	SAT_DECLARE_MATHS_ADD_CLASS( "U32 + U32", UInt, uint32_t );

}
