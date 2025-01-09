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

#include "Saturn/Core/Ref.h"
#include "Saturn/Serialisation/RawSerialisation.h"
#include "Saturn/Core/UUID.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace Saturn {

	class Link : public RefTarget
	{
	public:
		Link() = default;

		Link( UUID id, UUID startPinId, UUID endPinId ) :
			ID( id ), StartPinID( startPinId ), EndPinID( endPinId ), Color( 255, 255, 255 )
		{
		}

		static void Serialise( const Ref<Link>& rObject, std::ofstream& rStream )
		{
			UUID::Serialise( rObject->ID, rStream );
			UUID::Serialise( rObject->StartPinID, rStream );
			UUID::Serialise( rObject->EndPinID, rStream );

			RawSerialisation::WriteObject( rObject->Color, rStream );
		}

		static void Deserialise( Ref<Link>& rObject, std::ifstream& rStream )
		{
			UUID::Deserialise( rObject->ID, rStream );
			UUID::Deserialise( rObject->StartPinID, rStream );
			UUID::Deserialise( rObject->EndPinID, rStream );

			RawSerialisation::ReadObject( rObject->Color, rStream );
		}

	public:
		UUID ID = 0;

		// Will always be an output pin.
		UUID StartPinID = 0;
		// Will always be an input pin.
		UUID EndPinID = 0;

		ImColor Color;
	};

}