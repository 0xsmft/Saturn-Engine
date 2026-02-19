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

#pragma once

namespace Saturn::Auxiliary {

	//////////////////////////////////////////////////////////////////////////
	// VEC 2

	static void SerialiseImVec2( const ImVec2& rVector, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rVector.x, rStream );
		RawSerialisation::WriteObject( rVector.y, rStream );
	}

	static void DeserialiseImVec2( ImVec2& rVector, FDependentIStream& rStream )
	{
		RawSerialisation::ReadObject( rVector.x, rStream );
		RawSerialisation::ReadObject( rVector.y, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// VEC 4

	static void SerialiseImVec4( const ImVec4& rVector, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rVector.x, rStream );
		RawSerialisation::WriteObject( rVector.y, rStream );
		RawSerialisation::WriteObject( rVector.z, rStream );
		RawSerialisation::WriteObject( rVector.w, rStream );
	}

	static void DeserialiseImVec4( ImVec4& rVector, FDependentIStream& rStream )
	{
		RawSerialisation::ReadObject( rVector.x, rStream );
		RawSerialisation::ReadObject( rVector.y, rStream );
		RawSerialisation::ReadObject( rVector.z, rStream );
		RawSerialisation::ReadObject( rVector.w, rStream );
	}

	static void SerialiseImColor( const ImColor& rColor, std::ofstream& rStream )
	{
		SerialiseImVec4( rColor.Value, rStream );
	}

	static void DeserialiseImColor( ImColor& rColor, FDependentIStream& rStream )
	{
		DeserialiseImVec4( rColor.Value, rStream );
	}
}
