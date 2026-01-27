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
#include "AssetManager.h"

#include "MemoryAssetDependency.h"

#include "Saturn/Core/App.h"

#if !defined(SAT_DIST)
#include "Saturn/Serialisation/YAML/AssetManagerSerialiser.h"
#endif

#include "Saturn/Project/Project.h"

namespace Saturn {

	AssetManager::AssetManager()
	{
		SingletonStorage::AddSingleton( this );

		const auto project = Project::GetActiveProject();
		auto assetDir = project->GetFullAssetPath();
		assetDir /= "AssetRegistry.sreg";

		m_Assets = Ref<AssetRegistry>::Create();
		m_Assets->m_Path = assetDir;

		// In distribution builds asset registry is loaded by the Asset Bundle!
#if !defined(SAT_DIST)
		AssetManagerSerialiser ars;
		ars.Deserialise();
#endif
	}

	void AssetManager::Terminate()
	{
		if( m_Assets )
			m_Assets = nullptr;
	}

	Ref<Asset> AssetManager::FindAsset( AssetID id )
	{
		return m_Assets->FindAsset( id );
	}

	Ref<Asset> AssetManager::FindAsset( const std::filesystem::path& rPath )
	{
		return m_Assets->FindAsset( rPath );
	}

	Ref<Asset> AssetManager::FindAsset( const std::string& rName, AssetType type )
	{
		return m_Assets->FindAsset( rName, type );
	}

	void AssetManager::RemoveAsset( AssetID id )
	{
		Project::GetActiveProject()->RemoveAssetFromDefaults( id );

		if( IsAssetLoaded( id ) )
			m_Assets->m_LoadedAssets[ id ]->OnDelete();

		m_Assets->RemoveAsset( id );
		Save();
	}

	void AssetManager::UpdateAssetDependency( AssetID assetID, AssetID oldDepID, AssetID newDepID )
	{
		if( IsAssetLoaded( assetID ) )
		{
			m_Assets->m_LoadedAssets[ assetID ]->OnAssetDependencyReplace( oldDepID, newDepID );
			Save();
		}
	}

	AssetID AssetManager::DuplicateAsset( Ref<Asset> asset )
	{
		Ref<Asset> dupedAsset = m_Assets->m_Assets[ m_Assets->CreateAsset( asset->Type ) ];
		dupedAsset->Path = asset->Path;
		dupedAsset->Name = asset->Name;

		return dupedAsset->ID;
	}

	AssetID AssetManager::CreateAsset( AssetType type )
	{
		return m_Assets->CreateAsset( type );
	}

	bool AssetManager::IsAssetLoaded( AssetID id )
	{
		return m_Assets->IsAssetLoaded( id );
	}

	AssetID AssetManager::PathToID( const std::filesystem::path& rPath )
	{
		return m_Assets->PathToID( rPath );
	}

	void AssetManager::Save() const
	{
		// In distribution builds we are in a Read Only state!
#if !defined(SAT_DIST)
		AssetManagerSerialiser ars;
		ars.Serialise();
#endif
	}

#if !defined(SAT_DIST)

	void AssetManager::RenameAsset( AssetID id, const std::string& rName )
	{
		Ref<Asset> asset = m_Assets->FindAsset( id );
		if( asset )
		{
			const std::wstring& rExt = asset->Path.extension();

			asset->Name = rName;
			asset->Path.replace_filename( rName );
			asset->Path.replace_extension( rExt );

			if( IsAssetLoaded( id ) )
			{
				m_Assets->m_LoadedAssets[ id ]->Name = rName;
				m_Assets->m_LoadedAssets[ id ]->Path = asset->Path;
			}

			Save();
		}
	}

	void AssetManager::UpdateAssetPathsOnRename( const std::filesystem::path& rOldPath, const std::filesystem::path& rNewPath )
	{
		bool assetRegistryModified = false;

		for( auto& [id, rAsset] : m_Assets->m_Assets )
		{
			const std::filesystem::path pathStem = rAsset->Path.parent_path();
			if( pathStem == rOldPath )
			{
				rAsset->Path = rNewPath / rAsset->Name;
			
				if( IsAssetLoaded( id ) )
				{
					// Okay to use rAsset->Name...
					m_Assets->m_LoadedAssets[ id ]->Path = rNewPath / rAsset->Name;
				}

				assetRegistryModified = true;
			}
		}

		if( assetRegistryModified )
			AssetManager::Get().Save();
	}

	void AssetManager::RegisterMemoryAssetDependency( AssetID dependencyID, MemoryAssetDependencyBase* pBase )
	{
		if( dependencyID != 0 )
			m_MemoryAssetDependencies[ dependencyID ].insert( { pBase } );
	}

