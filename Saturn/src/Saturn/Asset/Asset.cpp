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
#include "Asset.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	void Asset::SetAbsolutePath( const std::filesystem::path& rPath )
	{
		const std::filesystem::path rootDir = Project::GetActiveProject()->GetRootDir();

		Path = std::filesystem::relative( rPath, rootDir );
		Name = Path.stem().string();
	}

	void Asset::SerialiseData( std::ofstream& rStream ) const
	{
		// TODO: Support writing for a filesystem path.
		RawSerialisation::WriteString( Name, rStream );
		RawSerialisation::WriteString( Path, rStream );

		RawSerialisation::WriteObject( ID, rStream );
		RawSerialisation::WriteObject( Type, rStream );
		RawSerialisation::WriteObject( Version, rStream );
	}

	void Asset::DeserialiseData( std::ifstream& rStream )
	{
		// TODO: Support reading for a filesystem path.
		Name = RawSerialisation::ReadString( rStream );
		Path = RawSerialisation::ReadString( rStream );

		RawSerialisation::ReadObject( ID, rStream );
		RawSerialisation::ReadObject( Type, rStream );
		RawSerialisation::ReadObject( Version, rStream );
	}

}
