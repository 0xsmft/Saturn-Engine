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
#include "Node.h"

#include "UI/NodeEditor.h"

#include "Saturn/Serialisation/RawSerialisation.h"

#include "builders.h"

namespace Saturn {

	Node::Node( const std::string& rName )
		: Name( rName )
	{
	}

	Node::~Node()
	{
	}

	void Node::Destroy()
	{
		for( auto& rInput : Inputs )
		{
			rInput = nullptr;
		}

		for( auto& rOutput : Outputs )
		{
			rOutput = nullptr;
		}

		Inputs.clear();
		Outputs.clear();
	}

	void Node::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, NodeEditorBase* pBase )
	{
		rBuilder.Begin( ed::NodeId( ID ) );

		rBuilder.Header( Color );

		ImGui::Spring( 0 );
		ImGui::TextUnformatted( Name.c_str() );
		ImGui::Spring( 1 );
		ImGui::Dummy( ImVec2( 0, 28 ) );
		ImGui::Spring( 0 );

		rBuilder.EndHeader();

		uint32_t pinIndex = 0;
		for( auto& rInput : Inputs )
		{
			rInput->Render( rBuilder, pBase->IsLinked( rInput->ID ), pinIndex );
			pinIndex++;
		}

		for( auto& rOutput : Outputs )
		{
			if( rOutput->Type == PinType::Delegate )
				continue;

			rOutput->Render( rBuilder, pBase->IsLinked( rOutput->ID ), 0 );
		}

		rBuilder.End();
	}

	static void SerialiseImColor( const ImColor& rColor, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rColor.Value, rStream );
	}

	static void DeserialiseImColor( ImColor& rColor, Node::IStream& rStream )
	{
		RawSerialisation::ReadObject( rColor.Value, rStream );
	}

	static void SerialiseImVec2( const ImVec2& rVector, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rVector.x, rStream );
		RawSerialisation::WriteObject( rVector.y, rStream );
	}

	static void DeserialiseImVec2( ImVec2& rVector, Node::IStream& rStream )
	{
		RawSerialisation::ReadObject( rVector.x, rStream );
		RawSerialisation::ReadObject( rVector.y, rStream );
	}

	void Node::Serialise( const Ref<Node>& rObject, std::ofstream& rStream )
	{
		UUID::Serialise( rObject->ID, rStream );
		RawSerialisation::WriteString( rObject->Name, rStream );

		RawSerialisation::WriteObject( rObject->Color, rStream );
		RawSerialisation::WriteObject( rObject->Type, rStream );
		SerialiseImVec2( rObject->Size, rStream );
		SerialiseImVec2( rObject->Position, rStream );

		RawSerialisation::WriteString( rObject->ActiveState, rStream );
		RawSerialisation::WriteString( rObject->SavedState, rStream );

		for( const auto& rInput : rObject->Inputs )
		{
			Pin::Serialise( rInput, rStream );
		}

		for( const auto& rOutput : rObject->Outputs )
		{
			Pin::Serialise( rOutput, rStream );
		}

		rObject->OnSerialise( rStream );
	}

	void Node::Deserialise( Ref<Node>& rObject, IStream& rStream )
	{
		UUID::Deserialise( rObject->ID, rStream );
		rObject->Name = RawSerialisation::ReadString( rStream );

		RawSerialisation::ReadObject( rObject->Color, rStream );
		RawSerialisation::ReadObject( rObject->Type, rStream );
		DeserialiseImVec2( rObject->Size, rStream );
		DeserialiseImVec2( rObject->Position, rStream );

		rObject->ActiveState = RawSerialisation::ReadString( rStream );
		rObject->SavedState = RawSerialisation::ReadString( rStream );

		for( size_t i = 0; i < rObject->Inputs.size(); i++ )
		{
			Pin::Deserialise( rObject->Inputs[ i ], rStream );
		}

		for( size_t i = 0; i < rObject->Outputs.size(); i++ )
		{
			Pin::Deserialise( rObject->Outputs[ i ], rStream );
		}

#if !defined(SAT_DIST)
		ed::SetNodePosition( ed::NodeId( rObject->ID ), rObject->Position );
#endif

		rObject->OnDeserialise( rStream );
	}
}
