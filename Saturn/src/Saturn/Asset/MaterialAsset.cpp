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

#include "sppch.h"
#include "MaterialAsset.h"

#include "AssetManager.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Vulkan/Renderer.h"
#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/Project/Project.h"

#include "TextureSourceAsset.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/MaterialAssetViewer/MaterialAssetViewer.h"
#endif

namespace Saturn {

	MaterialAsset::MaterialAsset( Ref<Material> material )
	{
		if( material == nullptr )
		{
			m_Material = Ref<Material>::Create( ShaderLibrary::Get().Find( "shader_new" ), "New Material" );
			Default();
		}
		else
			m_Material->Copy( material );
	}

	MaterialAsset::MaterialAsset( const Ref<Asset>& rBase, Ref<Material> material )
		: Asset( rBase )
	{
		if( material == nullptr )
		{
			m_Material = Ref<Material>::Create( ShaderLibrary::Get().Find( "shader_new" ), "New Material" );
			Default();
		}
		else
			m_Material->Copy( material );
	}

	void MaterialAsset::Default()
	{
		if( m_Material == nullptr )
			return;

		m_Material->SetResource( "u_AlbedoTexture", Renderer::Get()->GetPinkTexture() );
		m_Material->SetResource( "u_NormalTexture", Renderer::Get()->GetPinkTexture() );
		m_Material->SetResource( "u_MetallicTexture", Renderer::Get()->GetPinkTexture() );
		m_Material->SetResource( "u_RoughnessTexture", Renderer::Get()->GetPinkTexture() );

		m_Material->SetPC<glm::vec3>( "u_Materials.AlbedoColor", { 1.0f, 1.0f, 1.0f } );
		m_Material->SetPC<float>( "u_Materials.Metalness", 1.0f );
		m_Material->SetPC<float>( "u_Materials.Roughness", 1.0f );
		m_Material->SetPC<float>( "u_Materials.UseNormalMap", 0.0f );
		m_Material->SetPC<float>( "u_Materials.Emissive", 0.0f );
	}

	MaterialAsset::~MaterialAsset()
	{
		m_TextureCache.clear();
		m_PendingTextureChanges.clear();

		m_Material = nullptr;
	}

#if !defined(SAT_DIST)
	void MaterialAsset::OnAssetDependencyReplace( AssetID oldID, AssetID newID )
	{
		SAT_CORE_ASSERT( m_Material );

		Ref<MaterialAssetViewer> assetViewer = Ref<MaterialAssetViewer>::Create( ID );
		assetViewer->HandleAssetDependencyReplace( oldID, newID );
	}
#endif

	bool MaterialAsset::CanPurge() const
	{
		// We can purge if all our textures have a ref-count of 2.
		return true;
	}

	Saturn::Ref<Saturn::Texture2D> MaterialAsset::GetAlbeoMap() const
	{
		return m_Material->GetResource( "u_AlbedoTexture" );
	}

	Saturn::Ref<Saturn::Texture2D> MaterialAsset::GetNormalMap() const
	{
		return m_Material->GetResource( "u_NormalTexture" );
	}

	Saturn::Ref<Saturn::Texture2D> MaterialAsset::GetMetallicMap() const
	{
		return m_Material->GetResource( "u_MetallicTexture" );
	}

	Saturn::Ref<Saturn::Texture2D> MaterialAsset::GetRoughnessMap() const
	{
		return m_Material->GetResource( "u_RoughnessTexture" );
	}

	glm::vec3 MaterialAsset::GetAlbeoColor()
	{
		return m_Material->Get<glm::vec3>( "u_Materials.AlbedoColor" );
	}

	void MaterialAsset::SetAlbeoColor( const glm::vec3& color )
	{
		MarkDirty();

		m_Material->SetPC<glm::vec3>( "u_Materials.AlbedoColor", color );
	}

	void MaterialAsset::UseNormalMap( bool val )
	{
		MarkDirty();

		m_Material->SetPC<float>( "u_Materials.UseNormalMap", val );
	}

	void MaterialAsset::SetRoughness( float val )
	{
		MarkDirty();

		m_Material->SetPC<float>( "u_Materials.Roughness", val );
	}

