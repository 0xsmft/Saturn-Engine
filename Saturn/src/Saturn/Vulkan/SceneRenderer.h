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

#include "SceneRendererFlags.h"
#include "Saturn/Scene/Scene.h"
#include "Saturn/Scene/Entity.h"
#include "Mesh.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Serialisation/Raw/ImageFileAux.h"

#include "Renderer.h"
#include "EnvironmentMap.h"
#include "DescriptorSet.h"
#include "Framebuffer.h"
#include "ComputePipeline.h"
#include "StorageBufferSet.h"
#include "UniformBufferSet.h"

#include "Pipeline.h"

constexpr int SHADOW_CASCADE_COUNT = 4;
constexpr int MAX_POINT_LIGHTS = 512;

namespace Saturn {

	struct DrawCommand
	{
		Ref< StaticMesh > Mesh = nullptr;
		uint32_t SubmeshIndex = 0;
		uint32_t Instances = 0;
		uint32_t InstanceOffset = 0;
	};

	struct SelectedDrawCommand
	{
		Ref< StaticMesh > Mesh = nullptr;
		uint32_t SubmeshIndex = 0;
		uint32_t TransfromBufferOffset = 0;
	};

	struct DynamicDrawCommand
	{
		Ref<SkeletalMesh> Mesh = nullptr;
		uint32_t SubmeshIndex = 0;
		uint32_t Instances = 0;
	};

	struct ShadowCascade
	{
		Ref< Framebuffer > Framebuffer = nullptr;

		float SplitDepth = 0.0f;
		glm::mat4 ViewProjection{};
	};

	// Most of theses structs MUST (most of the time) match the structs in the shader.
	// DirLight
	struct DirLight
	{
		glm::vec3 Direction{};
		float Padding = 0.0f;
		glm::vec3 Radiance{};
		float Multiplier = 0.0f;
	};

	enum class BloomStage : uint8_t
	{
		Prefilter,
		Downsample,
		FirstUpsample,
		Upsample
	};

	enum class AOTechnique : uint8_t
	{
		None,

		// Screen Space AO
		SSAO,

		// Horizon Based AO+
		HBAO,
	};

	struct StaticMeshKey
	{
		AssetID MeshID = 0;
		Ref<MaterialRegistry> Registry;
		uint32_t SubmeshIndex;
		bool SelectedKey = false;

		StaticMeshKey() = default;
		StaticMeshKey( AssetID meshID, Ref<MaterialRegistry> materialReg, uint32_t submeshIndex, bool selected = false ) : MeshID( meshID ), SubmeshIndex( submeshIndex ), SelectedKey( selected ) { Registry = materialReg; }

		bool operator==( const StaticMeshKey& rKey )
		{
			return ( MeshID == rKey.MeshID && Registry == rKey.Registry && SubmeshIndex == rKey.SubmeshIndex && rKey.SelectedKey == SelectedKey );
		}

		bool operator==( const StaticMeshKey& rKey ) const
		{
			return ( MeshID == rKey.MeshID && Registry == rKey.Registry && SubmeshIndex == rKey.SubmeshIndex && rKey.SelectedKey == SelectedKey );
		}
	};

	// Data that gets sent to the vertex shader
	struct TransformBufferData
	{
		glm::vec4 TransfromBufferR[ 4 ];
	};

	// For each mesh, what offset are we and how much transform does it have.
	struct TransformBuffer
	{
		uint32_t Offset = 0;
		std::vector<TransformBufferData> Data;
	};

	// For each mesh, what offset are we and how much transform does it have.
	struct BoneTransformBuffer
	{
		uint32_t Offset = 0;
		uint32_t Stride = 0;
		std::vector<glm::mat4> Data;
	};

	struct SubmeshTransformVB
	{
		Ref<VertexBuffer> VertexBuffer;
		TransformBufferData* pData = nullptr;
	};
}

namespace std {

	template<>
	struct hash< Saturn::StaticMeshKey >
	{
		size_t operator()( const Saturn::StaticMeshKey& rKey ) const
		{
			return rKey.Registry->GetID() ^ rKey.MeshID ^ rKey.SubmeshIndex;
		}
	};
}

namespace Saturn {

	struct UBGridMatrices
	{
		glm::mat4 ViewProjection;

