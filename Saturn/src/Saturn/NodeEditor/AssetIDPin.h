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

#include "Pin.h"
#include "Saturn/Asset/Asset.h"

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
#include <string>
#endif

namespace Saturn {

	class AssetIDPin : public Pin
	{
	public:
		AssetIDPin() = default;
		AssetIDPin( const std::string& rName, PinKind kind, AssetType assetType );
		AssetIDPin( UUID ID, const std::string& rName, PinType type, UUID nodeid );

		~AssetIDPin();

		AssetID GetAssetID() const { return m_AssetID; }
		void SetAssetID( AssetID ID ) { m_AssetID = ID; }

		AssetType GetAssetType() const { return m_AssetType; }
		void SetAssetType( AssetType type ) { m_AssetType = type; }

	protected:
		virtual void OnRenderOutput() override;
		virtual void OnRenderInput() override;

	protected:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	private:
		void RenderInternal();

	private:
		AssetID m_AssetID = 0;
		AssetType m_AssetType = AssetType::Unknown;
#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		std::string m_AssetName = "";
#endif
	};

}
