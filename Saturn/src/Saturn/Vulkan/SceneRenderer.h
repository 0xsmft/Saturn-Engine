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

#include "Renderer.h"
#include "EnvironmentMap.h"
#include "DescriptorSet.h"
#include "Framebuffer.h"
#include "ComputePipeline.h"
#include "StorageBufferSet.h"
#include "UniformBufferSet.h"
#include "Sampler.h"

#include "Pipeline.h"

constexpr uint32_t SHADOW_CASCADE_COUNT = 4u;
constexpr size_t MAX_POINT_LIGHTS = 32llu;

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

		// Ground-Truth AO
		GTAO,
	};

	// Anti-Aliasing techniques
	enum class AATechnique : uint8_t
	{
		None,

		SubpixelMorphological,

		FastApproximate,
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

	// Fragment, Binding 13
	struct UBPointLights
	{
		uint32_t nbLights = 0;
		PointLight Lights[ MAX_POINT_LIGHTS ]{};
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

	enum SceneCompositeFlags
	{
		SceneCompositeFlag_Default = 0,
		
		// Composite AO for GTAO
		SceneCompositeFlag_GTAO  = BIT( 0 ),

		// Do not composite bloom
		SceneCompositeFlag_NoBloom = BIT( 1 ),
	};

	class RendererData
	{
		RendererData( RendererData& ) = delete;
		RendererData& operator=( RendererData& ) = delete;
	public:
		RendererData() = default;
		~RendererData() = default;

		void Terminate();
		void ClearSSAOResources();
		void ClearHBAOResources();
		void ClearGTAOResources();

	public:
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
		bool EnableBloom = true;

		uint32_t Width = 0;
		uint32_t Height = 0;

		glm::vec2 ViewportPos{};
		uint32_t FrameCount = 0;

		//////////////////////////////////////////////////////////////////////////
		// Scene comp flags (placed here for better memory layout)

		uint32_t SceneCompositeFlags = SceneCompositeFlag_Default;

		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// TIMERS
		//////////////////////////////////////////////////////////////////////////
	
		Timer GeometryPassTimer;
		Timer ShadowMapTimers[SHADOW_CASCADE_COUNT];
		Timer PreDepthTimer;
		Timer LightCullingTimer;
		Timer BloomTimer;
		Timer SSAOTimer;
		Timer HBAOTimer;
		Timer GTAOTimer;
		Timer AOBlurTimer;
		Timer SMAAPassTimer;
		Timer SceneCompPPTimer;

		//////////////////////////////////////////////////////////////////////////
		Ref<StorageBufferSet> SBBoneTransforms;
		Ref<StorageBufferSet> StorageBufferSet;
		Ref<UniformBufferSet> UniformBufferSet;

		glm::mat4* BoneTransformData = nullptr;

		//////////////////////////////////////////////////////////////////////////
		// Quad Vertex and Index buffers
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;

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

		Ref<Pass> PreDepthPass;
		Ref<Pipeline> PreDepthPipeline;
		Ref<Pipeline> PreDepthDynamicPipeline;
		Ref<Framebuffer> PreDepthFramebuffer;
		Ref<Material> PreDepthMaterial;
		Ref<Material> PreDepthDynamicMaterial;
		Ref<Material> PreDepthDynamicMaterialSet2;

		Ref<ComputePipeline> LightCullingPipeline;
		Ref<Material> LightCullingMaterial;
		glm::vec3 LightCullingWorkGroups{};

		// Geometry
		//////////////////////////////////////////////////////////////////////////

		// Render pass for all grid, skybox and meshes.
		Ref<Pass> GeometryPass;
		Ref<Framebuffer> GeometryFramebuffer;

		// STATIC MESHES

		// Main geometry for static meshes.
		Ref<Pipeline> StaticMeshPipeline;
		Ref<Material> StaticMeshMaterial;
	
		// DYNAMIC MESHES

		// Main geometry for dynamic meshes.
		Ref<Pipeline> DynamicMeshPipeline;
		Ref<Material> DynamicMeshMaterial;

		// GRID
		Ref<Pipeline> GridPipeline;
		std::array<Ref<Material>, MAX_FRAMES_IN_FLIGHT> GridMaterials;

		// SKYBOX
		EnvironmentMap SceneEnvironment;

		Ref<Pipeline> SkyboxPipeline;
		Ref<Material> SkyboxMaterial;

		Ref<Material> PreethamMaterial;

		float SkyboxLod = 0.0f;
		float Intensity = 1.0f;

		//////////////////////////////////////////////////////////////////////////
		
		// End Geometry
		
		//////////////////////////////////////////////////////////////////////////

		// Begin SSAO vvvv
		Ref<Pipeline>	 SSAOPipeline;
		Ref<Pass>		 SSAORenderPass;
		Ref<Material>	 SSAOMaterial;
		Ref<Framebuffer> SSAOFramebuffer;
		Ref<Texture2D>	 SSAONoiseImage;

		Ref<Pipeline>	 AOBlurPipeline;
		Ref<Pass>		 AOBlurRenderPass;
		Ref<Material>	 AOBlurMaterial;
		Ref<Framebuffer> AOBlurFramebuffer;
		// End SSAO ^^^^

		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////

		// Begin Scene Composite
		Ref<Pass> SceneComposite;
		Ref< Framebuffer > SceneCompositeFramebuffer;

		Ref<Pipeline> SceneCompositePipeline;
		
		// Input
		Ref<Material> SceneCompositeMaterial;
		
		// Texture pass
		//////////////////////////////////////////////////////////////////////////
		Ref<Pass> TexturePass;
		Ref<Pipeline> TexturePassPipeline;
		// Input
		Ref<Material> TexturePassMaterial;

		//////////////////////////////////////////////////////////////////////////
		// End Scene Composite
		//////////////////////////////////////////////////////////////////////////

		// Selected Geometry
		//////////////////////////////////////////////////////////////////////////

		Ref<Pass> SelectedGeometryPass;
		Ref<Framebuffer> SelectedGeometryFramebuffer;
		Ref<Pipeline> SelectedGeometryPipeline;
		Ref<Pipeline> SelectedGeometryDynamicPipeline;
		Ref<Material> SelectedGeometryMaterial;

		// Jumpflooding
		//////////////////////////////////////////////////////////////////////////

		Ref<Framebuffer> JumpFloodFirstPassFB;
		Ref<Pass> JumpFloodFirstPass;
		Ref<Material> JumpFloodFirstMaterial;
		Ref<Pipeline> JumpFloodFirstPipeline;

		Ref<Pass> JumpFloodEvenPass;
		Ref<Framebuffer> JumpFloodEvenFB;
		Ref<Material> JumpFloodEvenMaterial;
		Ref<Pipeline> JumpFloodEvenPipeline;

		Ref<Framebuffer> JumpFloodOddFB;
		Ref<Pass> JumpFloodOddPass;
		Ref<Material> JumpFloodOddMaterial;
		Ref<Pipeline> JumpFloodOddPipeline;

		// Bloom
		//////////////////////////////////////////////////////////////////////////

		struct BloomTexture
		{
			Ref<Texture2D> Texture;
			std::vector<VkDescriptorImageInfo> ImageInfos;
		};

		std::array<BloomTexture, 3> BloomTextures;
		Ref<ComputePipeline> BloomComputePipeline;
		Ref<Texture2D> BloomDirtTexture;

		Ref<Material> BloomPrefilterMaterial;
		Ref<Material> BloomFirstUpsampleMaterial;

		std::vector<Ref<Material>> BloomDownsampleAMaterials;
		std::vector<Ref<Material>> BloomDownsampleBMaterials;
		std::vector<Ref<Material>> BloomUpsampleMaterials;

		uint32_t BloomWorkSize = 4;

		// The value of a pixel component before it's considered to be an emissive object.
		float BloomThreshold = 1.0f;
		float BloomDirtIntensity = 20.0f;

		//////////////////////////////////////////////////////////////////////////
		// BRDF Lut
		Ref<Texture2D> BRDFLUT_Texture;

		// Late Composite
		//////////////////////////////////////////////////////////////////////////
		Ref<Pass> LateCompositePass;
		Ref<Framebuffer> LateCompositeFramebuffer;

		// Physics Outline
		//////////////////////////////////////////////////////////////////////////
		Ref<Pipeline> PhysicsOutlinePipeline;
		Ref<Material> PhysicsOutlineMaterial;

		// General use samplers.
		//////////////////////////////////////////////////////////////////////////
		
		Ref<Sampler> GeneralUseLinearSampler;
		Ref<Sampler> GeneralUsePointSampler;

		// SMAA
		//////////////////////////////////////////////////////////////////////////
		Ref<ComputePipeline> SMAAEdgeDetectionPipeline;
		Ref<ComputePipeline> SMAAFinalPipeline;

		Ref<Material> SMAAEdgingMaterial;
		Ref<Image2D> SMAAEdgeDetectionOutImage;

		Ref<Material> SMAAFinalMaterial;
		Ref<Image2D> SMAAFinalOutImage;

		Ref<Texture2D> SMAASearchTexture;
		Ref<Texture2D> SMAAAreaTexture;

		// SMAA Composition
		Ref<Pipeline> SMAACompPipeline;
		Ref<Material> SMAACompMaterial;
		Ref<Framebuffer> SMAACompFB;
		Ref<Pass> SMAACompPass;

		// GTAO
		//////////////////////////////////////////////////////////////////////////

		float GTAOEffectRadius = 0.50f;
		float GTAOEffectFalloffRange = 0.62f;
		float GTAORadiusMultiplier = 1.46f;

		// Prefilter
		Ref<ComputePipeline> GTAOPrefilterPipeline;
		Ref<Material> GTAOPrefilterMaterial;
		Ref<Texture2D> GTAOPrefilterOutImage;

		// Image infos for GTAO out image, one image info per image mip.
		std::vector<VkDescriptorImageInfo> GTAOImageInfos;

		// Main Pass
		Ref<ComputePipeline> GTAOMainPipeline;
		Ref<Material> GTAOMainMaterial;
		Ref<Image2D> GTAOEdgesImage;
		Ref<Image2D> GTAONoisyOut;

		// Denoise pass
		Ref<ComputePipeline> GTAODenoisePipeline;

		struct PerDenoisePassInformation
		{
			Ref<Material> Material;
			Ref<Image2D>  OutImage;
		};

		size_t GTAODenoisePassCount = 5llu;
		std::vector<PerDenoisePassInformation> GTAODenoisePassesInformation;

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

		Ref< Shader > GridShader;
		Ref< Shader > SkyboxShader;
		Ref< Shader > PreethamShader;
		Ref< Shader > StaticMeshShader;
		Ref< Shader > DynamicMeshShader;
		Ref< Shader > SceneCompositeShader;
		Ref< Shader > TexturePassShader;
		Ref< Shader > DirShadowMapShader;
		Ref< Shader > DirShadowMapDynamicShader;
		Ref< Shader > SSAOShader;
		Ref< Shader > SSAOBlurShader;
		Ref< Shader > PreDepthShader;
		Ref< Shader > PreDepthDynamicShader;
		Ref< Shader > LightCullingShader;
		Ref< Shader > BloomShader;
		Ref< Shader > PhysicsOutlineShader;
		Ref< Shader > SelectionShader;
		Ref< Shader > SelectionDynamicShader;
		Ref< Shader > JmpFloodFirstShader;
		Ref< Shader > JmpFloodEvenShader;
		Ref< Shader > JmpFloodOddShader;
		Ref< Shader > SMAAEdgeDetectionShader;
		Ref< Shader > SMAABlendingShader;
		Ref< Shader > SMAACompositionShader;
		Ref< Shader > GTAOPrefilterShader;
		Ref< Shader > GTAOMainPassShader;
		Ref< Shader > GTAODenoiseShader;
	};

	struct SceneRendererSpecification
	{
		// If these are left zero, the SceneRenderer will change these to match
		// the current window's size.
		uint32_t Width = 0, Height = 0;

		AOTechnique AOTechnique = AOTechnique::SSAO;
		SceneRendererFlags Flags = SceneRendererFlag_NoFlags;
		Ref<Scene> TargetScene = nullptr;
	};

	class Renderer2D;
	class AluraRenderer;

	class SceneRenderer : public RefTarget
	{
		using ScheduledFunc = std::function<void()>;
	public:
		SceneRenderer() = default;
		SceneRenderer( SceneRendererSpecification& rSpec );
		virtual ~SceneRenderer();

		void ImGuiRender();

		void SetCurrentScene( Scene* pScene );
#if !defined(SAT_DIST)
		Scene* GetCurrentScene() const { return m_pScene; }
#endif

		void SubmitStaticMesh( SharedPtr<Entity> entity, Ref<StaticMesh> mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SubmitDynamicMesh( SharedPtr<Entity> entity, Ref<SkeletalMesh> mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms );
		
		// NOTE: mesh is not the physical mesh in the entity, it is the physics mesh!
		void SubmitPhysicsCollider( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SubmitSelectedStaticMesh( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SubmitSelectedDynamicMesh( SharedPtr<Entity> entity, Ref< SkeletalMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform );

		void SetViewportSize( uint32_t w, uint32_t h );
		void SetViewportPosition( float x, float y ) { m_RendererData.ViewportPos = glm::vec2( x, y ); }

		void HandleKeCommand( const std::string& rKey, const std::string& rValue );

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

		AOTechnique GetAOTechnique() const { return m_AOTechnique; }

		uint32_t Width() const { return m_RendererData.Width; }
		uint32_t Height() const { return m_RendererData.Height; }

		Ref<Renderer2D> GetRenderer2D() const;
		Ref<AluraRenderer> GetAluraRenderer() const;

		void DisableOrEnableBloom();
		void DisableAO();

	private:
		void Init();
		void Terminate();

		void RenderGrid();
		void RenderSkybox();
		void CheckInvalidSkybox();

		void UpdateCascades( const glm::vec3& Direction );

		void CreateGridComponents();

		void CreateSkyboxComponents();

		void InitGeneralUseComponents();

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
		void InitAO( AOTechnique oldTechnique, bool skipScheduler = false );
		void InitAOBlur();
		void InitSelectionPass();
		void InitJumpFlood();
		void InitJmpfFirstPass();
		void InitJmpfEvenPass();
		void InitJmpfOddPass();
		void InitSMAAPass();
		void InitSMAAEdge();
		void InitSMAABlending();
		void InitSMAAComposition();
		void InitGTAOPass();
		void InitGTAOPrefilter();
		void InitGTAOMainPass();
		void InitGTAODenoisePass();

		void InitBuffers();
		void InitRenderer2D();
		void InitAlura();

		void DirShadowMapPass();
		void PreDepthPass();
		void LightCullingPass();
		void GeometryPass();
		void BloomPass();
		void SSAOPass();
		void SSAOBlurPass();
		void GTAOPass();
		void GTAOPrefilterPass();
		void GTAOMainPass();
		void GTAODenoisePass();
		void SelectedGeometryPass();
		void JumpFloodPass();
		void SceneCompositePass();
		void LateCompPhysicsOutline();
		void JumpFloodLatePass();
		void SMAACompositionPass();
		void SMAABlendingPass();
		void SMAAEdgePass();
		void SMAAPass();
		void TexturePass();

		void RenderStaticMeshes();
		void RenderDynamicMeshes();

		void AddScheduledFunction( ScheduledFunc&& rrFunc );

#if !defined(SAT_DIST)
		void OnShaderReloaded( const std::string& rName );
#endif

		void CreateBloomMaterials();
		void BindSceneCompositeAOTexture();
		void SendBoneDataToMap( Ref<SkeletalMesh> mesh, const StaticMeshKey& rKey, const std::vector<glm::mat4>& rBoneTransforms );

		Ref<TextureCube> CreateDymanicSky();
	private:
		SceneRendererFlags m_Flags;
		AOTechnique m_AOTechnique = AOTechnique::SSAO;

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
		std::unordered_map< StaticMeshKey, DynamicDrawCommand > m_DynamicSelectedMeshDrawList;

		//////////////////////////////////////////////////////////////////////////

		std::vector< ScheduledFunc > m_ScheduledFunctions;
		ScheduledFunc m_LightCullingFunction;

	private:
		friend class Scene;
		friend class VulkanContext;
	};
}
