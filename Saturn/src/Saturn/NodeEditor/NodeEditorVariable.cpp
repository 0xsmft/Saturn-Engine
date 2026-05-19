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
#include "NodeEditorVariable.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	void NodeEditorVariable::Serialise( const Ref<NodeEditorVariable>& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteString( rObject->m_Name, rStream );
		RawSerialisation::WriteObjectChecked( rObject->m_VariableID, rStream );
		RawSerialisation::WriteObjectChecked( rObject->m_DataType, rStream );
	}

	void NodeEditorVariable::Deserialise( Ref<NodeEditorVariable>& rObject, FDependentIStream& rStream )
	{
		rObject->m_Name = RawSerialisation::ReadString( rStream );
		RawSerialisation::ReadObjectChecked( rObject->m_VariableID, rStream );
		RawSerialisation::ReadObjectChecked( rObject->m_DataType, rStream );
	}
	
}
