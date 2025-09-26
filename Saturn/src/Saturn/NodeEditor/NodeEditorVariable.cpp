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
#include "NodeEditorVariable.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	void NodeEditorVariable::Serialise( const Ref<NodeEditorVariable>& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteObjectChecked( rObject->m_DataType, rStream );
		RawSerialisation::WriteObjectChecked( rObject->m_VariableID, rStream );

		// Value
		switch( rObject->m_DataType )
		{
			case NodeEditorVariableDataType::Float:
			{
				const auto value = rObject->Get<float>();
				RawSerialisation::WriteObjectChecked( value, rStream );
			} break;

			case NodeEditorVariableDataType::Int:
			{
				const auto value = rObject->Get<int>();
				RawSerialisation::WriteObjectChecked( value, rStream );
			} break;

			case NodeEditorVariableDataType::Double:
			{
				const auto value = rObject->Get<double>();
				RawSerialisation::WriteObjectChecked( value, rStream );
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				const auto value = rObject->Get<bool>();
				RawSerialisation::WriteObjectChecked( value, rStream );
			} break;

			case NodeEditorVariableDataType::Vec2:
			{
				const auto value = rObject->Get<glm::vec2>();
				RawSerialisation::WriteVec2( value, rStream );
			} break;

			case NodeEditorVariableDataType::Vec3:
			{
				const auto value = rObject->Get<glm::vec3>();
				RawSerialisation::WriteVec3( value, rStream );
			} break;

			case NodeEditorVariableDataType::Vec4:
			{
				const auto value = rObject->Get<glm::vec4>();
				RawSerialisation::WriteObjectChecked( value, rStream );
			} break;

			case NodeEditorVariableDataType::Class:
			{
				const auto pClass = rObject->Get<SClass*>();
				RawSerialisation::WriteObjectChecked( pClass->GetHash(), rStream );
			} break;

			case NodeEditorVariableDataType::String:
			{
				const auto value = rObject->Get<std::string>();
				RawSerialisation::WriteString( value, rStream );
			} break;

			case NodeEditorVariableDataType::Unknown:
			default: break;
		}

		RawSerialisation::WriteString( rObject->m_Name, rStream );
	}

	void NodeEditorVariable::Deserialise( Ref<NodeEditorVariable>& rObject, FDependentIStream& rStream )
	{
		RawSerialisation::ReadObjectChecked( rObject->m_DataType, rStream );
		RawSerialisation::ReadObjectChecked( rObject->m_VariableID, rStream );
		
		// Value
		switch( rObject->m_DataType )
		{
			case NodeEditorVariableDataType::Float:
			{
				float value = 0.0f;
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Int:
			{
				int value = 0u;
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Double:
			{
				double value = 0.0;
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				bool value = false;
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Vec2:
			{
				glm::vec2 value{};
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Vec3:
			{
				glm::vec3 value{};
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Vec4:
			{
				glm::vec4 value{};
				RawSerialisation::ReadObjectChecked( value, rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Class:
			{
				uint64_t classHash = 0llu;
				RawSerialisation::ReadObjectChecked( classHash, rStream );
			
				// WARNING: TODO: Shouldn't be calling RFastCheckClass
				SClass* pClass = ClassMetadataHandler::Get().RFastCheckClass( classHash );
				rObject->m_Value = pClass;
			} break;

			case NodeEditorVariableDataType::String:
			{
				std::string value{};
				value  = RawSerialisation::ReadString( rStream );

				rObject->m_Value = value;
			} break;

			case NodeEditorVariableDataType::Unknown:
			default: break;
		}

		rObject->m_Name = RawSerialisation::ReadString( rStream );
	}
	
}