		glm::mat4 Transform;

		float Scale;
		float Res;
	};

	struct UBSkyboxMatrices
	{
		glm::mat4 InverseVP;
	};

	// Vertex, Binding 0
	struct UBStaticMeshMatrices
	{
		glm::mat4 ViewProjection;
		glm::mat4 View;
	};

	struct PCStaticMeshMaterial
	{
		alignas( 4 ) float UseAlbedoTexture;
		alignas( 4 ) float UseMetallicTexture;
		alignas( 4 ) float UseRoughnessTexture;
		alignas( 4 ) float UseNormalTexture;

		alignas( 16 ) glm::vec4 AlbedoColor;
		alignas( 4 ) float Metalness;
		alignas( 4 ) float Roughness;
	};

	// Fragment, Binding 13
	struct UBPointLights
	{
		uint32_t nbLights = 0;
		PointLight Lights[ 512 ]{};
	};

	// Vertex, Binding 1
	struct UBLightData
	{
		glm::mat4 LightMatrix[ 4 ];
	};

	// Fragment, Binding 2
	struct UBSceneData
	{
		DirLight Lights;
		glm::vec3 CameraPosition;
	};

	// Fragment, Binding 3
	struct UBShadowData
	{
		glm::vec4 CascadeSplits;
	};

	struct RendererData
	{
		void Terminate();
		
		//////////////////////////////////////////////////////////////////////////
		// COMMAND POOLS & BUFFERS
		//////////////////////////////////////////////////////////////////////////
		
		VkCommandBuffer CommandBuffer = nullptr;
		
		//////////////////////////////////////////////////////////////////////////

		RendererCamera CurrentCamera;

		//////////////////////////////////////////////////////////////////////////
		
		bool IsSwapchainTarget = false;
		bool Resized = false;
		bool SSAONoiseGenerated = false;
		bool EnableShadows = true;

		uint32_t Width = 0;
		uint32_t Height = 0;

		glm::vec2 ViewportPos{};
		uint32_t FrameCount = 0;

		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// TIMERS
		//////////////////////////////////////////////////////////////////////////
	
		Timer GeometryPassTimer;
		Timer ShadowMapTimers[SHADOW_CASCADE_COUNT];
		Timer PreDepthTimer;
		Timer LightCullingTimer;
		Timer BloomTimer;
		Timer SceneCompPPTimer;

		//////////////////////////////////////////////////////////////////////////
		Ref<StorageBufferSet> SBBoneTransforms;
		Ref<StorageBufferSet> StorageBufferSet;
		Ref<UniformBufferSet> UniformBufferSet;

		glm::mat4* BoneTransformData = nullptr;

		//////////////////////////////////////////////////////////////////////////
		// Quad Vertex and Index buffers
		Ref<VertexBuffer> QuadVertexBuffer = nullptr;
		Ref<IndexBuffer> QuadIndexBuffer = nullptr;

		// DirShadowMap
		//////////////////////////////////////////////////////////////////////////

		std::vector< Ref< Pass > > DirShadowMapPasses;
		std::vector< Ref< Pipeline > > DirShadowMapPipelines;
		std::vector< Ref< Pipeline > > DirShadowMapDynamicPipelines;
		Ref<Material> DirShadowMapMaterial;
		Ref<Material> DirShadowMapDynamicMaterialSet2;

		float CascadeSplitLambda = 0.92f;
		float CascadeFarPlaneOffset = 100.0f;
		float CascadeNearPlaneOffset = -150.0f;
		
		std::vector< ShadowCascade > ShadowCascades;

		// PreDepth + Light culling
		//////////////////////////////////////////////////////////////////////////

		Ref<Pass> PreDepthPass = nullptr;
		Ref<Pipeline> PreDepthPipeline = nullptr;
		Ref<Pipeline> PreDepthDynamicPipeline = nullptr;
		Ref<Framebuffer> PreDepthFramebuffer = nullptr;
		Ref<Material> PreDepthMaterial = nullptr;
		Ref<Material> PreDepthDynamicMaterial = nullptr;
		Ref<Material> PreDepthDynamicMaterialSet2 = nullptr;

		Ref<ComputePipeline> LightCullingPipeline = nullptr;
		Ref<Material> LightCullingMaterial = nullptr;
		glm::vec3 LightCullingWorkGroups{};

