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

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

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

	void NodeEditorNodeBase::Serialise( std::ofstream& rStream, bool isForDist ) const
	{
		RawSerialisation::WriteUUID( ID, rStream );
		RawSerialisation::WriteString( Name, rStream );

#if !defined(SAT_DIST)
		RawSerialisation::WriteObject( Color, rStream );
		RawSerialisation::WriteObject( RenderType, rStream );
		Auxiliary::SerialiseImVec2( ed::GetNodeSize( ed::NodeId( ID ) ), rStream );
		Auxiliary::SerialiseImVec2( ed::GetNodePosition( ed::NodeId( ID ) ), rStream );

		RawSerialisation::WriteString( ActiveState, rStream );
		RawSerialisation::WriteString( SavedState, rStream );
#endif

		RawSerialisation::WriteObject( Inputs.size(), rStream );
		RawSerialisation::WriteObject( Outputs.size(), rStream );

		for( const auto& rInput : Inputs )
		{
			rInput->Serialise( rStream );
		}

		for( const auto& rOutput : Outputs )
		{
			rOutput->Serialise( rStream );
		}
	}

	void NodeEditorNodeBase::Deserialise( FDependentIStream& rStream )
	{
		RawSerialisation::ReadUUID( ID, rStream );
		Name = RawSerialisation::ReadString( rStream );

#if !defined(SAT_DIST)
		RawSerialisation::ReadObject( Color, rStream );
		RawSerialisation::ReadObject( RenderType, rStream );

		// TODO: Will be removed, kept here for compatibility. 
		ImVec2 size{};
		Auxiliary::DeserialiseImVec2( size, rStream );

		ImVec2 position{};
		Auxiliary::DeserialiseImVec2( position, rStream );
	
		ed::SetNodePosition( ed::NodeId( ID ), position );

		ActiveState = RawSerialisation::ReadString( rStream );
		SavedState = RawSerialisation::ReadString( rStream );
#endif

		// We make sure to read the last saved pin count and not the real pin size from the Node it self,
		// This is to make sure that if a pin was added in a newer version but has not been serialised yet, 
		// we only read pins that we'd had before to stop us over reading data.

		size_t inputSize = 0llu, outputSize = 0llu;
		RawSerialisation::ReadObject( inputSize, rStream );
		RawSerialisation::ReadObject( outputSize, rStream );

		// NOTE: Pins are already created at this point hence why we don't write the size of the pins
		//       All we do is read back the data
		for( size_t i = 0; i < inputSize; ++i )
		{
			Inputs[ i ]->Deserialise( rStream );
		}

		for( size_t i = 0; i < outputSize; ++i )
		{
			Outputs[ i ]->Deserialise( rStream );
		}
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

static Saturn::SClass* RStaticLnkNodeEditorNodeBase()
{
	static Saturn::SClass* pClass = nullptr;
	if( !pClass ) 
	{
		const Saturn::SClassSpecification spec
		{ 
			"NodeEditorNodeBase", 
			( Saturn::SClassFlags ) Saturn::SC_VisibleInEditor | Saturn::SC_NoExtendedMetadata | Saturn::SC_Abstract, 
			0, 
			sizeof( Saturn::NodeEditorNodeBase ), alignof( Saturn::NodeEditorNodeBase ), 
			Saturn::FNV1A64( "NodeEditorNodeBase" ), 
			Saturn::NodeEditorNodeBase::Super::StaticClass(), nullptr, RStaticLnkNodeEditorNodeBase, nullptr, {}
		}; 
		
		Saturn::SClass::RConstructClass( pClass, spec );
	} 
	
	return pClass;
}

Saturn::SClass* Saturn::NodeEditorNodeBase::GetStaticClassInternal() 
{
	return RStaticLnkNodeEditorNodeBase();
} 

static Saturn::SClassRegistrar RCRNodeEditorNodeBase( RStaticLnkNodeEditorNodeBase );
