/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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
#include "AssetIDPin.h"

#include "Saturn/NodeEditor/Node.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AssetIDPin::AssetIDPin( const std::string& rName, PinKind kind, AssetType assetType )
		: Pin( rName, PinType::AssetID, kind ), m_AssetType( assetType )
	{
	}

	AssetIDPin::AssetIDPin( UUID ID, const std::string& rName, PinType type, UUID nodeid )
		: Pin( ID, rName, PinType::AssetID, nodeid )
	{
	}

	AssetIDPin::~AssetIDPin()
	{
	}

	void AssetIDPin::OnRenderInput()
	{
		Render();
	}

	void AssetIDPin::OnRenderOutput()
	{
		Render();
	}

	void AssetIDPin::Render()
	{
		bool openAssetIDPopup = false;

		std::string name = m_AssetID == 0 ? "Select Asset" : std::to_string( m_AssetID );
		if( ImGui::Button( name.c_str() ) )
		{
			openAssetIDPopup = true;
		}

		ed::Suspend();
		Auxiliary::DrawAssetFinder( m_AssetType, &openAssetIDPopup, m_AssetID );
		ed::Resume();
	}

	void AssetIDPin::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_AssetID, rStream );
	}

	void AssetIDPin::OnDeserialise( std::ifstream& rStream )
	{
		RawSerialisation::ReadObject( m_AssetID, rStream );
	}

}