		// Geometry
		//////////////////////////////////////////////////////////////////////////

		// Render pass for all grid, skybox and meshes.
		Ref<Pass> GeometryPass = nullptr;
		Ref<Framebuffer> GeometryFramebuffer = nullptr;

		// STATIC MESHES

		// Main geometry for static meshes.
		Ref<Pipeline> StaticMeshPipeline;
		Ref<Material> StaticMeshMaterial;
	
		// DYNAMIC MESHES

		// Main geometry for dynamic meshes.
		Ref<Pipeline> DynamicMeshPipeline;
		Ref<Material> DynamicMeshMaterial;

		// GRID
		Ref<Pipeline> GridPipeline = nullptr;
		Ref<Material> GridMaterial = nullptr;

		// SKYBOX
		Ref<EnvironmentMap> SceneEnvironment = nullptr;

		Ref<Pipeline> SkyboxPipeline = nullptr;
		Ref<Material> SkyboxMaterial = nullptr;

		Ref<Material> PreethamMaterial = nullptr;

		float SkyboxLod = 0.0f;
		float Intensity = 1.0f;

		//////////////////////////////////////////////////////////////////////////
		
		// End Geometry
		
		//////////////////////////////////////////////////////////////////////////

		// Begin AO
		Ref<Pipeline>	 SSAOPipeline   = nullptr;
		Ref<Pass>		 SSAORenderPass = nullptr;
		Ref<Material>	 SSAOMaterial   = nullptr;
		Ref<Framebuffer> SSAOFramebuffer = nullptr;
		Ref<Texture2D>	 SSAONoiseImage = nullptr;

		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////

		// Begin Scene Composite
		
		Ref<Pass> SceneComposite = nullptr;
		Ref< Framebuffer > SceneCompositeFramebuffer = nullptr;

		Ref<Pipeline> SceneCompositePipeline = nullptr;
		
		// Input
		Ref<Material> SceneCompositeMaterial = nullptr;
		
		// Texture pass
		//////////////////////////////////////////////////////////////////////////
		Ref<Pass> TexturePass = nullptr;
		Ref<Pipeline> TexturePassPipeline = nullptr;
		// Input
		Ref<Material> TexturePassMaterial = nullptr;

		//////////////////////////////////////////////////////////////////////////
		// End Scene Composite
		//////////////////////////////////////////////////////////////////////////

		// Selected Geometry
		//////////////////////////////////////////////////////////////////////////

		Ref<Pass> SelectedGeometryPass = nullptr;
		Ref<Framebuffer> SelectedGeometryFramebuffer = nullptr;
		Ref<Pipeline> SelectedGeometryPipeline = nullptr;
		Ref<Material> SelectedGeometryMaterial = nullptr;

		// Jumpflooding
		//////////////////////////////////////////////////////////////////////////

		Ref<Framebuffer> JumpFloodFirstPassFB = nullptr;
		Ref<Pass> JumpFloodFirstPass = nullptr;
		Ref<Material> JumpFloodFirstMaterial = nullptr;
		Ref<Pipeline> JumpFloodFirstPipeline = nullptr;

		Ref<Pass> JumpFloodEvenPass = nullptr;
		Ref<Framebuffer> JumpFloodEvenFB = nullptr;
		Ref<Material> JumpFloodEvenMaterial = nullptr;
		Ref<Pipeline> JumpFloodEvenPipeline = nullptr;

		Ref<Framebuffer> JumpFloodOddFB = nullptr;
		Ref<Pass> JumpFloodOddPass = nullptr;
		Ref<Material> JumpFloodOddMaterial = nullptr;
		Ref<Pipeline> JumpFloodOddPipeline = nullptr;

		// Bloom
		//////////////////////////////////////////////////////////////////////////

		struct BloomTexture
		{
			Ref<Texture2D> Texture;
			std::vector<VkDescriptorImageInfo> ImageInfos;
		};

		std::array<BloomTexture, 3> BloomTextures;
		Ref<ComputePipeline> BloomComputePipeline = nullptr;
		Ref<Texture2D> BloomDirtTexture = nullptr;
		Ref< DescriptorSet > BloomDS = nullptr;

