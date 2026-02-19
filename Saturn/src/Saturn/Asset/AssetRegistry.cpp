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

#include <sppch.h>
#include "AssetRegistry.h"

namespace Saturn {

	AssetRegistry::AssetRegistry()
	{
	}

	AssetRegistry::AssetRegistry( const AssetRegistry& rOther )
		: m_Assets( rOther.m_Assets ), 
		m_LoadedAssets( rOther.m_LoadedAssets ), 
		m_Path( rOther.m_Path )
	{
	}

	AssetRegistry::~AssetRegistry()
	{
		m_Assets.clear();

		for( auto& [id, rAsset] : m_LoadedAssets )
		{
			rAsset = nullptr;
		}

		m_LoadedAssets.clear();
	}

	void AssetRegistry::CopyFrom( const Ref<AssetRegistry>& rOther )
	{
		m_Path = rOther->m_Path;
	
		m_Assets.insert( rOther->m_Assets.begin(), rOther->m_Assets.end() );
		m_LoadedAssets.insert( rOther->m_LoadedAssets.begin(), rOther->m_LoadedAssets.end() );
	}

	AssetID AssetRegistry::CreateAsset( AssetType type )
	{
		Ref<Asset> asset = Ref<Asset>::Create();
		asset->Type = type;
		asset->ID = UUID();

		m_Assets[ asset->ID ] = asset;

		return asset->ID;
	}

	Ref<Asset> AssetRegistry::FindAsset( AssetID id )
	{
		const auto Itr = m_Assets.find( id );
		return Itr == m_Assets.end() ? nullptr : Itr->second;
	}

	Ref<Asset> AssetRegistry::FindAsset( const std::filesystem::path& rPath )
	{
		const auto Itr = std::find_if( m_Assets.begin(), m_Assets.end(), 
			[rPath](const auto& kv)
			{
				return kv.second->Path == rPath;
			} );

		return Itr == m_Assets.end() ? nullptr : Itr->second;
	}

	Ref<Asset> AssetRegistry::FindAsset( const std::string& rName, AssetType type )
	{
		for( const auto& [id, asset] : m_Assets )
		{
			if( asset->Name == rName && asset->Type == type )
				return asset;
		}

		return nullptr;
	}

	std::vector<AssetID> AssetRegistry::FindAssetsWithType( AssetType type ) const
	{
		std::vector<AssetID> result;

		// There is a better way of doing this however we'll just keep it for now.
		for( const auto& [id, asset] : m_Assets )
		{
			if( asset->Type == type )
				result.push_back( id );
		}

		return result;
	}

	AssetID AssetRegistry::PathToID( const std::filesystem::path& rPath )
	{
		for( const auto& [id, asset] : m_Assets )
		{
			if( asset->Path == rPath )
				return id;
		}

		return 0;
	}

	void AssetRegistry::RemoveAsset( AssetID id )
	{
		if( DoesIDExists( id ) ) 
		{
			m_Assets[ id ] = nullptr;

			if( IsAssetLoaded( id ) )
			{
				m_LoadedAssets[ id ] = nullptr;
				m_LoadedAssets.erase( id );
			}

			m_Assets.erase( id );
		}
	}

	void AssetRegistry::DestroyAsset( AssetID id )
	{
		if( DoesIDExists( id ) )
		{
			m_Assets[ id ] = nullptr;

			if( IsAssetLoaded( id ) )
			{
				m_LoadedAssets[ id ] = nullptr;
			}
		}
	}

	bool AssetRegistry::DoesIDExists( AssetID id ) const
	{
		return m_Assets.contains( id );
	}

	bool AssetRegistry::IsAssetLoaded( AssetID id ) const
	{
		return m_LoadedAssets.contains( id );
	}

	size_t AssetRegistry::GetSize() const
	{
		return m_Assets.size();
	}

	void AssetRegistry::AddAsset( AssetID id )
	{
		// Rare chance of this ever happening as there are 2^64 random numbers that can be generated!
		// So chance of collision is 1/2^64
		SAT_CORE_ASSERT( m_Assets.find( id ) == m_Assets.end(), "Asset already exists!" );

		m_Assets[ id ] = Ref<Asset>::Create();
		m_Assets[ id ]->ID = id;
	}
}
