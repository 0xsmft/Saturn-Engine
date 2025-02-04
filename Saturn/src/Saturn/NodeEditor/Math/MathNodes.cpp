/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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
#include "MathNodes.h"

#include "Saturn/NodeEditor/Runtime/NodeEditorRuntime.h"

namespace Saturn {

	MathAddFloats::MathAddFloats()
		: Node()
	{
		Name = "Add Float";
		CreateNode();
	}

	MathAddFloats::MathAddFloats( const std::string& rName )
		: Node()
	{
		Name = rName;
		CreateNode();
	}

	void MathAddFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Add;
		Color = ImColor( 147, 226, 74 );

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	MathAddFloats::~MathAddFloats()
	{
	}

	NodeEditorCompilationStatus MathAddFloats::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		if( pEvaluator == nullptr )
			return NodeEditorCompilationStatus::Failed;
		
		float Result{};

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result += pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		Ref<Link> link = pEvaluator->GetTargetEditor()->FindLinkByPin( Outputs[ 0 ]->ID );

		Ref<Pin> pin = pEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
		if( pin && pin->Type == PinType::Float )
		{
			Ref<FloatPin> fpin = pin.As<FloatPin>();
			fpin->Data = Result;
		}

		return NodeEditorCompilationStatus::Success;
	}
	//////////////////////////////////////////////////////////////////////////
	// MATH SUBTRACT (FLOAT)

	MathSubFloats::MathSubFloats()
	{
		Name = "Subtract Float";
		CreateNode();
	}

	MathSubFloats::MathSubFloats( const std::string& rName )
	{
		Name = rName;
		CreateNode();
	}

	MathSubFloats::~MathSubFloats()
	{
	}

	void MathSubFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Subtract;
		Color = ImColor( 147, 226, 74 );

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	NodeEditorCompilationStatus MathSubFloats::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		if( pEvaluator == nullptr )
			return NodeEditorCompilationStatus::Failed;

		float Result{};

		float maxValue = std::numeric_limits<float>::lowest();
		float maxIndex = 0;

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			if( pin->Data > maxValue )
			{
				maxValue = pin->Data;
				maxIndex = i;
			}
		}

		// Subtract from highest
		Result = maxValue;

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			if( i == maxIndex ) continue;

			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result -= pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		Ref<Link> link = pEvaluator->GetTargetEditor()->FindLinkByPin( Outputs[ 0 ]->ID );

		Ref<Pin> pin = pEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
		if( pin && pin->Type == PinType::Float )
		{
			Ref<FloatPin> fpin = pin.As<FloatPin>();
			fpin->Data = Result;
		}

		return NodeEditorCompilationStatus::Success;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATH MULTIPLY (FLOAT)

	MathMulFloats::MathMulFloats()
		: Node()
	{
		Name = "Multiply Float";
		CreateNode();
	}

	MathMulFloats::MathMulFloats( const std::string& rName )
		: Node()
	{
		Name = rName;
		CreateNode();
	}

	void MathMulFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Multiply;
		Color = ImColor( 147, 226, 74 );

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	MathMulFloats::~MathMulFloats()
	{
	}

	NodeEditorCompilationStatus MathMulFloats::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		if( pEvaluator == nullptr )
			return NodeEditorCompilationStatus::Failed;

		float Result{};

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result *= pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		Ref<Link> link = pEvaluator->GetTargetEditor()->FindLinkByPin( Outputs[ 0 ]->ID );

		Ref<Pin> pin = pEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
		if( pin && pin->Type == PinType::Float )
		{
			Ref<FloatPin> fpin = pin.As<FloatPin>();
			fpin->Data = Result;
		}

		return NodeEditorCompilationStatus::Success;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATH DIVIDE (FLOAT)

	MathDivideFloats::MathDivideFloats()
		: Node()
	{
		Name = "Divide Float";
		CreateNode();
	}

	MathDivideFloats::MathDivideFloats( const std::string& rName )
		: Node()
	{
		Name = rName;
		CreateNode();
	}

	void MathDivideFloats::CreateNode()
	{
		ExecutionType = NodeExecutionType::Divide;
		Color = ImColor( 147, 226, 74 );

		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "", PinKind::Input ) );

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
	}

	MathDivideFloats::~MathDivideFloats()
	{
	}

	NodeEditorCompilationStatus MathDivideFloats::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		if( pEvaluator == nullptr )
			return NodeEditorCompilationStatus::Failed;

		float Result{};

		float maxValue = std::numeric_limits<float>::lowest();
		float maxIndex = 0;

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			if( pin->Data > maxValue )
			{
				maxValue = pin->Data;
				maxIndex = i;
			}
		}

		Result = maxValue;

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result /= pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		Ref<Link> link = pEvaluator->GetTargetEditor()->FindLinkByPin( Outputs[ 0 ]->ID );

		Ref<Pin> pin = pEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
		if( pin && pin->Type == PinType::Float )
		{
			Ref<FloatPin> fpin = pin.As<FloatPin>();
			fpin->Data = Result;
		}

		return NodeEditorCompilationStatus::Success;
	}

}