	void MaterialAsset::SetMetalness( float val )
	{
		MarkDirty();

		m_Material->SetPC<float>( "u_Materials.Metalness", val );
	}

	void MaterialAsset::SetEmissive( float val )
	{
		MarkDirty();

		m_Material->SetPC<float>( "u_Materials.Emissive", val );
	}

	Saturn::Ref<Saturn::Texture2D> MaterialAsset::GetResource( const std::string& rName )
	{
		return m_Material->GetResource( rName );
	}

	void MaterialAsset::SetResource( const std::string& rName, const Ref<Texture2D>& rTexture )
	{
		MarkDirty();

		m_PendingTextureChanges[ rName ] = rTexture;
	}

	void MaterialAsset::RT_Bind( const std::vector<std::vector<VkWriteDescriptorSet>>& rStorageBufferWDS )
	{
		m_Material->RT_Update();
	}

	void MaterialAsset::RT_Reset()
	{
		// We don't want to default the texture because what if the user has only changed the normal map. And we'd be reseting all of the textures.
		m_TextureCache.clear();
		m_PendingTextureChanges.clear();

		RenderThread::Get().Queue( [ this ]()
		{
			m_Material->SetResource( "u_AlbedoTexture", Renderer::Get()->GetPinkTexture() );
			m_Material->SetResource( "u_NormalTexture", Renderer::Get()->GetPinkTexture() );
			m_Material->SetResource( "u_MetallicTexture", Renderer::Get()->GetPinkTexture() );
			m_Material->SetResource( "u_RoughnessTexture", Renderer::Get()->GetPinkTexture() );
			
			// No longer using texture source assets.
			m_TextureSourceAssets.fill( nullptr );
		} );
	}

	void MaterialAsset::RT_Update( const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds )
	{
		// Update textures.
		for( auto& [name, texture] : m_PendingTextureChanges )
		{
			if( m_TextureCache[ name ].imageView == texture->GetDescriptorInfo().imageView )
			{
				continue;
			}

			// Does not exists, add and update.
			m_TextureCache[ name ] = texture->GetDescriptorInfo();
			m_Material->SetResource( name, texture );
		}

		if( m_PendingTextureChanges.size() )
			m_PendingTextureChanges.clear();

		const auto frame = Renderer::Get()->GetCurrentFrame();
		for( const auto& rWds : rExtraWds[ frame ] )
		{
			m_Material->PushExternalWds( rWds );
		}

		m_Material->RT_Update();

#if !defined( SAT_DIST )
		if( m_ValuesChanged ) 
		{
			m_ValuesChanged = false;
		}
#endif
	}

	void MaterialAsset::RT_ApplyChanges()
	{
		/*
		RenderThread::Get().Queue( [ this ]()
		{
			// Load texture (auto assume we have not loaded them).
			Ref<Texture2D> texture = nullptr;

			std::unordered_map<uint32_t, std::string> IndexToTextureIndex =
			{
				{ 0, "u_AlbedoTexture" },
				{ 1, "u_NormalTexture" },
				{ 2, "u_MetallicTexture" },
				{ 3, "u_RoughnessTexture" }
			};

			for( auto&& [index, path] : m_VPendingTextureChanges )
			{
				const auto fullPath = Project::GetActiveProject()->FilepathAbs( path );
				texture = Ref<Texture2D>::Create( fullPath, AddressingMode::Repeat, false );

				m_Material->SetResource( IndexToTextureIndex[ index ], texture );
			}

			UseNormalMap( m_Material->GetResource( "u_NormalTexture" ) != Renderer::Get()->GetPinkTexture() );

			m_ValuesChanged = false;

			// Save the material
			MaterialAssetSerialiser mas;
			mas.Serialise( this );
		} );
		*/
	}

	bool MaterialAsset::IsUsingNormalMap()
	{
		return ( bool ) m_Material->Get<float>( "u_Materials.UseNormalMap" );
	}

	float MaterialAsset::GetRoughness()
	{
		return m_Material->Get<float>( "u_Materials.Roughness" );
	}

	float MaterialAsset::GetMetalness()
	{
		return m_Material->Get<float>( "u_Materials.Metalness" );
	}

	float MaterialAsset::GetEmissive()
	{
		return m_Material->Get<float>( "u_Materials.Emissive" );
	}