	void AssetManager::UnregisterMemoryAssetDependency( AssetID dependencyID, MemoryAssetDependencyBase* pBase )
	{
		if( m_MemoryAssetDependencies.find( dependencyID ) != m_MemoryAssetDependencies.end() )
		{
			m_MemoryAssetDependencies[ dependencyID ].erase( { pBase } );

			// Remove dependency if there is no more dependencies
			if( !m_MemoryAssetDependencies[ dependencyID ].size() ) m_MemoryAssetDependencies.erase( dependencyID );
		}
	}

	const std::unordered_map<Saturn::AssetID, std::unordered_set<MemoryAssetDependencyBase*>> AssetManager::GetAssetDependencies() const
	{
		return m_MemoryAssetDependencies;
	}

	const std::unordered_set<MemoryAssetDependencyBase*> AssetManager::GetAssetDependenciesForAsset( const Ref<Asset> asset ) const
	{
		if( m_MemoryAssetDependencies.contains( asset->ID ) )
		{
			return m_MemoryAssetDependencies.at( asset->ID );
		}
		else
			return {};
	}

	bool AssetManager::DoesAssetHaveDependencies( Ref<Asset> asset )
	{
		return m_MemoryAssetDependencies.contains( asset->ID ) || CheckPureAssetDependencies( asset );
	}

	bool AssetManager::CheckPureAssetDependencies( Ref<Asset> asset ) 
	{
		for( const auto& [assetID, deps] : m_AssetDependencies )
		{
			for( const auto& depID : deps )
			{
				if( depID == asset->ID )
					return true;
			}
		}

		return false;
	}

	void AssetManager::SanitiseAssetDependencies()
	{
		std::vector<AssetID> pendingIDsToRemove;

		for( auto& [assetID, deps] : m_AssetDependencies )
		{
			if( !AssetManager::Get().DoesAssetIDExist( assetID ) )
			{
				pendingIDsToRemove.emplace_back( assetID );
			}
			else
			{
				std::erase_if( deps, []( const auto& rDependant ) -> bool
				{
					return !AssetManager::Get().DoesAssetIDExist( rDependant );
				} );

				// Remove from asset dependencies map if we no longer have dependencies.
				if( deps.empty() )
					pendingIDsToRemove.emplace_back( assetID );
			}
		}

		for( auto& rAssetID : pendingIDsToRemove )
		{
			SAT_CORE_WARN( "Removing useless Asset Dependency: {0}", rAssetID );
			m_AssetDependencies.erase( rAssetID );
		}
	}

	void AssetManager::RegisterAssetDependency( AssetID assetID, AssetID dependencyID )
	{
		if( assetID != 0 && dependencyID != 0 )
			m_AssetDependencies[ assetID ].insert( dependencyID );
	}

	void AssetManager::UnregisterAssetDependency( AssetID assetID, AssetID dependencyID )
	{
		if( m_AssetDependencies.find( dependencyID ) != m_AssetDependencies.end() )
		{
			m_AssetDependencies[ dependencyID ].erase( assetID );

			if( !m_AssetDependencies[ dependencyID ].size() ) m_AssetDependencies.erase( dependencyID );
		}
	}

	void AssetManager::UnregisterAllAssetDependencies( AssetID assetID )
	{
		m_AssetDependencies.erase( assetID );
	}

	const std::unordered_map<AssetID, std::unordered_set<AssetID>> AssetManager::GetPureAssetDependencies() const
	{
		return m_AssetDependencies;
	}

	const std::unordered_set<AssetID> AssetManager::GetPureAssetDependenciesForAsset( const Ref<Asset> asset ) const
	{
		std::unordered_set<AssetID> map;

		for( auto& [assetID, deps] : m_AssetDependencies )
		{
			for( auto& depID : deps )
			{
				if( depID == asset->ID )
				{
					map.insert( assetID );
				}
			}
		}

		return map;
	}

#else
	void AssetManager::RegisterMemoryAssetDependency( AssetID dependencyID, MemoryAssetDependencyBase* pBase )
	{
	}

	void AssetManager::UnregisterMemoryAssetDependency( AssetID dependencyID, MemoryAssetDependencyBase* pBase )
	{
	}

	const std::unordered_map<Saturn::AssetID, std::unordered_set<MemoryAssetDependencyBase*>> AssetManager::GetAssetDependencies() const
	{
		return {};
	}

	const std::unordered_set<MemoryAssetDependencyBase*> AssetManager::GetAssetDependenciesForAsset( const Ref<Asset> asset ) const
	{
		return {};
	}

	bool AssetManager::DoesAssetHaveDependencies( Ref<Asset> asset )
	{
		return false;
	}

	void AssetManager::RegisterAssetDependency( AssetID assetID, AssetID dependencyID )
	{
	}

	void AssetManager::UnregisterAssetDependency( AssetID assetID, AssetID dependencyID )
	{
	}

	void AssetManager::UnregisterAllAssetDependencies( AssetID assetID )
	{
	}

	const std::unordered_map<AssetID, std::unordered_set<AssetID>> AssetManager::GetPureAssetDependencies() const
	{
		return {};
	}

#endif

}
