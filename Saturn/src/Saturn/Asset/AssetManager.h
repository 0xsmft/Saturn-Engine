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

#include "AssetDependency.h"
#include "AssetRegistry.h"

#if defined(SAT_DIST)
#include "VFSAssetImporter.h"
#endif

namespace Saturn {

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
		// \return Ref<Ty> if found, nullptr if not
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
			m_Assets->TerminateAsset( id );
		}

		[[deprecated( "Saturn::AssetManager::GetCombinedAssetMap is deprecated and will be removed. Consider using \"AssetManager::GetAssetRegistry::GetAssetMap\" instead." )]]
		inline AssetMap GetCombinedAssetMap() { return m_Assets->GetAssetMap(); }

		[[deprecated( "Saturn::AssetManager::GetCombinedLoadedAssetMap is deprecated and will be removed. Consider using \"AssetManager::GetAssetRegistry::GetLoadedAssetMap\" instead." )]]
		inline AssetMap GetCombinedLoadedAssetMap() { return m_Assets->GetLoadedAssetsMap(); }

		Ref<AssetRegistry>& GetAssetRegistry() { return m_Assets; }
		const Ref<AssetRegistry>& GetAssetRegistry() const { return m_Assets; }

		bool IsAssetLoaded( AssetID id );

		AssetID PathToID( const std::filesystem::path& rPath );

		void Save();

		template<typename Func>
		void Each( Func Function ) 
		{
			for( auto&& [ id, asset ] : m_Assets->GetAssetMap() )
			{
				Function( asset );
			}
		}

		[[nodiscard]] bool DoesAssetIDExist( AssetID id ) 
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

		size_t GetAssetRegistrySize() { return m_Assets->GetSize(); }

		void RegisterAssetDependency( AssetID dependencyID, AssetDependencyBase* pBase )
		{
			if( dependencyID != 0 )
				m_AssetDependencies[ dependencyID ].insert( { pBase } );
		}

		void UnregisterAssetDependency( AssetID dependencyID, AssetDependencyBase* pBase )
		{
			if( m_AssetDependencies.find( dependencyID ) != m_AssetDependencies.end() )
			{
				m_AssetDependencies[ dependencyID ].erase( { pBase } );

				if( !m_AssetDependencies[ dependencyID ].size() ) m_AssetDependencies.erase( dependencyID );
			}
		}

		const std::unordered_map<AssetID, std::unordered_set<AssetDependencyBase*>> GetAssetDependencies() const
		{
			return m_AssetDependencies;
		}

		const std::unordered_set<AssetDependencyBase*> GetAssetDependenciesForAsset( const Ref<Asset> asset ) const
		{
			if( m_AssetDependencies.contains( asset->ID ) )
			{
				return m_AssetDependencies.at( asset->ID );
			}
			else
				return {};
		}

		[[nodiscard]] bool DoesAssetHaveDependencies( Ref<Asset> asset );

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
		Ref<AssetRegistry> m_Assets = nullptr;

		// An Asset in our registry -> unordered_set of AssetDependency who depend on Asset
		//                 AssetID                     WhatDependsOnMe
		std::unordered_map<AssetID, std::unordered_set<AssetDependencyBase*>> m_AssetDependencies;

#if defined(SAT_DIST)
		VFSAssetImporter m_Importer;
#else
		AssetImporter m_Importer;
#endif

	private:
		friend class AssetBundle;
	};

}