	void MaterialAsset::SetAlbeoMap( const Ref<TextureSourceAsset> texture )
	{
		MarkDirty();

		m_PendingTextureChanges[ "u_AlbedoTexture" ] = texture->GetTexture();
		m_TextureSourceAssets[ 0 ] = texture;
	}

	void MaterialAsset::SetNormalMap( const Ref<TextureSourceAsset> texture )
	{
		MarkDirty();

		m_PendingTextureChanges[ "u_NormalTexture" ] = texture->GetTexture();
		m_TextureSourceAssets[ 1 ] = texture;
	}

	void MaterialAsset::SetMetallicMap( const Ref<TextureSourceAsset> texture )
	{
		MarkDirty();

		m_PendingTextureChanges[ "u_MetallicTexture" ] = texture->GetTexture();
		m_TextureSourceAssets[ 2 ] = texture;
	}

	void MaterialAsset::SetRoughnessMap( const Ref<TextureSourceAsset> texture )
	{
		MarkDirty();

		m_PendingTextureChanges[ "u_RoughnessTexture" ] = texture->GetTexture();
		m_TextureSourceAssets[ 3 ] = texture;
	}

	void MaterialAsset::ForceUpdate()
	{
		for( auto& [name, texture] : m_PendingTextureChanges )
		{
			if( m_TextureCache[ name ].imageView == texture->GetDescriptorInfo().imageView )
			{
				continue;
			}

			// Does not exists, add and update
			m_TextureCache[ name ] = texture->GetDescriptorInfo();
			m_Material->SetResource( name, texture );
		}

		m_PendingTextureChanges.clear();
	}

	void MaterialAsset::SetAlbeoMap( UUID AssetID )
	{
		if( AssetID == 0 )
		{
			m_PendingTextureChanges[ "u_AlbedoTexture" ] = Renderer::Get()->GetPinkTexture();
			m_TextureSourceAssets[ 0 ] = nullptr;
		}
		else
		{
			Ref<TextureSourceAsset> sourceAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( AssetID );
			m_PendingTextureChanges[ "u_AlbedoTexture" ] = sourceAsset->GetTexture();
		}
	}

	void MaterialAsset::SetNormalMap( UUID AssetID )
	{
		if( AssetID == 0 )
		{
			m_PendingTextureChanges[ "u_NormalTexture" ] = Renderer::Get()->GetPinkTexture();
			m_TextureSourceAssets[ 1 ] = nullptr;
		}
		else
		{
			Ref<TextureSourceAsset> sourceAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( AssetID );
			m_PendingTextureChanges[ "u_NormalTexture" ] = sourceAsset->GetTexture();
			m_TextureSourceAssets[ 1 ] = sourceAsset;
		}
	}

	void MaterialAsset::SetMetallicMap( UUID AssetID )
	{
		if( AssetID == 0 )
		{
			m_PendingTextureChanges[ "u_MetallicTexture" ] = Renderer::Get()->GetPinkTexture();
			m_TextureSourceAssets[ 2 ] = nullptr;
		}
		else
		{
			Ref<TextureSourceAsset> sourceAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( AssetID );
			m_PendingTextureChanges[ "u_MetallicTexture" ] = sourceAsset->GetTexture();
			m_TextureSourceAssets[ 2 ] = sourceAsset;
		}
	}