		Ref<Material> BloomPrefilterMaterial;
		Ref<Material> BloomFirstUpsampleMaterial;

		std::vector<Ref<Material>> BloomDownsampleAMaterials;
		std::vector<Ref<Material>> BloomDownsampleBMaterials;
		std::vector<Ref<Material>> BloomUpsampleMaterials;

		uint32_t BloomWorkSize = 4;

		// The value of a pixel component before it's considered to be an emissive object.
		float BloomThreshold = 1.5f;
		float BloomDirtIntensity = 20.0f;

		// BDRF Lut
		//////////////////////////////////////////////////////////////////////////
		Ref<Texture2D> BRDFLUT_Texture = nullptr;

		// Late Composite
		//////////////////////////////////////////////////////////////////////////
		Ref<Pass> LateCompositePass = nullptr;
		Ref<Framebuffer> LateCompositeFramebuffer = nullptr;

		// Physics Outline
		//////////////////////////////////////////////////////////////////////////
		Ref<Pipeline> PhysicsOutlinePipeline = nullptr;
		Ref<Material> PhysicsOutlineMaterial = nullptr;

		// Instanced Rendering
		//////////////////////////////////////////////////////////////////////////
		// 		
		// MESH ID -> TRANSFORMS
		std::unordered_map< StaticMeshKey, TransformBuffer > MeshTransforms;

		std::unordered_map< StaticMeshKey, BoneTransformBuffer > BoneTransformMap;

		// This holds the entire transform data for each submesh, per frame in flight.
		std::vector< SubmeshTransformVB > SubmeshTransformData;

		//////////////////////////////////////////////////////////////////////////
		// SHADERS

		Ref< Shader > GridShader = nullptr;
		Ref< Shader > SkyboxShader = nullptr;
		Ref< Shader > PreethamShader = nullptr;
		Ref< Shader > StaticMeshShader = nullptr;
		Ref< Shader > DynamicMeshShader = nullptr;
		Ref< Shader > SceneCompositeShader = nullptr;
		Ref< Shader > TexturePassShader = nullptr;
		Ref< Shader > DirShadowMapShader = nullptr;
		Ref< Shader > DirShadowMapDynamicShader = nullptr;
		Ref< Shader > AOShader = nullptr;
		Ref< Shader > AOCompositeShader = nullptr;
		Ref< Shader > PreDepthShader = nullptr;
		Ref< Shader > PreDepthDynamicShader = nullptr;
		Ref< Shader > LightCullingShader = nullptr;
		Ref< Shader > BloomShader = nullptr;
		Ref< Shader > PhysicsOutlineShader = nullptr;
		Ref< Shader > SelectionShader = nullptr;
		Ref< Shader > JmpFloodFirstShader = nullptr;
		Ref< Shader > JmpFloodEvenShader = nullptr;
		Ref< Shader > JmpFloodOddShader = nullptr;
	};

	class Renderer2D;
	class AluraRenderer;

	class SceneRenderer : public RefTarget
	{
		using ScheduledFunc = std::function<void()>;
	public:
		SceneRenderer() = default;
		SceneRenderer( SceneRendererFlags flags );
		~SceneRenderer() { Terminate(); }

		void ImGuiRender();

		void SetCurrentScene( Scene* pScene );
#if !defined(SAT_DIST)
		Scene* GetCurrentScene() const { return m_pScene; }
#endif

