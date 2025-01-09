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

#pragma once

#include <string>

#include "Texture.h"

#include "ShaderDataType.h"

#include "Saturn/Core/Memory/Buffer.h"

#include "Saturn/Serialisation/RawSerialisation.h"

namespace Saturn {
	
	// A shader uniform represents a uniform variable in a shader.
	class ShaderUniform : public RefTarget
	{
	public:
		std::string Name = "";
		int Location = -1;
		ShaderDataType DataType = ShaderDataType::None;
		bool IsPushConstantData = false;

		uint32_t Offset = 0;
		uint32_t Size = 0;

		Buffer Data;

	public:
		ShaderUniform() 
		{
		}
		
		~ShaderUniform()
		{
			Terminate();
		}

		ShaderUniform( const std::string& name, int location, ShaderDataType type, size_t size, uint32_t offset, bool isPushConstantData = false )
			: Name( name ), Location( location ), DataType( type ), IsPushConstantData( isPushConstantData ), Size( (uint32_t)size ), Offset( offset )
		{
			Data.Allocate( size );
			Data.Zero_Memory();
		}

		void Terminate()
		{	
			Location = -1;
			DataType = ShaderDataType::None;
		}

		static void Serialise( const ShaderUniform& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteString( rObject.Name, rStream );
			RawSerialisation::WriteObject( rObject.Location, rStream );
			RawSerialisation::WriteObject( rObject.DataType, rStream );
			RawSerialisation::WriteObject( rObject.IsPushConstantData, rStream );
			RawSerialisation::WriteObject( rObject.Offset, rStream );
			RawSerialisation::WriteObject( rObject.Size, rStream );
		}

		static void Deserialise( ShaderUniform& rObject, std::ifstream& rStream )
		{
			rObject.Name = RawSerialisation::ReadString( rStream );

			RawSerialisation::ReadObject( rObject.Location, rStream );
			RawSerialisation::ReadObject( rObject.DataType, rStream );
			RawSerialisation::ReadObject( rObject.IsPushConstantData, rStream );
			RawSerialisation::ReadObject( rObject.Offset, rStream );
			RawSerialisation::ReadObject( rObject.Size, rStream );
		}

	private:
		friend class ShaderBundle;
	};

}