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

#include "sppch.h"
#include "AssetImporter.h"

namespace Saturn {

	AssetImporter::~AssetImporter()
	{
		m_AssetSerialisers.clear();
	}

	void AssetImporter::Init()
	{
		m_AssetSerialisers[ AssetType::Material			] = std::make_unique<MaterialAssetSerialiser>();
		m_AssetSerialisers[ AssetType::Prefab			] = std::make_unique<PrefabSerialiser>();
		m_AssetSerialisers[ AssetType::StaticMesh		] = std::make_unique<StaticMeshAssetSerialiser>();
		m_AssetSerialisers[ AssetType::Sound			] = std::make_unique<SoundSpecificationAssetSerialiser>();
		m_AssetSerialisers[ AssetType::PhysicsMaterial  ] = std::make_unique<PhysicsMaterialAssetSerialiser>();
	}

	bool AssetImporter::TryLoadData( Ref<Asset>& rAsset )
	{
		return m_AssetSerialisers[ rAsset->Type ]->TryLoadData( rAsset );
	}
}