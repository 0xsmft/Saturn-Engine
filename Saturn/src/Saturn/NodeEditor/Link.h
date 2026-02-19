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

#include "Saturn/Core/Ref.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"
#include "Saturn/Core/UUID.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "Saturn/ImGui/SerialisationAux.h"

namespace Saturn {

	class Link : public RefTarget
	{
	public:
		Link() = default;

		Link( UUID id, UUID startPinId, UUID endPinId, ImColor color = ImColor( 255, 255, 255 ) ) :
			ID( id ), StartPinID( startPinId ), EndPinID( endPinId ), Color( color )
		{
		}

		static void Serialise( const Ref<Link>& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteUUID( rObject->ID, rStream );
			RawSerialisation::WriteUUID( rObject->StartPinID, rStream );
			RawSerialisation::WriteUUID( rObject->EndPinID, rStream );

			Auxiliary::SerialiseImColor( rObject->Color, rStream );
		}

		template<typename IStream>
		static void Deserialise( Ref<Link>& rObject, IStream& rStream )
		{
			RawSerialisation::ReadUUID( rObject->ID, rStream );
			RawSerialisation::ReadUUID( rObject->StartPinID, rStream );
			RawSerialisation::ReadUUID( rObject->EndPinID, rStream );

			Auxiliary::DeserialiseImColor( rObject->Color, rStream );
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