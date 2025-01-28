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

#include "Saturn/Vulkan/Material.h"

#include "Asset.h"

namespace Saturn {

	class StaticMesh;

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset( Ref<Material> material );
		virtual ~MaterialAsset();

		// Texture
		Ref<Texture2D> GetAlbeoMap();
		Ref<Texture2D> GetNormalMap();
		Ref<Texture2D> GetMetallicMap();
		Ref<Texture2D> GetRoughnessMap();

		// Colors and values

		glm::vec3 GetAlbeoColor();
		float IsUsingNormalMap();
		float GetRoughness();
		float GetMetalness();
		float GetEmissive();

		std::string& GetName() { return m_Material->GetName(); }
		const std::string& GetName() const { return m_Material->GetName(); }

		void SetAlbeoMap( Ref<Texture2D>& rTexture );
		void SetNormalMap( Ref<Texture2D>& rTexture );
		void SetMetallicMap( Ref<Texture2D>& rTexture );
		void SetRoughnessMap( Ref<Texture2D>& rTexture );

		void SetAlbeoColor( glm::vec3 color );
		void UseNormalMap( bool val );
		void SetRoughness( float val );
		void SetMetalness( float val );
		void SetEmissive( float val );

		Ref<Texture2D> GetResource( const std::string& rName );
		void SetResource( const std::string& rName, const Ref<Texture2D>& rTexture );

		template<typename Ty>
		Ty& Get( const std::string& rName ) 
		{
			return m_Material->Get< Ty >( rName );
		}

		template<typename Ty>
		void Set( const std::string& rName, const Ty& rValue )
		{
			m_ValuesChanged = true;

			m_Material->Set( rName, rValue );
		}
		
		void Reset();

		// Updates Uniform buffers, texture and storage buffers.
		void Bind( const Ref< StaticMesh >& rMesh, Submesh& rSubmsh, Ref< Shader >& Shader, const VkWriteDescriptorSet& rStorageBufferWDS = VkWriteDescriptorSet{} );

		void RT_Bind( const std::vector<std::vector<VkWriteDescriptorSet>>& rStorageBufferWDS = std::vector<std::vector<VkWriteDescriptorSet>>() );

		void Clean();

		Buffer GetPushConstantData() { return m_Material->m_PushConstantData; }

		Ref<Material> GetMaterial() const { return m_Material; }

		void ApplyChanges();
		void SetMaterial( const Ref<Material> material );

		void SetName( const std::string& rName ) { return m_Material->SetName( rName ); }

	private:
		void Default();

	private:

		// Used by MaterialAssetSerialiser & Material asset viewer (node editor)
		void SetAlbeoMap( const std::filesystem::path& rPath );
		void SetNormalMap( const std::filesystem::path& rPath );
		void SetMetallicMap( const std::filesystem::path& rPath );
		void SetRoughnessMap( const std::filesystem::path& rPath );

		// Used by RawMaterialAssetSerialiser
		void SetAlbeoMap( UUID AssetID );
		void SetNormalMap( UUID AssetID );
		void SetMetallicMap( UUID AssetID );
		void SetRoughnessMap( UUID AssetID );

		void ForceUpdate();

		inline void MarkDirty() { m_ValuesChanged = true; }

	private:
		Ref<Material> m_Material = nullptr;

		bool m_ValuesChanged = false;

		std::unordered_map< std::string, VkDescriptorImageInfo > m_TextureCache;

		Ref<Material> m_PendingMaterialChange = nullptr;

		std::unordered_map< std::string, Ref<Texture2D> > m_PendingTextureChanges;
		std::unordered_map< uint32_t, std::filesystem::path > m_VPendingTextureChanges;
	private:
		friend class MaterialNodeEditorEvaluator;
		friend class MaterialOutputNode;
		friend class MaterialAssetViewer;
		friend class MaterialAssetSerialiser;
		friend class RawMaterialAssetSerialiser;
	};

	class MaterialRegistry : public RefTarget
	{
	public:
		MaterialRegistry();
		// TODO: When we have animated meshes this will need to be re-worked.
		MaterialRegistry( const Ref<StaticMesh>& mesh );

		~MaterialRegistry();

		void Copy( const Ref<MaterialRegistry>& rSrc );

		void AddAsset( uint32_t index );
		void AddAsset( const Ref<MaterialAsset>& rAsset );

		Ref<MaterialAsset> GetAsset( AssetID id );

		void SetMaterial( uint32_t index, AssetID id );
		void ResetMaterial( uint32_t index );
	
		std::vector< Ref<MaterialAsset> >& GetMaterialAssets() { return m_Materials; }
		const std::vector< Ref<MaterialAsset> >& GetMaterialAssets() const { return m_Materials; }

		UUID GetID() { return m_ID; }
		const UUID GetID() const { return m_ID; }

		bool HasOverrides( uint32_t index ) const { return m_HasOverridden[ index ]; }
		bool HasAnyOverrides();
		void SetOverries( uint32_t index, bool val ) { m_HasOverridden[ index ] = val; }

		// This does not copy the material registry and is only to be used by the scene serialiser.
		void SetMesh( Ref<StaticMesh> mesh ) { m_Mesh = mesh; }

	public:
		static void Serialise( const MaterialRegistry& rRegistry, std::ofstream& rStream );
		static void Serialise( const Ref<MaterialRegistry>& rRegistry, std::ofstream& rStream );
		
		static void Deserialise( Ref<MaterialRegistry>& rRegistry, std::istream& rStream );

	private:
		void SetMaterial( uint32_t index, Ref<MaterialAsset> material );

	private:
		Ref<StaticMesh> m_Mesh = nullptr;
		std::vector< Ref<MaterialAsset> > m_Materials;

		// This may not be the best way.
		std::vector<bool> m_HasOverridden;

		// We want to keep an ID so this is unique to any other material registry.
		// But really it's because I want to use in the hash function for StaticMeshKey.
		UUID m_ID;
	private:
		friend class MaterialAsset;
		friend class StaticMesh;
		friend class StaticMeshAssetSerialiser;
	};
}