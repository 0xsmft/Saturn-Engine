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
#include "NodeEditorNodeBase.h"

#include "UI/NodeEditor.h"

#include "Saturn/Serialisation/RawSerialisation.h"

namespace Saturn {

	NodeEditorNodeBase::NodeEditorNodeBase( const std::string& rName )
		: Name( rName )
	{
	}

	NodeEditorNodeBase::~NodeEditorNodeBase()
	{
	}

	void NodeEditorNodeBase::Destroy()
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

	static void SerialiseImColor( const ImColor& rColor, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rColor.Value, rStream );
	}

	static void DeserialiseImColor( ImColor& rColor, NodeEditorNodeBase::IStream& rStream )
	{
		RawSerialisation::ReadObject( rColor.Value, rStream );
	}

	static void SerialiseImVec2( const ImVec2& rVector, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rVector.x, rStream );
		RawSerialisation::WriteObject( rVector.y, rStream );
	}

	static void DeserialiseImVec2( ImVec2& rVector, NodeEditorNodeBase::IStream& rStream )
	{
		RawSerialisation::ReadObject( rVector.x, rStream );
		RawSerialisation::ReadObject( rVector.y, rStream );
	}

	void NodeEditorNodeBase::Serialise( std::ofstream& rStream ) const
	{
		UUID::Serialise( ID, rStream );
		RawSerialisation::WriteString( Name, rStream );

		RawSerialisation::WriteObject( Color, rStream );
		RawSerialisation::WriteObject( Type, rStream );
		SerialiseImVec2( Size, rStream );
		SerialiseImVec2( Position, rStream );

		RawSerialisation::WriteString( ActiveState, rStream );
		RawSerialisation::WriteString( SavedState, rStream );

		for( const auto& rInput : Inputs )
		{
			Pin::Serialise( rInput, rStream );
		}

		for( const auto& rOutput : Outputs )
		{
			Pin::Serialise( rOutput, rStream );
		}
	}

	void NodeEditorNodeBase::Deserialise( IStream& rStream )
	{
		UUID::Deserialise( ID, rStream );
		Name = RawSerialisation::ReadString( rStream );

		RawSerialisation::ReadObject( Color, rStream );
		RawSerialisation::ReadObject( Type, rStream );
		DeserialiseImVec2( Size, rStream );
		DeserialiseImVec2( Position, rStream );

		ActiveState = RawSerialisation::ReadString( rStream );
		SavedState = RawSerialisation::ReadString( rStream );

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Pin::Deserialise( Inputs[ i ], rStream );
		}

		for( size_t i = 0; i < Outputs.size(); i++ )
		{
			Pin::Deserialise( Outputs[ i ], rStream );
		}

#if !defined(SAT_DIST)
		ed::SetNodePosition( ed::NodeId( ID ), Position );
#endif
	}

}
