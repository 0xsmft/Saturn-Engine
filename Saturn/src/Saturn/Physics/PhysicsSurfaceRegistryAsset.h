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

#include "Saturn/Asset/Asset.h"

namespace Saturn {

	struct PhysicsSurface
	{
		std::string Name;
#if !defined(SAT_DIST)
		// Unique ID for AssetViewer
		UUID RenderID;
#endif
	};

	//
	// The surface registry asset is an asset that holds
	// the list of all the names of the possible surface
	// types.
	// 
	// Each PhysicsMaterial will have a surface type, so
	// on contact callbacks the material ID is given, thus
	// we can figure out what surface was hit and maybe 
	// decide what sound to play or maybe deal some damage.
	//
	class PhysicsSurfaceRegistryAsset : public Asset
	{
	public:
		PhysicsSurfaceRegistryAsset();
		PhysicsSurfaceRegistryAsset( const Ref<Asset>& rBase );

		virtual ~PhysicsSurfaceRegistryAsset();

	public:
		void AddSurfaceType( const std::string& rName );

	public:
		[[nodiscard]] bool DoesSurfaceTypeExist( const std::string& rName );

		const std::vector<PhysicsSurface>& GetNamesList() const { return m_Surfaces; }

	private:
		std::vector<PhysicsSurface> m_Surfaces;

	private:
		friend class PhysicsSurfaceRegistryAssetViewer;
		friend class PhysicsSurfaceRegistryAssetSerialiser;
		friend class RawPhysicsSurfaceRegistryAssetSerialiser;
	};
}
