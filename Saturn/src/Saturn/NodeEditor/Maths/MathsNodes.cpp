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

#include "sppch.h"
#include "MathsNodes.h"

#include "Saturn/NodeEditor/Runtime/NodeEditorRuntime.h"

// TODO: Should remove this
#include "MathsNodeLibrary.h"

#include "MathsTasks.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	MathsAddFloats::MathsAddFloats()
		: NodeEditorBlueprintNode( "Add Float" )
	{
		CreateNode();
	}

	MathsAddFloats::MathsAddFloats( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void MathsAddFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Add;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	MathsAddFloats::~MathsAddFloats()
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// MATHS SUBTRACT (FLOAT)

	MathsSubFloats::MathsSubFloats()
		: NodeEditorBlueprintNode( "Subtract Float" )
	{
		CreateNode();
	}

	MathsSubFloats::MathsSubFloats( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	MathsSubFloats::~MathsSubFloats()
	{
	}

	void MathsSubFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Subtract;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	//////////////////////////////////////////////////////////////////////////
	// MATHS MULTIPLY (FLOAT)

	MathsMulFloats::MathsMulFloats()
		: NodeEditorBlueprintNode( "Multiply Float" )
	{
		CreateNode();
	}

	MathsMulFloats::MathsMulFloats( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void MathsMulFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Multiply;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	MathsMulFloats::~MathsMulFloats()
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// MATHS DIVIDE (FLOAT)

	MathsDivideFloats::MathsDivideFloats()
		: NodeEditorBlueprintNode( "Divide Float" )
	{
		CreateNode();
	}

	MathsDivideFloats::MathsDivideFloats( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void MathsDivideFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Divide;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	MathsDivideFloats::~MathsDivideFloats()
	{
	}

	//////////////////////////////////////////////////////////////////////
	// MATHS NODE AUXILIARY

	SharedPtr<NodeEditorNodeBase> MathsNodesAuxiliary::DrawContextMenu( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<NodeEditorNodeBase> node;

		if( ImGui::MenuItem( "Add (Float)" ) ) 
		{
			node = MathsNodeLibrary::SpawnMathAdd( nodeEditor );
		}

		if( ImGui::MenuItem( "Subtract (Float)" ) )
		{
			node = MathsNodeLibrary::SpawnMathSub( nodeEditor );
		}

		if( ImGui::MenuItem( "Multiply (Float)" ) )
		{
			node = MathsNodeLibrary::SpawnMathMul( nodeEditor );
		}

		if( ImGui::MenuItem( "Divide (Float)" ) )
		{
			node = MathsNodeLibrary::SpawnMathDiv( nodeEditor );
		}

		if( ImGui::MenuItem( "Greater Than (Float)" ) )
		{
			node = MathsNodeLibrary::SpawnMathGT( nodeEditor );
		}

		if( ImGui::MenuItem( "Less Than (Float)" ) )
		{
			node = MathsNodeLibrary::SpawnMathLT( nodeEditor );
		}

		if( ImGui::MenuItem( "Not" ) )
		{
			node = MathsNodeLibrary::SpawnNotBool( nodeEditor );
		}

		if( ImGui::MenuItem( "Or" ) )
		{
			node = MathsNodeLibrary::SpawnOrBool( nodeEditor );
		}

		return node;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATHS GREATER THAN (FLOAT)

	MathsGreaterThanFloats::MathsGreaterThanFloats()
		: NodeEditorBlueprintNode( "Greater Than (Float)" )
	{
		CreateNode();
	}

	MathsGreaterThanFloats::MathsGreaterThanFloats( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void MathsGreaterThanFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::GreaterThan;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<FloatPin>::Create( "In", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Value", PinKind::Input ) );

		Outputs.push_back( Ref<BoolPin>::Create( "Out", PinKind::Output ) );
	}

	MathsGreaterThanFloats::~MathsGreaterThanFloats()
	{
	}

	NodeEditorTaskBase* MathsGreaterThanFloats::ConvertToTask()
	{
		return NewObject<SMathsGreaterThanFloatsTask>();
	}

	//////////////////////////////////////////////////////////////////////////
	// MATHS LESS THAN (FLOAT)

	MathsLessThanFloats::MathsLessThanFloats()
		: NodeEditorBlueprintNode( "Less Than (Float)" )
	{
		CreateNode();
	}

	MathsLessThanFloats::MathsLessThanFloats( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void MathsLessThanFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::LessThan;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<FloatPin>::Create( "In", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Value", PinKind::Input ) );

		Outputs.push_back( Ref<BoolPin>::Create( "Out", PinKind::Output ) );
	}

	MathsLessThanFloats::~MathsLessThanFloats()
	{
	}

	NodeEditorTaskBase* MathsLessThanFloats::ConvertToTask()
	{
		return NewObject<SMathsLessThanFloatsTask>();
	}

	//////////////////////////////////////////////////////////////////////////
	// MATHS NOT

	MathsNot::MathsNot()
		: Super( "Not" )
	{
		CreateNode();
	}

	MathsNot::MathsNot( const std::string& rName )
		: Super( rName )
	{
		CreateNode();
	}

	void MathsNot::CreateNode()
	{
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<BoolPin>::Create( "In", PinKind::Input ) );
		Outputs.push_back( Ref<BoolPin>::Create( "Result", PinKind::Output ) );
	}

	MathsNot::~MathsNot()
	{
	}

	NodeEditorTaskBase* MathsNot::ConvertToTask()
	{
		return NewObject<SMathsNotTask>();
	}

	//////////////////////////////////////////////////////////////////////////

	MathsOr::MathsOr()
		: Super()
	{
		CreateNode();
	}

	MathsOr::MathsOr( const std::string& rName )
		: Super()
	{
		CreateNode();
	}

	void MathsOr::CreateNode()
	{
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		Inputs.push_back( Ref<BoolPin>::Create( "A", PinKind::Input ) );
		Inputs.push_back( Ref<BoolPin>::Create( "B", PinKind::Input ) );

		Outputs.push_back( Ref<BoolPin>::Create( "Result", PinKind::Output ) );
	}

	MathsOr::~MathsOr()
	{
	}

	NodeEditorTaskBase* MathsOr::ConvertToTask()
	{
		return NewObject<SMathsOrTask>();
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( MathsAddFloats );
SAT_X31_CREATE_AUTO_REG( MathsSubFloats );
SAT_X31_CREATE_AUTO_REG( MathsMulFloats );
SAT_X31_CREATE_AUTO_REG( MathsDivideFloats );
SAT_X31_CREATE_AUTO_REG( MathsGreaterThanFloats );
SAT_X31_CREATE_AUTO_REG( MathsLessThanFloats );
SAT_X31_CREATE_AUTO_REG( MathsNot );
SAT_X31_CREATE_AUTO_REG( MathsOr );