	void MaterialAsset::SetRoughnessMap( UUID AssetID )
	{
		if( AssetID == 0 )
		{
			m_PendingTextureChanges[ "u_RoughnessTexture" ] = Renderer::Get()->GetPinkTexture();
			m_TextureSourceAssets[ 3 ] = nullptr;
		}
		else
		{
			Ref<TextureSourceAsset> sourceAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( AssetID );
			m_PendingTextureChanges[ "u_RoughnessTexture" ] = sourceAsset->GetTexture();
			m_TextureSourceAssets[ 3 ] = sourceAsset;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL REGISTRY
	//////////////////////////////////////////////////////////////////////////

	MaterialRegistry::MaterialRegistry()
	{
	}

	MaterialRegistry::MaterialRegistry( const Ref<StaticMesh>& mesh )
	{
		Copy( mesh->GetMaterialRegistry() );
	}

	MaterialRegistry::MaterialRegistry( const Ref<SkeletalMesh>& mesh )
	{
		Copy( mesh->GetMaterialRegistry() );
	}

	MaterialRegistry::~MaterialRegistry()
	{
		m_Materials.clear();
	}

	void MaterialRegistry::Copy( const Ref<MaterialRegistry>& rSrc )
	{
		m_Materials.clear();

		m_Materials = rSrc->m_Materials;
		m_HasOverridden = rSrc->m_HasOverridden;
	}

	void MaterialRegistry::AddAsset( uint32_t index )
	{
		m_Materials[ index ] = nullptr;
	}

	void MaterialRegistry::AddAsset( const Ref<MaterialAsset>& rAsset )
	{
		m_Materials.push_back( rAsset );
		m_HasOverridden.push_back( false );
	}

	Saturn::Ref<Saturn::MaterialAsset> MaterialRegistry::GetAsset( AssetID id )
	{
		return m_Materials.at( id );
	}

	void MaterialRegistry::SetMaterial( uint32_t index, AssetID id )
	{
		m_HasOverridden[ index ] = true;
		m_Materials[ index ] = AssetManager::Get()->GetAssetAs<MaterialAsset>( id );
	}

	void MaterialRegistry::SetMaterial( uint32_t index, Ref<MaterialAsset> material )
	{
		m_HasOverridden[ index ] = false;
		m_Materials[ index ] = material;
	}

	void MaterialRegistry::SetMaterialNoOvr( uint32_t index, AssetID id )
	{
//		AddTargetMaterialAsset( index, id );

		m_HasOverridden[ index ] = false;
		m_Materials[ index ] = AssetManager::Get()->GetAssetAs<MaterialAsset>( id );
	}

	void MaterialRegistry::ResetMaterial( uint32_t index, Ref<MaterialRegistry> srcRegistry )
	{
		m_HasOverridden[ index ] = false;
		m_Materials[ index ] = srcRegistry->GetMaterialAssets()[ index ];
	}

	bool MaterialRegistry::HasAnyOverrides() const
	{
		return std::any_of( m_HasOverridden.begin(), m_HasOverridden.end(), []( bool x ) { return x; } );
	}

	void MaterialRegistry::Serialise( const MaterialRegistry& rRegistry, std::ofstream& rStream )
	{
		RawSerialisation::WriteVector( rRegistry.m_HasOverridden, rStream );

		size_t mapSize = rRegistry.m_Materials.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( const auto& rMaterial : rRegistry.m_Materials )
		{
			if( rMaterial )
				RawSerialisation::WriteObject( rMaterial->ID, rStream );
			else
				RawSerialisation::WriteObject( UUID( 0 ), rStream );
		}
	}

	void MaterialRegistry::Serialise( const Ref<MaterialRegistry>& rRegistry, std::ofstream& rStream )
	{
		RawSerialisation::WriteVector( rRegistry->m_HasOverridden, rStream );
		
		size_t mapSize = rRegistry->m_Materials.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( const auto& rMaterial : rRegistry->m_Materials )
		{
			if( rMaterial )
				RawSerialisation::WriteObject( rMaterial->ID, rStream );
			else
				RawSerialisation::WriteObject( UUID( 0 ), rStream );
		}
	}

	void MaterialRegistry::Deserialise( Ref<MaterialRegistry>& rRegistry, std::istream& rStream )
	{
		RawSerialisation::ReadVector( rRegistry->m_HasOverridden, rStream );

		size_t mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		rRegistry->m_Materials.reserve( mapSize );

		for( size_t i = 0; i < mapSize; ++i )
		{
			UUID MaterialID;
			RawSerialisation::ReadObject( MaterialID, rStream );

			// Load material asset
			// Will call RawMaterialAssetSerialiser
			Ref<MaterialAsset> asset = AssetManager::Get()->GetAssetAs<MaterialAsset>( MaterialID );
			
			if( asset )
			{
				rRegistry->AddAsset( asset );
			}
			else // Fallback to default
			{
				const auto defaultProjectAsset = AssetManager::Get()->FindAsset( Project::GetActiveProject()->GetDefaultMaterialAsset() );

				rRegistry->AddAsset( Ref<MaterialAsset>::Create( nullptr ) );
			}
		}
	}
}
