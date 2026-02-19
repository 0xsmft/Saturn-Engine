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

#include "AssetImporterBase.h"

namespace Saturn {

	// We have two main AssetImporters.
	// This one is for importing YAML files, i.e. when we are in the editor, and in development.
	// The other is for importing assets from the VFS and is used when all of the assets are "cooked" used in distribution.
	class AssetImporter : public AssetImporterBase
	{
	public:
		AssetImporter() { Init(); }
		~AssetImporter();

	public:
		[[nodiscard]] bool TryLoadData(       Ref<Asset>& rAsset ) override;

		static AssetImporterType GetStaticType() { return AssetImporterType::YAML; }

	private:
		void Init();

		// TODO: Maybe change to Ref?
		std::unordered_map<AssetType, std::unique_ptr<AssetSerialiser>> m_AssetSerialisers;
	};
}
