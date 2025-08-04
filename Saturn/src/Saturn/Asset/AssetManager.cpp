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
#include "Saturn/Serialisation/AssetManagerSerialiser.h"

#include "Saturn/Project/Project.h"

namespace Saturn {

	AssetManager::AssetManager()
	{
		SingletonStorage::AddSingleton( this );

		auto project = Project::GetActiveProject();
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
		m_Assets->RemoveAsset( id );
		Save();
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
		AssetManagerSerialiser ars;
		ars.Serialise();
	}

#if !defined(SAT_DIST)
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
		for( auto& [assetID, deps] : m_AssetDependencies )
		{
			for( auto& depID : deps )
			{
				if( depID == asset->ID )
					return true;
			}
		}

		return false;
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
