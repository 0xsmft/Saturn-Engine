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

	Saturn::NodeEvaluationState MathsAddFloats::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		if( evaluator == nullptr )
			return NodeEvaluationState::Failed;
		
		float Result{};

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result += pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		auto links = evaluator->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );

		for( const auto& rLink : links )
		{
			Ref<Pin> pin = evaluator->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin && pin->Type == PinType::Float )
			{
				Ref<FloatPin> fpin = pin.As<FloatPin>();
				fpin->Data = Result;
			}
		}

		return NodeEvaluationState::Evaluated;
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

	Saturn::NodeEvaluationState MathsSubFloats::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		if( evaluator == nullptr )
			return NodeEvaluationState::Failed;

		float Result{};

		float maxValue = std::numeric_limits<float>::lowest();
		size_t maxIndex = 0;

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
		auto links = evaluator->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );
		for( const auto& rLink : links )
		{
			Ref<Pin> pin = evaluator->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin && pin->Type == PinType::Float )
			{
				Ref<FloatPin> fpin = pin.As<FloatPin>();
				fpin->Data = Result;
			}
		}

		return NodeEvaluationState::Evaluated;
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

	Saturn::NodeEvaluationState MathsMulFloats::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		if( evaluator == nullptr )
			return NodeEvaluationState::Failed;

		float Result{};

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result *= pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		auto links = evaluator->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );
		for( const auto& rLink : links )
		{
			Ref<Pin> pin = evaluator->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin && pin->Type == PinType::Float )
			{
				Ref<FloatPin> fpin = pin.As<FloatPin>();
				fpin->Data = Result;
			}
		}

		return NodeEvaluationState::Evaluated;
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

	Saturn::NodeEvaluationState MathsDivideFloats::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		if( evaluator == nullptr )
			return NodeEvaluationState::Failed;

		float Result{};

		float maxValue = std::numeric_limits<float>::lowest();
		size_t maxIndex = 0;

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
			if( i == maxIndex ) continue;

			Ref<FloatPin> pin = Inputs[ i ].As<FloatPin>();
			Result /= pin->Data;
		}

		Outputs[ 0 ].As<FloatPin>()->Data = Result;

		// Write our output to other node input
		auto links = evaluator->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );
		for( const auto& rLink : links )
		{
			Ref<Pin> pin = evaluator->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin && pin->Type == PinType::Float )
			{
				Ref<FloatPin> fpin = pin.As<FloatPin>();
				fpin->Data = Result;
			}
		}

		return NodeEvaluationState::Evaluated;
	}

}
