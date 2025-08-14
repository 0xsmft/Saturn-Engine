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

#include "VDirectory.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include <string>

namespace Saturn {

	class VFile : public RefTarget
	{
	public:
		VFile() {}
		VFile( const std::string& rName ) : Name( rName ), ParentDir( nullptr ) {}
		VFile( const std::string& rName, VDirectory* pParentDir ) : Name( rName ), ParentDir( pParentDir ) {}

		~VFile() = default;

	public:
		std::string Name;
		VDirectory* ParentDir = nullptr;
		
		std::vector<char> FileContent;
	
	public:
		static void Serialise( const Ref<VFile>& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteString( rObject->Name, rStream );
		}

		static void Deserialise( Ref<VFile>& rObject, std::ifstream& rStream )
		{
			rObject->Name = RawSerialisation::ReadString( rStream );
		}
	};
}