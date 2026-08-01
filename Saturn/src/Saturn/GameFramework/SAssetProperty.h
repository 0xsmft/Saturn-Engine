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

#include "SProperty.h"

#include "Saturn/Asset/Asset.h"

namespace Saturn {

	//
	// SProperty class for handling Assets.
	// 
	// This class holds the required AssetType
	// that can be specified as part of the 
	// SPROPERTY() macro.
	// 
	// By default it's AssetType::Unknown
	// meaning all assets will show up
	// in the Editor.
	//
	class SAssetProperty : public SProperty
	{
		SAssetProperty() = default;
	public:
		SAssetProperty( const std::string& rName, SPropertyType propType, const void* pGetFnp, void* pSetFnp, AssetType assetType )
			: SProperty( rName, propType, pGetFnp, pSetFnp ), m_AssetType( assetType )
		{
		}

		virtual ~SAssetProperty() = default;

		uint64_t GetProperty( SObject* pObject ) const;
		uint64_t GetProperty( const SObject* pObject ) const;

	public:
		AssetType GetAssetType() const { return m_AssetType; }

	private:
		AssetType m_AssetType = AssetType::Unknown;
	};

}
