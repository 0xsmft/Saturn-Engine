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

#include "AssetRegistry.h"

#if defined(SAT_DIST)
#include "VFSAssetImporter.h"
#endif

#include <unordered_set>

namespace Saturn {

	class MemoryAssetDependencyBase;

	class AssetManager : public RefTarget
	{
	public:
		static inline AssetManager& Get() { return *SingletonStorage::GetSingleton<AssetManager>(); }
	public:
		AssetManager();
		~AssetManager() { Terminate(); }

		void Terminate();

	public:
		AssetID CreateAsset( AssetType type );

		Ref<Asset> FindAsset( AssetID id );
		Ref<Asset> FindAsset( const std::string& rName, AssetType type );

		// Note: rPath must be a relative path.
		Ref<Asset> FindAsset( const std::filesystem::path& rPath );

		template<typename Ty, typename... Args>
		Ref<Asset> CreateAssetAs( AssetType type, Args&&... rrArgs )
		{
			static_assert( std::is_base_of<Asset, Ty>::value, "Ty must be a child of Asset class!" );

			// This might not be the best way, first we create the "real" asset and add it to the registry, then create the template asset.
			auto id = CreateAsset( type );

			Ref<Ty> asset = Ref<Ty>::Create( std::forward<Args>( rrArgs )... );
			asset->ID = id;
			asset->Type = type;

			return asset;
		}

		// Import Asset using Ty
		// Where Ty is an asset.
		// This will try to find the loaded asset, if it does not exists it will try to load it.
		// @return Ref<Ty> if found, nullptr if not
		template<typename Ty>
		Ref<Ty> GetAssetAs( AssetID id )
		{
			static_assert( std::is_base_of<Asset, Ty>::value, "Ty must be a child of Asset class!" );

			return ImportAssetAs<Ty>( m_Assets, id );
		}

		// WARNING: THIS WILL PERMANENTLY REMOVE THE ASSET FROM THE REGISTRY!
		void RemoveAsset( AssetID id );

		void UnloadAsset( AssetID id )
		{
			m_Assets->DestroyAsset( id );
		}

		[[deprecated( "Saturn::AssetManager::GetCombinedAssetMap is deprecated and will be removed. Consider using \"AssetManager::GetAssetRegistry::GetAssetMap\" instead." )]]
		inline UnorderedAssetMap GetCombinedAssetMap() { return m_Assets->GetAssetMap(); }

		[[deprecated( "Saturn::AssetManager::GetCombinedLoadedAssetMap is deprecated and will be removed. Consider using \"AssetManager::GetAssetRegistry::GetLoadedAssetMap\" instead." )]]
		inline UnorderedAssetMap GetCombinedLoadedAssetMap() { return m_Assets->GetLoadedAssetsMap(); }

		Ref<AssetRegistry>& GetAssetRegistry() { return m_Assets; }
		const Ref<AssetRegistry>& GetAssetRegistry() const { return m_Assets; }

		bool IsAssetLoaded( AssetID id );

		AssetID PathToID( const std::filesystem::path& rPath );

		void Save() const;

		template<typename Func>
		void Each( Func Function ) 
		{
			for( auto&& [ id, asset ] : m_Assets->GetAssetMap() )
			{
				Function( asset );
			}
		}

		[[nodiscard]] bool DoesAssetIDExist( AssetID id ) const
		{
			return m_Assets->DoesIDExists( id );
		}

		void BumpAssetVersion( uint32_t newVersion ) 
		{
			for( auto& [ id, rAsset ] : m_Assets->m_Assets )
			{
				rAsset->Version = newVersion;
			}
		}

		size_t GetAssetRegistrySize() const { return m_Assets->GetSize(); }

	public:
		// Memory Asset Dependencies
		void RegisterMemoryAssetDependency( AssetID dependencyID, MemoryAssetDependencyBase* pBase );
		void UnregisterMemoryAssetDependency( AssetID dependencyID, MemoryAssetDependencyBase* pBase );

		const std::unordered_map<AssetID, std::unordered_set<MemoryAssetDependencyBase*>> GetAssetDependencies() const;

		const std::unordered_set<MemoryAssetDependencyBase*> GetAssetDependenciesForAsset( const Ref<Asset> asset ) const;
		[[nodiscard]] bool DoesAssetHaveDependencies( Ref<Asset> asset );

		// Asset Dependencies, i.e. asset interdependence, known as "Pure Dependencies" in the Engine.
		void RegisterAssetDependency( AssetID assetID, AssetID dependencyID );
		void UnregisterAssetDependency( AssetID assetID, AssetID dependencyID );
		void UnregisterAllAssetDependencies( AssetID assetID );
		bool CheckPureAssetDependencies( Ref<Asset> asset );

		const std::unordered_map<AssetID, std::unordered_set<AssetID>> GetPureAssetDependencies() const;
		const std::unordered_set<AssetID> GetPureAssetDependenciesForAsset( const Ref<Asset> asset ) const;

	private:
		template<typename Ty>
		Ref<Ty> ImportAssetAs( Ref<AssetRegistry> TargetRegistry, AssetID id )
		{
			auto AssetItr = TargetRegistry->m_Assets.find( id );

			if( AssetItr == TargetRegistry->m_Assets.end() )
				return nullptr;

			Ref<Asset> asset = AssetItr->second;

			if( !TargetRegistry->IsAssetLoaded( id ) )
			{
				bool loaded = m_Importer.TryLoadData( asset );
				if( !loaded )
					return nullptr;

				TargetRegistry->m_LoadedAssets[ id ] = asset;
			}
			else
				asset = TargetRegistry->m_LoadedAssets.at( id );

			return asset.As<Ty>();
		}

	private:
#if !defined(SAT_DIST)
		// An Asset in our registry -> unordered_set of AssetDependency who depend on Asset
		// Memory Dependency
		//                 AssetID                     WhatDependsOnMe
		std::unordered_map<AssetID, std::unordered_set<MemoryAssetDependencyBase*>> m_MemoryAssetDependencies;

		// Asset Dependency
		//                 AssetID                     WhatIDependOn
		std::unordered_map<AssetID, std::unordered_set<AssetID>> m_AssetDependencies;

		AssetImporter m_Importer;
#else
		VFSAssetImporter m_Importer;
#endif

		Ref<AssetRegistry> m_Assets = nullptr;
	private:
		friend class AssetBundle;
	};

}