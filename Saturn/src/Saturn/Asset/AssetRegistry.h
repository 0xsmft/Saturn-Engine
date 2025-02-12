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

#include "Asset.h"
#include "AssetImporter.h"

#include <unordered_map>
#include <unordered_set>

namespace Saturn {

	using AssetMap = std::unordered_map< AssetID, Ref<Asset> >;

	class AssetRegistry : public RefTarget
	{
	public:
		AssetRegistry();
		AssetRegistry( const AssetRegistry& rOther );

		~AssetRegistry();

		// Copies the other asset registry asset map, path and loaded assets into this.
		// NOTE: This will not copy the assets, this means that assets will still "shared" between them however loading assets into this asset registry will not affect the other.
		void CopyFrom( const Ref<AssetRegistry>& rOther );

		AssetID CreateAsset( AssetType type );
		Ref<Asset> FindAsset( AssetID id );

		Ref<Asset> FindAsset( const std::filesystem::path& rPath );
		Ref<Asset> FindAsset( const std::string& rName, AssetType type );

		std::vector<AssetID> FindAssetsWithType( AssetType type ) const;

		AssetID PathToID( const std::filesystem::path& rPath );

		void RemoveAsset( AssetID id );
		void TerminateAsset( AssetID id );

		[[nodiscard]] bool DoesIDExists( AssetID id );

		size_t GetSize();

		const AssetMap& GetAssetMap() const { return m_Assets; }
		const AssetMap& GetLoadedAssetsMap() const { return m_LoadedAssets; }

		std::filesystem::path& GetPath() { return m_Path; }
		const std::filesystem::path& GetPath() const { return m_Path; }

	private:
		void AddAsset( AssetID id );
		bool IsAssetLoaded( AssetID id );

  	private:
		AssetMap m_Assets;
		AssetMap m_LoadedAssets;

		std::filesystem::path m_Path;

	private:
		friend class AssetManagerSerialiser;
		friend class AssetManager;
		friend class AssetBundle;
	};
}