		void SubmitStaticMesh( SharedPtr<Entity> entity, Ref<StaticMesh> mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SubmitDynamicMesh( SharedPtr<Entity> entity, Ref<SkeletalMesh> mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms );
		
		// NOTE: mesh is not the physical mesh in the entity, it is the phsyics mesh!
		void SubmitPhysicsCollider( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SubmitSelectedStaticMesh( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SetViewportSize( uint32_t w, uint32_t h );
		void SetViewportPosition( float x, float y ) { m_RendererData.ViewportPos = glm::vec2( x, y ); }

		void FlushDrawList();

		void Recreate();

		void PreRender();
		void RenderScene();

		void SetCamera( const RendererCamera& Camera );
		const RendererCamera& GetRendererCamera() const { return m_RendererData.CurrentCamera; }

		Ref<Pass> GetGeometryPass() { return m_RendererData.GeometryPass; }
		const Ref<Pass> GetGeometryPass() const { return m_RendererData.GeometryPass; }

		Ref<Pass> GetLateComposite() { return m_RendererData.LateCompositePass; }
		const Ref<Pass> GetLateComposite() const { return m_RendererData.LateCompositePass; }

		Ref<Framebuffer> GetLateCompositeFramebuffer() { return m_RendererData.LateCompositeFramebuffer; }
		const Ref<Framebuffer> GetLateCompositeFramebuffer() const { return m_RendererData.LateCompositeFramebuffer; }

		Ref<Image2D> CompositeImage();

		void SetDynamicSky( float Turbidity, float Azimuth, float Inclination );
		
		bool HasFlag( SceneRendererFlags flag ) const;

		void SetSwapchainTarget( bool target ) { m_RendererData.IsSwapchainTarget = target; }
		void ChangeAOTechnique( AOTechnique newTechique );

		AOTechnique GetAOTechnique() const { return m_AOTechnique; }

		uint32_t Width() const { return m_RendererData.Width; }
		uint32_t Height() const { return m_RendererData.Height; }

		void Screenshot( const std::filesystem::path& rPath, const glm::vec2& rSize = {} );

		void RenderSelectionNextFrame() { /*m_RendererData.PendingSelectionPass = true;*/ }

		Ref<Renderer2D> GetRenderer2D() const;
		Ref<AluraRenderer> GetAluraRenderer() const;

	private:
		void Init();
		void Terminate();

		void RenderGrid();
		void RenderSkybox();
		void CheckInvalidSkybox();

		void UpdateCascades( const glm::vec3& Direction );

		void CreateGridComponents();

		void CreateSkyboxComponents();

		void InitGeometryPass();
		void InitDirShadowMap();
		void InitPreDepth();
		void InitBloom();
		void InitSceneComposite();
		void InitLateComposite();
		void InitPhysicsOutline();
		void InitTexturePass();
		void InitSSAO();
		void InitHBAO();
		void InitSelectionPass();
		void InitJumpFlood();
		void InitJmpfFirstPass();
		void InitJmpfEvenPass();
		void InitJmpfOddPass();

		void InitBuffers();
		void InitRenderer2D();
		void InitAlura();

		void DirShadowMapPass();
		void PreDepthPass();
		void LightCullingPass();
		void GeometryPass();
		void BloomPass();
		void SSAOPass();
		void SelectedGeometryPass();
		void JumpFloodPass();
		void SceneCompositePass();
		void LateCompPhysicsOutline();
		void JumpFloodLatePass();
		void TexturePass();

		void RenderStaticMeshes();
		void RenderDynamicMeshes();

		void AddScheduledFunction( ScheduledFunc&& rrFunc );

#if !defined(SAT_DIST)
		void OnShaderReloaded( const std::string& rName );
#endif

		void CreateBloomMaterials();
		void SendBoneDataToMap( Ref<SkeletalMesh> mesh, const StaticMeshKey& rKey, const std::vector<glm::mat4>& rBoneTransforms );

		Ref<TextureCube> CreateDymanicSky();
	private:
		SceneRendererFlags m_Flags;
		AOTechnique m_AOTechnique = AOTechnique::None;

		RendererData m_RendererData{};
		Scene* m_pScene = nullptr;
		Ref<Renderer2D> m_Renderer2D;
		Ref<AluraRenderer> m_AluraRenderer;

		std::unordered_map< StaticMeshKey, DrawCommand > m_DrawList;
		std::unordered_map< StaticMeshKey, DrawCommand > m_ShadowMapDrawList;
		std::unordered_map< StaticMeshKey, DrawCommand > m_PhysicsColliderDrawList;
		std::unordered_map< StaticMeshKey, DrawCommand > m_SelectedStaticMeshDrawList;
		
		std::unordered_map< StaticMeshKey, DynamicDrawCommand > m_DynamicShadowMapDrawList;
		std::unordered_map< StaticMeshKey, DynamicDrawCommand > m_DynamicDrawList;

		//////////////////////////////////////////////////////////////////////////

		std::vector< ScheduledFunc > m_ScheduledFunctions;
		ScheduledFunc m_LightCullingFunction;

	private:
		friend class Scene;
		friend class VulkanContext;
	};
}