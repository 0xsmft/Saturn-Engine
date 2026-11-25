/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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
#include "SceneRenderer.h"

#include "Saturn/Core/Renderer/RenderThread.h"
#include "Saturn/Core/Random.h"
#include "Saturn/Core/App.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "Texture.h"
#include "Mesh.h"
#include "Material.h"
#include "ComputePipeline.h"
#include "Renderer2D.h"
#include "AluraRenderer.h"
#include "PipelineCache.h"

#include "Saturn/Animation/SkeletonAsset.h"

#include "Saturn/Core/Buffer.h"
#include "Saturn/Core/Profiler.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/PackagedResources/BuiltinResourceManager.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#endif

#include <glm/gtx/matrix_decompose.hpp>

#if SAT_SSAO_OLD_KERNEL_GEN == 1
#include <random>
#endif

// SMAA
#include "Embedded/SMAA_SearchTex.embed"

constexpr auto SAT_PI = 3.14159265358979323846;
constexpr auto kShadowMapSize = 4096.0f;

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	SceneRenderer::SceneRenderer( /*const*/ SceneRendererSpecification& rSpec )
		: m_Flags( rSpec.Flags ), m_AOTechnique( rSpec.AOTechnique )
	{
		if( rSpec.Width != 0 || rSpec.Height != 0 )
		{
			m_RendererData.Width = rSpec.Width;
			m_RendererData.Height = rSpec.Height;
		}

		Init();

		if( rSpec.TargetScene )
		{
			SetCurrentScene( rSpec.TargetScene.Get() );
		}
	}

	SceneRenderer::~SceneRenderer()
	{
		Terminate();
	}

	//////////////////////////////////////////////////////////////////////////

	void SceneRenderer::Init()
	{
		if( m_RendererData.Width == 0 && m_RendererData.Height == 0 )
		{
			m_RendererData.Width = Application::Get()->GetWindow()->GetWidth();
			m_RendererData.Height = Application::Get()->GetWindow()->GetHeight();
		}

		//////////////////////////////////////////////////////////////////////////
		// Geometry 
		//////////////////////////////////////////////////////////////////////////

		m_RendererData.StorageBufferSet = Ref<StorageBufferSet>::Create( 0, 0 );
		m_RendererData.StorageBufferSet->Create( 0, 14 ); // Create Light culling buffer.

		m_RendererData.SBBoneTransforms = Ref<StorageBufferSet>::Create( 0, 0 );
		m_RendererData.SBBoneTransforms->Create( 2, 15, false ); // Create bone transform buffers.
		m_RendererData.SBBoneTransforms->Resize( 2, 15, sizeof( glm::mat4 ) * 1024 );

		// 1024 max animated meshes
		m_RendererData.BoneTransformData = new glm::mat4[ 1 * 1024 ]{};

		for( size_t i = 0; i < 1024; ++i )
		{
			m_RendererData.BoneTransformData[ i ] = glm::mat4( 1.0f ); // identity
		}

		m_RendererData.IsSwapchainTarget = HasFlag( SceneRendererFlag_SwapchainTarget );

		// Shared geometry UBs
		m_RendererData.UniformBufferSet = Ref<UniformBufferSet>::Create();
		// Fill out UBs
		//																	SIZE -> BINDING
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBStaticMeshMatrices ), 0u );
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBLightData ), 1u );
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBSceneData ), 2u );
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBShadowData ), 3u );
		m_RendererData.UniformBufferSet->CreateBuffer( 4llu, 12u ); // Debug Data
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBPointLights ), 13u );

		InitPipelineCache();

		InitPreDepth();

		InitGeometryPass();

		// Create grid.
		CreateGridComponents();

		// Create skybox.
		CreateSkyboxComponents();

		InitDirShadowMap();

		InitBloom();

		InitVolumetricCloudsPass();

		InitSceneComposite();

		InitLateComposite();

		InitGeneralUseComponents();

		InitSMAAPass();

		InitSelectionPass();

		InitJumpFlood();

		InitTexturePass();

		InitAO( AOTechnique::None );

		m_RendererData.BRDFLUT_Texture = BuiltinResourceManager::Get()->GetOrLoadResource<Texture2D>( PackagedResourceType::BRDFLutTexture );

		constexpr size_t kSkTransformCount = static_cast<size_t>( 1024 ) * 10;
		m_RendererData.SubmeshTransformData.resize( MAX_FRAMES_IN_FLIGHT );
		
		for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			m_RendererData.SubmeshTransformData[ i ].VertexBuffer = Ref<VertexBuffer>::Create( sizeof( TransformBufferData ) * kSkTransformCount );
			m_RendererData.SubmeshTransformData[ i ].pData = new TransformBufferData[ kSkTransformCount ];
		}

		//////////////////////////////////////////////////////////////////////////
		// Subrenderers
		InitRenderer2D();
		InitAlura();
	
#if !defined(SAT_DIST)
		Renderer::Get()->AddShaderReloadCB( SAT_BIND_EVENT_FN( OnShaderReloaded ) );
#endif
	}

	Ref<Image2D> SceneRenderer::CompositeImage()
	{
		return m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];
	}

	void SceneRenderer::Terminate()
	{
#if !defined(SAT_DIST)
		Renderer::Get()->ClearShaderReferences();
#endif
		m_pScene = nullptr;

		FlushDrawList();

		m_Renderer2D = nullptr;
		m_RendererData.Terminate();
	}

	void SceneRenderer::InitGeometryPass()
	{
		// Create render pass.
		if( !m_RendererData.GeometryPass )
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "Geometry Pass";
			PassSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16, ImageFormat::RGBA16, ImageFormat::DEPTH24STENCIL8 };
			PassSpec.LoadDepth = true;

			m_RendererData.GeometryPass = Ref< Pass >::Create( PassSpec );
		}

		// Create geometry framebuffer.
		if( m_RendererData.GeometryFramebuffer )
			m_RendererData.GeometryFramebuffer = nullptr;

		FramebufferSpecification FBSpec = {};
		FBSpec.RenderPass = m_RendererData.GeometryPass;
		FBSpec.Width = m_RendererData.Width;
		FBSpec.Height = m_RendererData.Height;
		FBSpec.ExistingImages[ 3 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource();
		// Depth will be the PreDepth image.
		FBSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16, ImageFormat::RGBA16 };

		m_RendererData.GeometryFramebuffer = Ref< Framebuffer >::Create( FBSpec );

		//////////////////////////////////////////////////////////////////////////
		// STATIC MESHES
		//////////////////////////////////////////////////////////////////////////

		// Create the static meshes pipeline.
		// Load the shader
		if( !m_RendererData.StaticMeshShader )
		{
			m_RendererData.StaticMeshShader = ShaderLibrary::Get().FindOrLoad( "shader_new", "content/shaders/shader_new.glsl" );
			m_RendererData.StaticMeshMaterial = Ref<Material>::Create( m_RendererData.StaticMeshShader, "StaticMeshMat" );
		}

		if( !m_RendererData.StaticMeshPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Static Meshes";
			PipelineSpec.Shader = m_RendererData.StaticMeshShader;
			PipelineSpec.RenderPass = m_RendererData.GeometryPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" },
			};
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;

			m_RendererData.StaticMeshPipeline = m_GeometryPipelineCache->EmplaceNewPipeline( m_RendererData.StaticMeshShader->GetShaderHash(), PipelineSpec );
		}

		//////////////////////////////////////////////////////////////////////////
		// DYNAMIC MESHES
		//////////////////////////////////////////////////////////////////////////

		// Create the static meshes pipeline.
		// Load the shader
		if( !m_RendererData.DynamicMeshShader )
		{
			m_RendererData.DynamicMeshShader = ShaderLibrary::Get().FindOrLoad( "shader_new_anim", "content/shaders/shader_new_anim.glsl" );

			m_RendererData.DynamicMeshMaterial = Ref<Material>::Create( m_RendererData.DynamicMeshShader, "DynamicMeshMat", 2 );

			for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
			{
				m_RendererData.DynamicMeshMaterial->SetSB( 15u, m_RendererData.SBBoneTransforms->Get( 2u, 15u, ( uint32_t ) i ) );
			}
		}

		if( !m_RendererData.DynamicMeshPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Dynamic Meshes";
			PipelineSpec.Shader = m_RendererData.DynamicMeshShader;
			PipelineSpec.RenderPass = m_RendererData.GeometryPass;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal"   },
				{ ShaderDataType::Float3, "a_Tangent"  },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" },
			};
			PipelineSpec.AdditionalLayoutAtEnd = {
				{ ShaderDataType::Int4,   "a_BoneIndices" },
				{ ShaderDataType::Float4, "a_BoneWeights" }
			};

			m_RendererData.DynamicMeshPipeline = m_GeometryPipelineCache->EmplaceNewPipeline( m_RendererData.DynamicMeshShader->GetShaderHash(), PipelineSpec );
		}

		if( !m_RendererData.DefaultMaterial )
		{
			m_RendererData.DefaultMaterial = Ref<Material>::Create( m_RendererData.StaticMeshShader, "DefMat" );

			Ref<MaterialAsset> memoryOnlyMaterialAsset = Ref<MaterialAsset>::Create( m_RendererData.DefaultMaterial );
			m_RendererData.DefaultMaterialRegistry = Ref<MaterialRegistry>::Create();
			m_RendererData.DefaultMaterialRegistry->AddAsset( memoryOnlyMaterialAsset );
		}
	}

	void SceneRenderer::InitDirShadowMap()
	{
		m_RendererData.ShadowCascades.resize( SHADOW_CASCADE_COUNT );
		m_RendererData.DirShadowMapPasses.resize( SHADOW_CASCADE_COUNT );
		m_RendererData.DirShadowMapPipelines.resize( SHADOW_CASCADE_COUNT );
		m_RendererData.DirShadowMapDynamicPipelines.resize( SHADOW_CASCADE_COUNT );

		if( !m_RendererData.DirShadowMapShader )
		{
			m_RendererData.DirShadowMapShader = ShaderLibrary::Get().FindOrLoad( "ShadowMap", "content/shaders/ShadowMap.glsl" );
			m_RendererData.DirShadowMapDynamicShader = ShaderLibrary::Get().FindOrLoad( "ShadowMap-Dynamic", "content/shaders/ShadowMap-Dynamic.glsl" );

			m_RendererData.DirShadowMapMaterial = Ref<Material>::Create( m_RendererData.DirShadowMapShader, "ShdMap" );

			m_RendererData.DirShadowMapDynamicMaterialSet2 = Ref<Material>::Create( m_RendererData.DirShadowMapDynamicShader, "DirShdMpSkS2", 1 );
			for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
			{
				m_RendererData.DirShadowMapDynamicMaterialSet2->SetSB( 15u, m_RendererData.SBBoneTransforms->Get( 2u, 15u, ( uint32_t ) i ) );
			}
		}

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Name = "DirShadowMap";
		PipelineSpec.Shader = m_RendererData.DirShadowMapShader;
		PipelineSpec.UseDepthTest = true;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tanget" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};
		PipelineSpec.InstanceLayout = {
			{ ShaderDataType::Float4, "a_TransformBufferR1" },
			{ ShaderDataType::Float4, "a_TransformBufferR2" },
			{ ShaderDataType::Float4, "a_TransformBufferR3" },
			{ ShaderDataType::Float4, "a_TransformBufferR4" }
		};
		PipelineSpec.CullMode = CullMode::Back;
		PipelineSpec.HasColorAttachment = false;
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;

		// Layered image
		Ref<Image2D> shadowImage = Ref<Image2D>::Create( ImageFormat::DEPTH32F, ( uint32_t ) kShadowMapSize, ( uint32_t ) kShadowMapSize, 4, 1 );
		shadowImage->SetDebugName( "Layered shadow image" );

		FramebufferSpecification FBSpec = {};
		FBSpec.Width = ( uint32_t ) kShadowMapSize;
		FBSpec.Height = ( uint32_t ) kShadowMapSize;
		FBSpec.ArrayLevels = SHADOW_CASCADE_COUNT;
		FBSpec.ExistingImages[ 0 ] = shadowImage;

		PassSpecification PassSpec = {};
		PassSpec.Name = "Dir Shadow Map";
		PassSpec.Attachments = { ImageFormat::Depth };

		for( size_t i = 0; i < SHADOW_CASCADE_COUNT; ++i )
		{
			m_RendererData.DirShadowMapPasses[ i ] = Ref<Pass>::Create( PassSpec );

			FBSpec.RenderPass = m_RendererData.DirShadowMapPasses[ i ];
			FBSpec.ExistingImageLayer = ( uint32_t ) i;

			PipelineSpec.RenderPass = m_RendererData.DirShadowMapPasses[ i ];

			m_RendererData.ShadowCascades[ i ].Framebuffer = Ref<Framebuffer>::Create( FBSpec );

			m_RendererData.DirShadowMapPipelines[ i ] = Ref< Pipeline >::Create( PipelineSpec );
		}
		
		//////////////////////////////////////////////////////////////////////////
		// Dynamic Meshes
		PipelineSpec.Name = "DirShadowMap-Dynamic";
		PipelineSpec.Shader = m_RendererData.DirShadowMapDynamicShader;
		PipelineSpec.AdditionalLayoutAtEnd = {
			{ ShaderDataType::Int4,   "a_BoneIndices" },
			{ ShaderDataType::Float4, "a_BoneWeights" }
		};

		PassSpec.Name = "Dir Shadow Map-Dynamic";
		PassSpec.Attachments = { ImageFormat::Depth };
		for( size_t i = 0; i < SHADOW_CASCADE_COUNT; ++i )
		{
			PipelineSpec.RenderPass = m_RendererData.DirShadowMapPasses[ i ];
			m_RendererData.DirShadowMapDynamicPipelines[ i ] = Ref< Pipeline >::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitPreDepth()
	{
		if( !m_RendererData.PreDepthPass ) 
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "PreDepth";
			PassSpec.Attachments = { ImageFormat::DEPTH24STENCIL8 };

			m_RendererData.PreDepthPass = Ref<Pass>::Create( PassSpec );
		}

		if( m_RendererData.PreDepthFramebuffer ) 
		{
			m_RendererData.PreDepthFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.RenderPass = m_RendererData.PreDepthPass;
			FBSpec.Attachments = { ImageFormat::DEPTH24STENCIL8 };

			m_RendererData.PreDepthFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.PreDepthShader )
		{
			m_RendererData.PreDepthShader = ShaderLibrary::Get().FindOrLoad( "PreDepth", "content/shaders/PreDepth.glsl" );
			m_RendererData.PreDepthDynamicShader = ShaderLibrary::Get().FindOrLoad( "PreDepth-Dynamic", "content/shaders/PreDepth-Dynamic.glsl" );
			m_RendererData.LightCullingShader = ShaderLibrary::Get().FindOrLoad( "LightCulling", "content/shaders/LightCulling.glsl" );

			m_RendererData.PreDepthMaterial = Ref<Material>::Create( m_RendererData.PreDepthShader, "PreDepth" );
			m_RendererData.PreDepthDynamicMaterial = Ref<Material>::Create( m_RendererData.PreDepthDynamicShader, "PreDepth" );
			m_RendererData.PreDepthDynamicMaterialSet2 = Ref<Material>::Create( m_RendererData.PreDepthDynamicShader, "PreDepth-S2", 1 );

			m_RendererData.LightCullingMaterial = Ref<Material>::Create( m_RendererData.LightCullingShader, "LightCulling" );
		}

		if( !m_RendererData.PreDepthPipeline || !m_RendererData.PreDepthPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "PreDepth";
			PipelineSpec.Shader = m_RendererData.PreDepthShader;
			PipelineSpec.RenderPass = m_RendererData.PreDepthPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS;
			PipelineSpec.HasColorAttachment = false;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tanget" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" }
			};

			m_RendererData.PreDepthPipeline = Ref<Pipeline>::Create( PipelineSpec );

			//////////////////////////////////////////////////////////////////////////
			// Dynamic (Animated)
			//////////////////////////////////////////////////////////////////////////

			PipelineSpec.Name = "PreDepth-Dynamic";
			PipelineSpec.Shader = m_RendererData.PreDepthDynamicShader;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal"   },
				{ ShaderDataType::Float3, "a_Tangent"  },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" },
			};
			PipelineSpec.AdditionalLayoutAtEnd = {
				{ ShaderDataType::Int4,   "a_BoneIndices" },
				{ ShaderDataType::Float4, "a_BoneWeights" }
			};

			m_RendererData.PreDepthDynamicPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}

		for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			m_RendererData.PreDepthDynamicMaterialSet2->SetSB( 15u, m_RendererData.SBBoneTransforms->Get( 2u, 15u, ( uint32_t ) i ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// Light culling
		//////////////////////////////////////////////////////////////////////////
		if( !m_RendererData.LightCullingPipeline )
			m_RendererData.LightCullingPipeline = Ref<ComputePipeline>::Create( m_RendererData.LightCullingShader );

		m_RendererData.LightCullingMaterial->SetResource( "u_PreDepth", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
	}

	void SceneRenderer::InitSceneComposite()
	{
		if( m_RendererData.SceneComposite )
			m_RendererData.SceneComposite->Recreate();
		else
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Scene Composite (PP) pass";
			PassSpec.Attachments = { ImageFormat::RGBA8 };

			m_RendererData.SceneComposite = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.SceneCompositeFramebuffer )
			m_RendererData.SceneCompositeFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height );
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.SceneComposite;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.CreateDepth = false;

			FBSpec.Attachments = { ImageFormat::RGBA8 };

			m_RendererData.SceneCompositeFramebuffer = Ref< Framebuffer >::Create( FBSpec );
		}

		if( !m_RendererData.SceneCompositeShader )
		{
			m_RendererData.SceneCompositeShader = ShaderLibrary::Get().FindOrLoad( "SceneComposite", "content/shaders/SceneComposite.glsl" );
		
			m_RendererData.SceneCompositeMaterial = Ref<Material>::Create( m_RendererData.SceneCompositeShader, "SceneComposite" );
		}

		m_RendererData.SceneCompositeMaterial->SetResource( "u_GeometryPassTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );
		m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ].Texture );
		m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomDirtTexture", Renderer::Get()->GetPinkTexture() );
		m_RendererData.SceneCompositeMaterial->SetResource( "u_DepthTexture", m_RendererData.GeometryFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.SceneCompositeMaterial->SetResource( "u_CloudsTexture", m_RendererData.CloudsReconstructOut );

		if( !m_RendererData.SceneCompositePipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Scene Composite";
			PipelineSpec.Shader = m_RendererData.SceneCompositeShader;
			PipelineSpec.RenderPass = m_RendererData.SceneComposite;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::None;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;

			m_RendererData.SceneCompositePipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitLateComposite()
	{
		if( m_RendererData.LateCompositePass )
			m_RendererData.LateCompositePass->Recreate();
		else
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Late Composite pass";
			PassSpec.LoadColor = true;
			PassSpec.LoadDepth = true;

			// Both the color and the depth will be loaded.
			// Color = Color attachment from the Scene Composite.
			// Depth = PreDepth.
			PassSpec.Attachments = { ImageFormat::RGBA8, ImageFormat::DEPTH24STENCIL8 };

			m_RendererData.LateCompositePass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.LateCompositeFramebuffer ) 
		{
			FramebufferSpecification NewSpec;

			NewSpec.ExistingImages[ 0 ] = CompositeImage();
			NewSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource();

			m_RendererData.LateCompositeFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height, NewSpec );
		}
		else	
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.LateCompositePass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;

			FBSpec.ExistingImages[ 0 ] = CompositeImage();
			FBSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource();

			m_RendererData.LateCompositeFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		// All of these use the late comp pass.
		InitPhysicsOutline();
	}

	void SceneRenderer::InitPhysicsOutline()
	{
		if( !m_RendererData.PhysicsOutlineShader )
		{
			m_RendererData.PhysicsOutlineShader = ShaderLibrary::Get().FindOrLoad( "PhysicsCollider", "content/shaders/PhysicsCollider.glsl" );
		}

		m_RendererData.PhysicsOutlineMaterial = Ref<Material>::Create( m_RendererData.PhysicsOutlineShader, "PhysicsOutline" );

		if( !m_RendererData.PhysicsOutlinePipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Late Composite (PhysCollider)";
			PipelineSpec.Shader = m_RendererData.PhysicsOutlineShader;
			PipelineSpec.RenderPass = m_RendererData.LateCompositePass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_LINE;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tanget" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" }
			};

			m_RendererData.PhysicsOutlinePipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitBloom()
	{
		if( !m_RendererData.BloomShader )
		{
			m_RendererData.BloomShader = ShaderLibrary::Get().FindOrLoad( "Bloom", "content/shaders/Bloom.glsl" );
		}

		if( !m_RendererData.BloomComputePipeline )
		{
			m_RendererData.BloomComputePipeline = Ref<ComputePipeline>::Create( m_RendererData.BloomShader );
			m_RendererData.BloomDirtTexture = Renderer::Get()->GetPinkTexture();
		}

		CreateBloomMaterials();
	}

	void SceneRenderer::InitTexturePass()
	{
		if( !m_RendererData.IsSwapchainTarget )
			return;

		if( !m_RendererData.TexturePassShader )
		{
			m_RendererData.TexturePassShader = ShaderLibrary::Get().FindOrLoad( "TexturePass", "content/shaders/TexturePass.glsl" );

			m_RendererData.TexturePassMaterial = Ref<Material>::Create( m_RendererData.TexturePassShader, "TexturePassMat" );
		}

		m_RendererData.TexturePassMaterial->SetResource( "u_InputTexture", m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ] );

		if( !m_RendererData.TexturePassPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Texture Pass";
			PipelineSpec.Shader = m_RendererData.TexturePassShader;
			PipelineSpec.RenderPass = VulkanContext::Get()->GetDefaultPass();
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::None;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};

			m_RendererData.TexturePassPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitSSAO()
	{
		if( !m_RendererData.SSAOShader )
		{
			m_RendererData.SSAOShader = ShaderLibrary::Get().FindOrLoad( "SSAO", "content/shaders/SSAO.glsl" );
			m_RendererData.SSAOMaterial = Ref<Material>::Create( m_RendererData.SSAOShader, "SSAO" );
		}

		m_RendererData.SSAOMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.SSAOMaterial->SetResource( "u_ViewNormalTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 1 ] );

		if( !m_RendererData.SSAONoiseGenerated || !m_RendererData.SSAONoiseImage )
		{
			std::vector<glm::vec4> ssaoNoise( 4 * 4 );
			for( size_t i = 0; i < ssaoNoise.size(); ++i )
			{
				ssaoNoise[ i ] = glm::vec4(
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					0.0F, 0.0F );
			}

			m_RendererData.SSAONoiseGenerated = true;

			m_RendererData.SSAONoiseImage = Ref<Texture2D>::Create( ImageFormat::RGBA32F, 4, 4, ssaoNoise.data(), false, AddressingMode::Repeat, TextureLoadFlags_NoMips );

			m_RendererData.SSAOMaterial->SetResource( "u_NoiseTexture", m_RendererData.SSAONoiseImage );

			auto xlerp = []( float a, float b, float f )
			{
				return a + f * ( b - a );
			};

#if SAT_SSAO_OLD_KERNEL_GEN == 1
			std::default_random_engine rndEngine;
			std::uniform_real_distribution<float> rndDist( 0.0f, 1.0f );

			// Sample kernel
			std::vector<glm::vec4> ssaoKernel( 32 );
			for( uint32_t i = 0; i < 32; ++i )
			{
				glm::vec3 sample( rndDist( rndEngine ) * 2.0 - 1.0, rndDist( rndEngine ) * 2.0 - 1.0, rndDist( rndEngine ) );
				sample = glm::normalize( sample );
				sample *= rndDist( rndEngine );

				float scale = float( i ) / float( 32 );
				scale = xlerp( 0.1f, 1.0f, scale * scale );
				ssaoKernel[ i ] = glm::vec4( sample * scale, 0.0f );
			}
#else
			std::vector<glm::vec4> ssaoKernel( 32 );
			for( uint32_t i = 0; i < 32; ++i )
			{
				glm::vec3 sample( 
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					Random::RandomFloatInRange( 0.0F, 1.0F ) );

				sample = glm::normalize( sample );
				sample *= Random::RandomFloatInRange( 0.0f, 1.0f );

				float scale = ( float ) i / 32.0f;
				scale = xlerp( 0.1f, 1.0f, scale * scale );

				ssaoKernel[ i ] = glm::vec4( sample * scale, 0.0f );
			}
#endif

			struct USSAOData
			{
				glm::vec4 Samples[ 32 ];
				// Radius
				float R = 0.5f;
			} u_Data{};

			std::memcpy( u_Data.Samples, ssaoKernel.data(), ssaoKernel.size() * sizeof( glm::vec4 ) );

			// We cannot write to the UniformBufferSet as we don't use the same UB as the other shaders
			m_RendererData.SSAOMaterial->UploadDataToUB( 1, &u_Data, sizeof( u_Data ) );
		}

		if( !m_RendererData.SSAORenderPass )
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "SSAO RP";
			PassSpec.Attachments = { ImageFormat::RED8 };

			m_RendererData.SSAORenderPass = Ref<Pass>::Create( PassSpec );
		}

		if( m_RendererData.SSAOFramebuffer )
		{
			m_RendererData.SSAOFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height );
		}
		else
		{
			FramebufferSpecification FBSpec;
			FBSpec.RenderPass = m_RendererData.SSAORenderPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.Attachments = { ImageFormat::RED8 };
			FBSpec.CreateDepth = false;

			m_RendererData.SSAOFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.SSAOPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "SSAO";
			PipelineSpec.Shader = m_RendererData.SSAOShader;
			PipelineSpec.RenderPass = m_RendererData.SSAORenderPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::None;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.BlendMode = PipelineBlendMode::OneZero;
			PipelineSpec.EnableBlending = false;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};

			m_RendererData.SSAOPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitHBAO()
	{
	}

	void SceneRenderer::InitAO( AOTechnique oldTechnique, bool skipScheduler /*=false*/ )
	{
		if( !skipScheduler )
		{
			// Schedule to the beginning of the next frame.
			AddScheduledFunction( [ oldTechnique, this ]()
			{
				switch( oldTechnique )
				{
					default:
					case AOTechnique::None:
						break;

					case AOTechnique::SSAO:
						m_RendererData.ClearSSAOResources();
						break;

					case AOTechnique::HBAO:
						m_RendererData.ClearHBAOResources();
						break;

					case AOTechnique::GTAO:
					{
						// Remove GTAO Flag
						m_RendererData.SceneCompositeFlags &= ~SceneCompositeFlag_GTAO;
//						m_RendererData.ClearGTAOResources();
					} break;
				}
			} );
		}

		// Init new resources now so they are ready for the next frame!
		switch( m_AOTechnique )
		{
			default:
			case AOTechnique::None:
				break;			
			
			case AOTechnique::SSAO:
			{
				InitSSAO();
				InitAOBlur();
			} break;
			
			case AOTechnique::HBAO:
			{
				InitHBAO();
			} break;

			case AOTechnique::GTAO: 
			{
				m_RendererData.SceneCompositeFlags |= SceneCompositeFlag_GTAO;
				InitGTAOPass();
			} break;
		}

		BindSceneCompositeAOTexture();
	}

	void SceneRenderer::InitAOBlur()
	{
		if( !m_RendererData.SSAOBlurShader )
		{
			m_RendererData.SSAOBlurShader = ShaderLibrary::Get().FindOrLoad( "SSAO-Blur", "content/shaders/SSAO-Blur.glsl" );
			m_RendererData.AOBlurMaterial = Ref<Material>::Create( m_RendererData.SSAOBlurShader, "SSAO" );
		}

		m_RendererData.AOBlurMaterial->SetResource( "u_AOTexture", m_RendererData.SSAOFramebuffer->GetColorAttachmentsResources()[ 0 ] );

		if( !m_RendererData.AOBlurRenderPass )
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "AO-Blur RP";
			PassSpec.Attachments = { ImageFormat::RED8 };

			m_RendererData.AOBlurRenderPass = Ref<Pass>::Create( PassSpec );
		}

		if( m_RendererData.AOBlurFramebuffer )
		{
			m_RendererData.AOBlurFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height );
		}
		else
		{
			FramebufferSpecification FBSpec;
			FBSpec.RenderPass = m_RendererData.AOBlurRenderPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.Attachments = { ImageFormat::RED8 };
			FBSpec.CreateDepth = false;

			m_RendererData.AOBlurFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.AOBlurPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "AO-Blur";
			PipelineSpec.Shader = m_RendererData.SSAOBlurShader;
			PipelineSpec.RenderPass = m_RendererData.SSAORenderPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::None;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.BlendMode = PipelineBlendMode::OneZero;
			PipelineSpec.EnableBlending = false;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};

			m_RendererData.AOBlurPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitSelectionPass()
	{
		if( !m_RendererData.SelectedGeometryPass )
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Selected Geometry pass";
			PassSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH24STENCIL8 };

			m_RendererData.SelectedGeometryPass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.SelectedGeometryFramebuffer )
		{
			m_RendererData.SelectedGeometryFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height, {} );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.SelectedGeometryPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH24STENCIL8 };

			m_RendererData.SelectedGeometryFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.SelectionShader )
		{
			m_RendererData.SelectionShader = ShaderLibrary::Get().FindOrLoad( "SelectedGeometry", "content/shaders/SelectedGeometry.glsl" );
			m_RendererData.SelectionDynamicShader = ShaderLibrary::Get().FindOrLoad( "SelectedGeometry-Dynamic", "content/shaders/SelectedGeometry-Dynamic.glsl" );
		}

		m_RendererData.SelectedGeometryMaterial = Ref<Material>::Create( m_RendererData.SelectionShader, "SelectedGeometry" );

		if( !m_RendererData.SelectedGeometryPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Selected Geometry";
			PipelineSpec.Shader = m_RendererData.SelectionShader;
			PipelineSpec.RenderPass = m_RendererData.SelectedGeometryPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tanget" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" }
			};

			m_RendererData.SelectedGeometryPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}

		if( !m_RendererData.SelectedGeometryDynamicPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Selected Geometry (Dynamic)";
			PipelineSpec.Shader = m_RendererData.SelectionDynamicShader;
			PipelineSpec.RenderPass = m_RendererData.SelectedGeometryPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tanget" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			PipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_TransformBufferR1" },
				{ ShaderDataType::Float4, "a_TransformBufferR2" },
				{ ShaderDataType::Float4, "a_TransformBufferR3" },
				{ ShaderDataType::Float4, "a_TransformBufferR4" }
			};
			PipelineSpec.AdditionalLayoutAtEnd = {
				{ ShaderDataType::Int4,   "a_BoneIndices" },
				{ ShaderDataType::Float4, "a_BoneWeights" }
			};

			m_RendererData.SelectedGeometryDynamicPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitJumpFlood()
	{
		if( !m_RendererData.JmpFloodFirstShader || !m_RendererData.JmpFloodEvenShader || !m_RendererData.JmpFloodOddShader )
		{
			m_RendererData.JmpFloodFirstShader = ShaderLibrary::Get().FindOrLoad( "JumpFloodFirst", "content/shaders/JumpFloodFirst.glsl" );
			m_RendererData.JmpFloodEvenShader = ShaderLibrary::Get().FindOrLoad( "JumpFloodEven", "content/shaders/JumpFloodEven.glsl" );
			m_RendererData.JmpFloodOddShader = ShaderLibrary::Get().FindOrLoad( "JumpFloodOdd", "content/shaders/JumpFloodOdd.glsl" );

			m_RendererData.JumpFloodFirstMaterial = Ref<Material>::Create( m_RendererData.JmpFloodFirstShader, "JumpFloodFirst" );
			m_RendererData.JumpFloodEvenMaterial = Ref<Material>::Create( m_RendererData.JmpFloodEvenShader, "JumpFloodEven" );
			m_RendererData.JumpFloodOddMaterial = Ref<Material>::Create( m_RendererData.JmpFloodOddShader, "JumpFloodOdd" );
		}

		InitJmpfFirstPass();
		InitJmpfEvenPass();
		InitJmpfOddPass();
	}

	void SceneRenderer::InitJmpfFirstPass()
	{
		if( !m_RendererData.JumpFloodFirstPass )
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Jump Flood 0 pass";
			PassSpec.Attachments = { ImageFormat::RGBA32F };

			m_RendererData.JumpFloodFirstPass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.JumpFloodFirstPassFB )
		{
			m_RendererData.JumpFloodFirstPassFB->Recreate( m_RendererData.Width, m_RendererData.Height, {} );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.JumpFloodFirstPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.CreateDepth = false;
			FBSpec.Attachments = { ImageFormat::RGBA32F };

			m_RendererData.JumpFloodFirstPassFB = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.JumpFloodFirstPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "JumpFloodFirst";
			PipelineSpec.Shader = m_RendererData.JmpFloodFirstShader;
			PipelineSpec.RenderPass = m_RendererData.JumpFloodFirstPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
			PipelineSpec.DepthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
			PipelineSpec.BlendMode = PipelineBlendMode::OneZero;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			m_RendererData.JumpFloodFirstPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}

		m_RendererData.JumpFloodFirstMaterial->SetResource( "u_InputTexture", m_RendererData.SelectedGeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );
	}

	void SceneRenderer::InitJmpfEvenPass()
	{
		if( !m_RendererData.JumpFloodEvenPass )
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Jump Flood Even pass";
			PassSpec.Attachments = { ImageFormat::RGBA32F };

			m_RendererData.JumpFloodEvenPass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.JumpFloodEvenFB )
		{
			m_RendererData.JumpFloodEvenFB->Recreate( m_RendererData.Width, m_RendererData.Height, {} );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.JumpFloodEvenPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.CreateDepth = false;
			FBSpec.Attachments = { ImageFormat::RGBA32F };

			m_RendererData.JumpFloodEvenFB = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.JumpFloodEvenPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "JumpFloodEvenPass";
			PipelineSpec.Shader = m_RendererData.JmpFloodEvenShader;
			PipelineSpec.RenderPass = m_RendererData.JumpFloodEvenPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
			PipelineSpec.DepthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
			PipelineSpec.BlendMode = PipelineBlendMode::OneZero;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			m_RendererData.JumpFloodEvenPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}

		m_RendererData.JumpFloodEvenMaterial->SetResource( "u_InputTexture", m_RendererData.JumpFloodFirstPassFB->GetColorAttachmentsResources()[ 0 ] );
	}

	void SceneRenderer::InitJmpfOddPass()
	{
		if( !m_RendererData.JumpFloodOddPass )
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Jump Flood Odd pass";
			PassSpec.LoadColor = true;
			PassSpec.Attachments = { ImageFormat::RGBA8 };

			m_RendererData.JumpFloodOddPass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.JumpFloodOddFB )
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];

			m_RendererData.JumpFloodOddFB->Recreate( m_RendererData.Width, m_RendererData.Height, FBSpec );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.JumpFloodOddPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.CreateDepth = false;
			FBSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];

			m_RendererData.JumpFloodOddFB = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.JumpFloodOddPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "JumpFloodOddPass";
			PipelineSpec.Shader = m_RendererData.JmpFloodOddShader;
			PipelineSpec.RenderPass = m_RendererData.JumpFloodOddPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			m_RendererData.JumpFloodOddPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}

		m_RendererData.JumpFloodOddMaterial->SetResource( "u_InputTexture", m_RendererData.JumpFloodEvenFB->GetColorAttachmentsResources()[ 0 ] );
	}

	void SceneRenderer::InitSMAAPass()
	{
		InitSMAAEdge();
		InitSMAABlending();
		InitSMAAComposition();
	}

	void SceneRenderer::InitSMAAEdge()
	{
		if( !m_RendererData.SMAAEdgeDetectionShader )
		{
			m_RendererData.SMAAEdgeDetectionShader = ShaderLibrary::Get().FindOrLoad( "SMAA-EdgeDetection", "content/shaders/SMAA-EdgeDetection.glsl" );
		}
		
		m_RendererData.SMAAEdgeDetectionOutImage = Ref<Image2D>::Create( 
			ImageFormat::RG8, 
			m_RendererData.Width, m_RendererData.Height, 
			1, 1, 1, 
			ImageTiling::Optimal, true );

		m_RendererData.SMAAEdgeDetectionPipeline = Ref<ComputePipeline>::Create( m_RendererData.SMAAEdgeDetectionShader );

		m_RendererData.SMAAEdgingMaterial = Ref<Material>::Create( m_RendererData.SMAAEdgeDetectionShader, "SMAAEdge" );
	
		m_RendererData.SMAAFinalOutImage = Ref<Image2D>::Create(
			ImageFormat::RGBA8,
			m_RendererData.Width, m_RendererData.Height,
			1, 1, 1,
			ImageTiling::Optimal, true
		);
		m_RendererData.SMAAFinalOutImage->SetDebugName( "SMAAFinalOutImage" );

		// Write the output image.
		m_RendererData.SMAAEdgingMaterial->SetResource( "o_OutEdges",  m_RendererData.SMAAEdgeDetectionOutImage );

		m_RendererData.SMAAEdgingMaterial->SetSeparateImage( "u_InFinalColor", 
			m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ] );

		m_RendererData.SMAAEdgingMaterial->SetResource( "s_PointSampler", m_RendererData.GeneralUseLinearSampler );
	}

	void SceneRenderer::InitSMAABlending()
	{
		if( !m_RendererData.SMAABlendingShader )
		{
			m_RendererData.SMAABlendingShader = ShaderLibrary::Get().FindOrLoad( "SMAA-BlendAndDoWeights", "content/shaders/SMAA-BlendAndDoWeights.glsl" );

			m_RendererData.SMAASearchTexture = Ref<Texture2D>::Create( 
				ImageFormat::RED8, 
				SAT_SMAA_SEARCHTEX_WIDTH,
				SAT_SMAA_SEARCHTEX_HEIGHT,
				&s_SMAASearchTexBytes,
				true );

			m_RendererData.SMAAAreaTexture = BuiltinResourceManager::Get()->GetOrLoadResource<Texture2D>( PackagedResourceType::SMAA_AreaTexture );
		}

		m_RendererData.SMAAFinalPipeline = Ref<ComputePipeline>::Create( m_RendererData.SMAABlendingShader );
	
		m_RendererData.SMAAFinalMaterial = Ref<Material>::Create( m_RendererData.SMAABlendingShader, "SMAABlending" );

		m_RendererData.SMAAFinalMaterial->SetResource( "o_Output", m_RendererData.SMAAFinalOutImage );
		
		m_RendererData.SMAAFinalMaterial->SetSeparateImage( "u_InputColorTexture", m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ] );
		m_RendererData.SMAAFinalMaterial->SetSeparateImage( "u_EdgesTexture", m_RendererData.SMAAEdgeDetectionOutImage );
		m_RendererData.SMAAFinalMaterial->SetSeparateImage( "u_SearchTexture", m_RendererData.SMAASearchTexture );
		m_RendererData.SMAAFinalMaterial->SetSeparateImage( "u_AreaTexture", m_RendererData.SMAAAreaTexture );
		m_RendererData.SMAAFinalMaterial->SetResource( "s_LinearSampler", m_RendererData.GeneralUseLinearSampler );
	}

	void SceneRenderer::InitSMAAComposition()
	{
		if( m_RendererData.SMAACompPass )
			m_RendererData.SMAACompPass->Recreate();
		else
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "SMMA Comp Pass";
			PassSpec.LoadColor = true;
			PassSpec.Attachments = { ImageFormat::RGBA8 };

			m_RendererData.SMAACompPass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.SMAACompFB )
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];

			m_RendererData.SMAACompFB->Recreate( m_RendererData.Width, m_RendererData.Height, FBSpec );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.SMAACompPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;
			FBSpec.CreateDepth = false;
			FBSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];

			m_RendererData.SMAACompFB = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_RendererData.SMAACompositionShader )
		{
			m_RendererData.SMAACompositionShader = ShaderLibrary::Get().FindOrLoad( "SMAA-Composition", "content/shaders/SMAA-Composition.glsl" );
		}

		m_RendererData.SMAACompMaterial = Ref<Material>::Create( m_RendererData.SMAACompositionShader, "SMAABlending" );
		m_RendererData.SMAACompMaterial->SetResource( "u_SMAAOutTexture", m_RendererData.SMAAFinalOutImage );

		if( !m_RendererData.SMAACompPipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Late Composite (SMAA)";
			PipelineSpec.Shader = m_RendererData.SMAACompositionShader;
			PipelineSpec.RenderPass = m_RendererData.SMAACompPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			m_RendererData.SMAACompPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitGTAOPass()
	{
		m_RendererData.ClearGTAOResources();

		InitGTAOPrefilter();
		InitGTAOMainPass();
		InitGTAODenoisePass();
	}

	void SceneRenderer::InitGTAOPrefilter()
	{
		if( !m_RendererData.GTAOPrefilterShader )
		{
			m_RendererData.GTAOPrefilterShader = ShaderLibrary::Get().FindOrLoad( "GTAO-Prefilter", "content/shaders/GTAO-Prefilter.glsl" );
		}

		m_RendererData.GTAOPrefilterOutImage = Ref<Texture2D>::Create( ImageFormat::RED32F, m_RendererData.Width, m_RendererData.Height, nullptr, true, AddressingMode::ClampToEdge );

		m_RendererData.GTAOPrefilterPipeline = Ref<ComputePipeline>::Create( m_RendererData.GTAOPrefilterShader );

		m_RendererData.GTAOPrefilterMaterial = Ref<Material>::Create( m_RendererData.GTAOPrefilterShader, "GTAOPrefilter" );
		m_RendererData.GTAOPrefilterMaterial->SetSeparateImage( "u_InDepth", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.GTAOPrefilterMaterial->SetResource( "s_LinearSampler", m_RendererData.GeneralUseLinearSampler );

		m_RendererData.GTAOImageInfos.resize( m_RendererData.GTAOPrefilterOutImage->GetMipMapLevels() );

		uint32_t mip = 0;
		for( auto& rImageInfo : m_RendererData.GTAOImageInfos )
		{
			rImageInfo = m_RendererData.GTAOPrefilterOutImage->GetDescriptorInfo();
			rImageInfo.imageView = m_RendererData.GTAOPrefilterOutImage->GetOrCreateMipImageView( mip );

			SetDebugUtilsObjectName( std::format( "GTAO Prefilter mip{}", mip ), ( uint64_t ) rImageInfo.imageView, VK_OBJECT_TYPE_IMAGE_VIEW );

			const std::string descriptorName = std::format( "o_OutDepths{}", mip );

			m_RendererData.GTAOPrefilterMaterial->SetResourceWithVulkanInfo( 
				descriptorName, 
				m_RendererData.GTAOPrefilterOutImage, rImageInfo );
		
			++mip;
		}
	}

	void SceneRenderer::InitGTAOMainPass()
	{
		if( !m_RendererData.GTAOMainPassShader )
		{
			m_RendererData.GTAOMainPassShader = ShaderLibrary::Get().FindOrLoad( "GTAO-MainPass", "content/shaders/GTAO-MainPass.glsl" );
		}

		m_RendererData.GTAOEdgesImage = Ref<Image2D>::Create( 
			ImageFormat::RED8, 
			m_RendererData.Width, m_RendererData.Height, 
			1u, 1u, 1u, ImageTiling::Optimal, true );

		m_RendererData.GTAOEdgesImage->SetDebugName( "GTAOEdgesImage" );

		m_RendererData.GTAONoisyOut = Ref<Image2D>::Create( 
			ImageFormat::RED32F, 
			m_RendererData.Width, m_RendererData.Height, 
			1u, 1u, 1u, ImageTiling::Optimal, true );
		
		m_RendererData.GTAONoisyOut->SetDebugName( "GTAONoisyOut" );

		m_RendererData.GTAOMainPipeline = Ref<ComputePipeline>::Create( m_RendererData.GTAOMainPassShader );

		m_RendererData.GTAOMainMaterial = Ref<Material>::Create( m_RendererData.GTAOMainPassShader, "GTAOMainMat" );

		m_RendererData.GTAOMainMaterial->SetSeparateImage( "u_InDepthPreDepth", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.GTAOMainMaterial->SetSeparateImage( "u_InDepthGTAO", m_RendererData.GTAOPrefilterOutImage );
		m_RendererData.GTAOMainMaterial->SetResource( "s_PointSampler", m_RendererData.GeneralUsePointSampler );
	
		m_RendererData.GTAOMainMaterial->SetResource( "o_Edges", m_RendererData.GTAOEdgesImage );
		m_RendererData.GTAOMainMaterial->SetResource( "o_NoisyGTAO", m_RendererData.GTAONoisyOut );
	}

	void SceneRenderer::InitGTAODenoisePass()
	{
		if( !m_RendererData.GTAODenoiseShader )
		{
			m_RendererData.GTAODenoiseShader = ShaderLibrary::Get().FindOrLoad( "GTAO-Denoise", "content/shaders/GTAO-Denoise.glsl" );
		}

		m_RendererData.GTAODenoisePipeline = Ref<ComputePipeline>::Create( m_RendererData.GTAODenoiseShader );

		m_RendererData.GTAODenoisePassesInformation.reserve( m_RendererData.GTAODenoisePassCount );

		for( size_t i = 0; i < m_RendererData.GTAODenoisePassCount; ++i )
		{
			auto& rPassInfo = m_RendererData.GTAODenoisePassesInformation.emplace_back();
			rPassInfo.OutImage = Ref<Image2D>::Create( ImageFormat::RED32F, m_RendererData.Width, m_RendererData.Height, 1u, 1u, 1u, ImageTiling::Optimal, true );

			rPassInfo.OutImage->SetDebugName( std::format( "GTAO-DenoiseImageOut{}", i ) );

			rPassInfo.Material = Ref<Material>::Create( m_RendererData.GTAODenoiseShader, "GTAODenoise" );
			
			if( i == 0 )
			{
				rPassInfo.Material->SetSeparateImage( "u_GTAOIn", m_RendererData.GTAONoisyOut );
			}
			else
			{
				rPassInfo.Material->SetSeparateImage( "u_GTAOIn", m_RendererData.GTAODenoisePassesInformation[ i - 1 ].OutImage );
			}

			rPassInfo.Material->SetSeparateImage( "u_EdgesIn", m_RendererData.GTAOEdgesImage );
			rPassInfo.Material->SetResource( "s_LinearSampler", m_RendererData.GeneralUseLinearSampler );
			rPassInfo.Material->SetResource( "o_GTAOOut", rPassInfo.OutImage );
		}
	}

	static AtmosphereParameters CreateDefaultAtmosphereParameters()
	{
		AtmosphereParameters atmos;

		constexpr float kEarthBottomRadius = 6360.0f;
		constexpr float kEarthTopRadius = kEarthBottomRadius + 60.0f;
		constexpr float kEarthRayleighScaleHeight = 8.0f;
		constexpr float kEarthMieScaleHeight = 1.2f;

		struct DensityProfileLayer
		{
			float Width = 0.0f;
			float ExpTerm = 0.0f;
			float ExpScale = 0.0f;
			float LinearTerm = 0.0f;
			float ConstantTerm = 0.0f;
		};

		struct DensityProfile
		{
			DensityProfileLayer Layers[ 2 ];
		};

		atmos.BottomRadius = kEarthBottomRadius;
		atmos.TopRadius = kEarthTopRadius;
		atmos.GroundAlbedo = glm::vec3{ 0.3f };

		DensityProfile rayleighDP{};
		DensityProfile meDP{};
		DensityProfile absorptionDP{};

		rayleighDP.Layers[ 0 ] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
		rayleighDP.Layers[ 1 ] = { 0.0f, 1.0f, -1.0f / kEarthRayleighScaleHeight, 0.0f, 0.0f };

		meDP.Layers[ 0 ] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
		meDP.Layers[ 1 ] = { 0.0f, 1.0f, -1.0f / kEarthMieScaleHeight, 0.0f, 0.0f };

		absorptionDP.Layers[ 0 ] = { 25.0f, 0.0f, 0.0f,  1.0f / 15.0f, -2.0f / 3.0f };
		absorptionDP.Layers[ 1 ] = { 0.0f, 0.0f, 0.0f, -1.0f / 15.0f,  8.0f / 3.0f };

		std::memcpy( &atmos.RayleighDensity[ 0 ].x, &rayleighDP, sizeof( rayleighDP ) );
		std::memcpy( &atmos.MieDensity[ 0 ].x, &meDP, sizeof( meDP ) );
		std::memcpy( &atmos.AbsorptionDensity[ 0 ].x, &absorptionDP, sizeof( absorptionDP ) );

		glm::vec3 rayleighScattering = { 0.005802f, 0.013558f, 0.033100f };
		atmos.RayleighScatteringColor = glm::normalize( rayleighScattering );
		atmos.RayleighScatterLength = glm::length( rayleighScattering );

		glm::vec3 mieScattering = { 0.003996f, 0.003996f, 0.003996f };
		atmos.MieScatteringColor = glm::normalize( mieScattering );
		atmos.MieScatteringLength = glm::length( mieScattering );

		glm::vec3 mieExtinction = { 0.004440f, 0.004440f, 0.004440f };
		glm::vec3 mieAbs = ( mieExtinction - mieScattering );

		atmos.MieAbsLength = glm::length( mieAbs );
		atmos.MieAbsColor = glm::normalize( mieAbs );

		atmos.MiePhaseFunctionG = 0.8f;

		glm::vec3 absorptionExtinction = { 0.000650f, 0.001881f, 0.000085f };
		atmos.AbsorptionColor = glm::normalize( absorptionExtinction );
		atmos.AbsorptionLength = glm::length( absorptionExtinction );

		atmos.MultipleScatteringFactor = 1.0f;
		atmos.ViewRayMarchMaxSPP = 31;
		atmos.ViewRayMarchMinSPP = 14;

		return atmos;
	}

	static void GetSceneSun( const DirectionalLight& rLight, glm::vec3& rDirection, glm::vec3& rColor )
	{
		rDirection = glm::vec3( 0.3f, 0.6f, 0.4f );
		rColor = glm::vec3( 1.0f, 0.95f, 0.9f );

		if( glm::length( rLight.Direction ) > 1e-4f )
		{
			rDirection = rLight.Direction;
			rColor = rLight.Radiance * rLight.Intensity;
		}

		rDirection = glm::normalize( rDirection );
	}

	void SceneRenderer::InitVolumetricCloudsPass()
	{
		InitVolumetricCloudsTextureResources();
		InitAtmosSkyTransmittance();
		InitAtmosSkyView();

		InitVolumetricCloudsBasicNoise();
		InitVolumetricCloudsDetailedNoise();

		InitVolumetricCloudsRaymarch();
		InitVolumetricCloudsShadow();
	}

	void SceneRenderer::InitVolumetricCloudsShadow()
	{
		// 1024 texels over 40 km (CLOUD_SHADOW_EXTENT_M) is ~39 m per texel, plenty for soft cloud shadows.
		m_RendererData.CloudShadowMap = Ref<Image2D>::Create( ImageFormat::RED32F, 1024u, 1024u, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudShadowMap->SetDebugName( "CloudShadowMap" );

		if( !m_RendererData.CloudShadowShader )
		{
			m_RendererData.CloudShadowShader = ShaderLibrary::Get().FindOrLoad( "CloudsShadow", "content/shaders/CloudsShadow.glsl" );
		}

		m_RendererData.CloudShadowPipeline = Ref<ComputePipeline>::Create( m_RendererData.CloudShadowShader );

		m_RendererData.CloudShadowMaterial = Ref<Material>::Create( m_RendererData.CloudShadowShader, "CloudShadowMat" );
		m_RendererData.CloudShadowMaterial->SetResource( "u_WeatherMapTexture", m_RendererData.CloudsWeatherMap );
		m_RendererData.CloudShadowMaterial->SetResource( "u_CurlTexture", m_RendererData.CloudsCurlTexture );
		m_RendererData.CloudShadowMaterial->SetResource( "u_BasicNoiseTexture", m_RendererData.CloudsNoiseBasicOut );
		m_RendererData.CloudShadowMaterial->SetResource( "u_DetailNoiseTexture", m_RendererData.CloudsNoiseDetailedOut );
		m_RendererData.CloudShadowMaterial->SetResource( "o_CloudShadow", m_RendererData.CloudShadowMap );
	}

	void SceneRenderer::InitVolumetricCloudsBasicNoise()
	{
		m_RendererData.CloudsNoiseBasicOut = Ref<Image3D>::Create( ImageFormat::RED8, 128u, 128u, 128u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudsNoiseBasicOut->SetDebugName( "CloudsBasicNoise" );

		if( !m_RendererData.CloudsNoiseBasicShader )
		{
			m_RendererData.CloudsNoiseBasicShader = ShaderLibrary::Get().FindOrLoad( "CloudsBasicNoise", "content/shaders/CloudsBasicNoise.glsl" );
		}

		m_RendererData.CloudsNoiseBasicPipeline = Ref<ComputePipeline>::Create( m_RendererData.CloudsNoiseBasicShader );

		m_RendererData.CloudsNoiseBasicMaterial = Ref<Material>::Create( m_RendererData.CloudsNoiseBasicShader, "CloudsNoiseMat" );
		m_RendererData.CloudsNoiseBasicMaterial->SetResource( "o_Output", m_RendererData.CloudsNoiseBasicOut );

		constexpr size_t kLocalSize = 8llu;
		const auto imgSize = m_RendererData.CloudsNoiseBasicOut->GetWidth();

		// Execute to generate the noise.
		m_RendererData.CloudsNoiseBasicPipeline->Bind();
		m_RendererData.CloudsNoiseBasicPipeline->Execute( m_RendererData.CloudsNoiseBasicMaterial, 
			( uint32_t ) glm::ceil( ( ( imgSize + kLocalSize ) - 1u ) / kLocalSize ),
			( uint32_t ) glm::ceil( ( ( imgSize + kLocalSize ) - 1u ) / kLocalSize ),
			m_RendererData.CloudsNoiseBasicOut->GetDepth() );

		// TRANSITION: GENERAL TO SHADER READ ONLY
		//			   so that we can use it again in our shaders again
		//             we also want to make sure that we do it in the
		//			   same cmd buffer as the compute shader.
		m_RendererData.CloudsNoiseBasicOut->TransitionImageLayout( 
			m_RendererData.CloudsNoiseBasicPipeline->GetCommandBuffer(), 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		m_RendererData.CloudsNoiseBasicPipeline->Unbind();
	}

	void SceneRenderer::InitVolumetricCloudsDetailedNoise()
	{
		m_RendererData.CloudsNoiseDetailedOut = Ref<Image3D>::Create( ImageFormat::RED8, 64u, 64u, 64u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudsNoiseDetailedOut->SetDebugName( "CloudsDetailNoise" );

		if( !m_RendererData.CloudsNoiseDetailedShader )
		{
			m_RendererData.CloudsNoiseDetailedShader = ShaderLibrary::Get().FindOrLoad( "CloudsDetailNoise", "content/shaders/CloudsDetailNoise.glsl" );
		}

		m_RendererData.CloudsNoiseDetailedPipeline = Ref<ComputePipeline>::Create( m_RendererData.CloudsNoiseDetailedShader );

		m_RendererData.CloudsNoiseDetailedMaterial = Ref<Material>::Create( m_RendererData.CloudsNoiseDetailedShader, "CloudsNoiseMat" );
		m_RendererData.CloudsNoiseDetailedMaterial->SetResource( "o_Output", m_RendererData.CloudsNoiseDetailedOut );

		constexpr size_t kLocalSize = 8llu;
		const auto imgSize = m_RendererData.CloudsNoiseDetailedOut->GetWidth();

		// Execute to generate the noise.
		m_RendererData.CloudsNoiseDetailedPipeline->Bind();
		m_RendererData.CloudsNoiseDetailedPipeline->Execute( m_RendererData.CloudsNoiseDetailedMaterial,
			( uint32_t ) glm::ceil( ( ( imgSize + kLocalSize ) - 1 ) / kLocalSize ),
			( uint32_t ) glm::ceil( ( ( imgSize + kLocalSize ) - 1 ) / kLocalSize ),
			m_RendererData.CloudsNoiseDetailedOut->GetDepth() );

		// TRANSITION: GENERAL TO SHADER READ ONLY
		//			   so that we can use it again in our shaders again
		//             we also want to make sure that we do it in the
		//			   same cmd buffer as the compute shader.
		m_RendererData.CloudsNoiseDetailedOut->TransitionImageLayout(
			m_RendererData.CloudsNoiseDetailedPipeline->GetCommandBuffer(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		m_RendererData.CloudsNoiseDetailedPipeline->Unbind();
	}

	void SceneRenderer::InitVolumetricCloudsTextureResources()
	{
		m_RendererData.CloudsWeatherMap = BuiltinResourceManager::Get()->GetOrLoadResource<Texture2D>( PackagedResourceType::WeatherMapTexture );
	
		m_RendererData.CloudsCurlTexture = BuiltinResourceManager::Get()->GetOrLoadResource<Texture2D>( PackagedResourceType::CurlTexture );
	}

	void SceneRenderer::InitVolumetricCloudsRaymarch()
	{
		const uint32_t reconstructWidth  = std::max( 2u, m_RendererData.Width );
		const uint32_t reconstructHeight = std::max( 2u, m_RendererData.Height );
		const uint32_t traceWidth  = ( reconstructWidth + 1u ) / 2u;
		const uint32_t traceHeight = ( reconstructHeight + 1u ) / 2u;

		m_RendererData.CloudsRaymarchOut = Ref<Image2D>::Create( ImageFormat::RGBA16F, traceWidth, traceHeight, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudsRaymarchOut->SetDebugName( "CloudsRaymarchOut" );

		m_RendererData.CloudsDistanceOut = Ref<Image2D>::Create( ImageFormat::RED32F, traceWidth, traceHeight, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudsDistanceOut->SetDebugName( "CloudsDistanceOut" );

		m_RendererData.CloudsReconstructOut = Ref<Image2D>::Create( ImageFormat::RGBA16F, reconstructWidth, reconstructHeight, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudsReconstructOut->SetDebugName( "CloudsReconstructOut" );

		m_RendererData.CloudsHistory = Ref<Image2D>::Create( ImageFormat::RGBA16F, reconstructWidth, reconstructHeight, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.CloudsHistory->SetDebugName( "CloudsHistory" );

		m_RendererData.CloudsHistoryValid = false;

		if( !m_RendererData.CloudsRaymarchShader )
		{
			m_RendererData.CloudsRaymarchShader = ShaderLibrary::Get().FindOrLoad( "CloudsRender", "content/shaders/CloudsRender.glsl" );
		}

		m_RendererData.CloudsRaymarchPipeline = Ref<ComputePipeline>::Create( m_RendererData.CloudsRaymarchShader );

		m_RendererData.CloudsRaymarchMaterial = Ref<Material>::Create( m_RendererData.CloudsRaymarchShader, "CloudsRaymarchMat" );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_WeatherMapTexture", m_RendererData.CloudsWeatherMap );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_CurlTexture", m_RendererData.CloudsCurlTexture );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_BasicNoiseTexture", m_RendererData.CloudsNoiseBasicOut );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_DetailNoiseTexture", m_RendererData.CloudsNoiseDetailedOut );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_TransmittanceLut", m_RendererData.TransmittanceLutOut );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );

		m_RendererData.CloudsRaymarchMaterial->SetResource( "o_Clouds", m_RendererData.CloudsRaymarchOut );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "o_CloudsDistance", m_RendererData.CloudsDistanceOut );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_SkyViewLut", m_RendererData.SkyViewLutOut );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_MultiScatteringLut", m_RendererData.MultiScatteringLutOut );

		// Temporal reconstruction
		if( !m_RendererData.CloudsReconstructShader )
		{
			m_RendererData.CloudsReconstructShader = ShaderLibrary::Get().FindOrLoad( "CloudsReconstruct", "content/shaders/CloudsReconstruct.glsl" );
		}

		m_RendererData.CloudsReconstructPipeline = Ref<ComputePipeline>::Create( m_RendererData.CloudsReconstructShader );

		m_RendererData.CloudsReconstructMaterial = Ref<Material>::Create( m_RendererData.CloudsReconstructShader, "CloudsReconstructMat" );
		m_RendererData.CloudsReconstructMaterial->SetResource( "u_CloudsCurrent", m_RendererData.CloudsRaymarchOut );
		m_RendererData.CloudsReconstructMaterial->SetResource( "u_CloudsDistance", m_RendererData.CloudsDistanceOut );
		m_RendererData.CloudsReconstructMaterial->SetResource( "u_CloudsHistory", m_RendererData.CloudsHistory );
		m_RendererData.CloudsReconstructMaterial->SetResource( "o_CloudsOut", m_RendererData.CloudsReconstructOut );
	}

	void SceneRenderer::InitAtmosSkyTransmittance()
	{
		m_RendererData.TransmittanceLutOut = Ref<Image2D>::Create( ImageFormat::RGBA16F, 256u, 64u, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.TransmittanceLutOut->SetDebugName( "SkyTransmittanceLut" );
		m_RendererData.AtmosphereTransmittanceValid = false;

		if( !m_RendererData.TransmittanceLutShader )
		{
			m_RendererData.TransmittanceLutShader = ShaderLibrary::Get().FindOrLoad( "SkyAtmosphereTransmittanceLut", "content/shaders/SkyAtmosphereTransmittanceLut.glsl" );
		}

		m_RendererData.TransmittanceLutPipeline = Ref<ComputePipeline>::Create( m_RendererData.TransmittanceLutShader );

		m_RendererData.TransmittanceLutMaterial = Ref<Material>::Create( m_RendererData.TransmittanceLutShader, "SkyTrnsLut" );
		m_RendererData.TransmittanceLutMaterial->SetResource( "o_OutTransmittanceLut", m_RendererData.TransmittanceLutOut );
		m_RendererData.TransmittanceLutMaterial->SetResource( "u_TransmittanceLut", m_RendererData.TransmittanceLutOut );
		m_RendererData.TransmittanceLutMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
	}

	void SceneRenderer::InitAtmosSkyView()
	{
		// Multiple scattering LUT, 32x32 as is specified Hillaire 2020.
		m_RendererData.MultiScatteringLutOut = Ref<Image2D>::Create( ImageFormat::RGBA16F, 32u, 32u, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.MultiScatteringLutOut->SetDebugName( "SkyMultiScatteringLut" );
		m_RendererData.AtmosphereMultiScatteringValid = false;

		if( !m_RendererData.MultiScatteringLutShader )
		{
			m_RendererData.MultiScatteringLutShader = ShaderLibrary::Get().FindOrLoad( "SkyAtmosphereMultiScatteringLut", "content/shaders/SkyAtmosphereMultiScatteringLut.glsl" );
		}

		m_RendererData.MultiScatteringLutPipeline = Ref<ComputePipeline>::Create( m_RendererData.MultiScatteringLutShader );

		m_RendererData.MultiScatteringLutMaterial = Ref<Material>::Create( m_RendererData.MultiScatteringLutShader, "SkyMultiScatLutMat" );
		m_RendererData.MultiScatteringLutMaterial->SetResource( "o_MultiScatteringLut", m_RendererData.MultiScatteringLutOut );
		m_RendererData.MultiScatteringLutMaterial->SetResource( "u_TransmittanceLut", m_RendererData.TransmittanceLutOut );
		m_RendererData.MultiScatteringLutMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );

		// Sky-view LUT, 192x108 as in Hillaire 2020.
		m_RendererData.SkyViewLutOut = Ref<Image2D>::Create( ImageFormat::RGBA16F, 192u, 108u, 1u, 1u, 1u, ImageTiling::Optimal, true );
		m_RendererData.SkyViewLutOut->SetDebugName( "SkyViewLut" );

		if( !m_RendererData.SkyViewLutShader )
		{
			m_RendererData.SkyViewLutShader = ShaderLibrary::Get().FindOrLoad( "SkyAtmosphereSkyViewLut", "content/shaders/SkyAtmosphereSkyViewLut.glsl" );
		}

		m_RendererData.SkyViewLutPipeline = Ref<ComputePipeline>::Create( m_RendererData.SkyViewLutShader );

		m_RendererData.SkyViewLutMaterial = Ref<Material>::Create( m_RendererData.SkyViewLutShader, "SkyViewLutMat" );
		m_RendererData.SkyViewLutMaterial->SetResource( "o_SkyViewLut", m_RendererData.SkyViewLutOut );
		m_RendererData.SkyViewLutMaterial->SetResource( "u_TransmittanceLut", m_RendererData.TransmittanceLutOut );
		m_RendererData.SkyViewLutMaterial->SetResource( "u_MultiScatteringLut", m_RendererData.MultiScatteringLutOut );
		m_RendererData.SkyViewLutMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );

		// Full screen sky draw in the geometry pass, set up like the skybox.
		if( !m_RendererData.SkyAtmosphereShader )
		{
			m_RendererData.SkyAtmosphereShader = ShaderLibrary::Get().FindOrLoad( "SkyAtmosphere", "content/shaders/SkyAtmosphere.glsl" );
		}

		m_RendererData.SkyAtmosphereMaterial = Ref<Material>::Create( m_RendererData.SkyAtmosphereShader, "SkyAtmosphereMat" );
		m_RendererData.SkyAtmosphereMaterial->SetResource( "u_SkyViewLut", m_RendererData.SkyViewLutOut );
		m_RendererData.SkyAtmosphereMaterial->SetResource( "u_TransmittanceLut", m_RendererData.TransmittanceLutOut );

		if( !m_RendererData.SkyAtmospherePipeline )
		{
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "SkyAtmosphere";
			PipelineSpec.Shader = m_RendererData.SkyAtmosphereShader;
			PipelineSpec.RenderPass = m_RendererData.GeometryPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			m_RendererData.SkyAtmospherePipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::RenderGrid()
	{
		SAT_PF_EVENT();

		// On dist, Scene Visualisation Options do not exist!
#if !defined(SAT_DIST)
		// Should we show the grid?
		const auto& rVisOptions = m_pScene->GetVisualisationOptions();
		const bool isRuntimeActive = m_pScene->IsRuntimeActive();
		if( !( isRuntimeActive ? rVisOptions.ShowGridOnRuntime : rVisOptions.ShowGrid ) )
		{
			return;
		}
#endif
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		// Set UB Data.
		const glm::mat4 trans = glm::rotate( glm::mat4( 1.0f ), glm::radians( 90.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( 16.0f ) );

		UBGridMatrices GridMatricesObject = {};
		GridMatricesObject.Transform = trans;
		GridMatricesObject.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		GridMatricesObject.Res = 0.025f;
		GridMatricesObject.Scale = 16.025f;

		m_RendererData.GridMaterials[ frame ]->UploadDataToUB( 0u, &GridMatricesObject, sizeof( UBGridMatrices ) );

		Renderer::Get()->SubmitFullscreenQuad(
			m_RendererData.CommandBuffer,
			m_RendererData.GridPipeline, 
			m_RendererData.GridMaterials[ frame ], 
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer 
		);
	}

	void SceneRenderer::RenderSkybox()
	{
		SAT_PF_EVENT();

		// Is this really needed?
		if( !m_pScene )
			return;

		auto& rSceneEnvironment = m_RendererData.SceneEnvironment;

		// The cubemap environment is still needed for image based lighting, so keep it up to
		// date even when the physical sky is what gets drawn.
		const bool hasCubemapSky = !( rSceneEnvironment.Azimuth == 0 && rSceneEnvironment.Inclination == 0 && rSceneEnvironment.Turbidity == 0 );

		// We have no skybox.
		if( !hasCubemapSky )
		{
			// I don't really like this.
			// TODO: Come back to this.
			if( rSceneEnvironment.IrradianceMap && rSceneEnvironment.RadianceMap )
			{
				rSceneEnvironment.RadianceMap = nullptr;
				rSceneEnvironment.IrradianceMap = nullptr;
			}
		}
		else
		{
			// Skybox values where set, but check if out textures exist or update them accordingly.
			CheckInvalidSkybox();
		}

		// Physical sky from the directional light replaces the cubemap draw.
		if( m_pScene->m_Lights.DirectionalLights[ 0 ].Sky.Enabled )
		{
			RenderPhysicalSky();
			return;
		}

		if( !hasCubemapSky )
			return;

		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;
		m_RendererData.SkyboxMaterial->SetResource( "u_CubeTexture", m_RendererData.SceneEnvironment.IrradianceMap );

		UBSkyboxMatrices SkyboxMatricesObject = {};
		SkyboxMatricesObject.InverseVP = glm::inverse( m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix );

		struct ub_Data
		{
			float SkyboxLod;
			// 0.19
			float Intensity;
		} u_Data;

		u_Data = {};
		// TODO: Maybe we could of used the skylight entity for this data?
		u_Data.SkyboxLod = m_RendererData.SkyboxLod;
		u_Data.Intensity = m_RendererData.Intensity;

		m_RendererData.SkyboxMaterial->UploadDataToUB( 0, &SkyboxMatricesObject, sizeof( SkyboxMatricesObject ) );
		m_RendererData.SkyboxMaterial->UploadDataToUB( 2, &u_Data, sizeof( u_Data ) );

		Renderer::Get()->SubmitFullscreenQuad( CommandBuffer, 
			m_RendererData.SkyboxPipeline, 
			m_RendererData.SkyboxMaterial,
			m_RendererData.QuadIndexBuffer, 
			m_RendererData.QuadVertexBuffer );
	}

	void SceneRenderer::RenderPhysicalSky()
	{
		SAT_PF_EVENT();

		const auto& rDirLight = m_pScene->m_Lights.DirectionalLights[ 0 ];
		const AtmosphereSkySettings& rSky = rDirLight.Sky;

		glm::vec3 sunDirection, sunColor;
		GetSceneSun( rDirLight, sunDirection, sunColor );

		const AtmosphereParameters atmosphere = CreateDefaultAtmosphereParameters();
		const glm::vec3 cameraPosMeters = glm::vec3( glm::inverse( m_RendererData.CurrentCamera.ViewMatrix )[ 3 ] );

		struct USkyData
		{
			glm::mat4 InverseViewProj;
			glm::vec4 CameraPosKm;
			glm::vec4 SunDirection;
			glm::vec4 SkyLight;
			glm::vec4 SunDisk;
			glm::vec4 Radii;
			glm::vec4 GroundAlbedo;
		} u_SkyData{};

		const float sunRadius = glm::radians( glm::max( rSky.SunDiskSizeDeg, 0.01f ) * 0.5f );

		u_SkyData.InverseViewProj = glm::inverse( m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix );
		u_SkyData.CameraPosKm = glm::vec4( cameraPosMeters * 0.001f + glm::vec3( 0.0f, atmosphere.BottomRadius, 0.0f ), 1.0f );
		u_SkyData.SunDirection = glm::vec4( sunDirection, 0.0f );
		u_SkyData.SkyLight = glm::vec4( sunColor * rSky.SunIlluminance * rSky.Intensity, 0.0f );
		u_SkyData.SunDisk = glm::vec4( glm::cos( sunRadius ), rSky.SunDiskIntensity, 0.0f, 0.0f );
		u_SkyData.Radii = glm::vec4( atmosphere.BottomRadius, atmosphere.TopRadius, 0.0f, 0.0f );
		u_SkyData.GroundAlbedo = glm::vec4( glm::clamp( rSky.GroundAlbedo, glm::vec3( 0.0f ), glm::vec3( 1.0f ) ), glm::clamp( rSky.HorizonFill, 0.0f, 1.0f ) );

		m_RendererData.SkyAtmosphereMaterial->UploadDataToUB( 3, &u_SkyData, sizeof( USkyData ) );

		Renderer::Get()->SubmitFullscreenQuad( m_RendererData.CommandBuffer,
			m_RendererData.SkyAtmospherePipeline,
			m_RendererData.SkyAtmosphereMaterial,
			m_RendererData.QuadIndexBuffer,
			m_RendererData.QuadVertexBuffer );
	}

	void SceneRenderer::CheckInvalidSkybox()
	{
		if( !m_pScene )
			return;

		// Invalid skybox, maybe null from loading a new scene? This only happens on the first frames so this is a hack.
		if( m_RendererData.SceneEnvironment.IrradianceMap == nullptr && m_RendererData.SceneEnvironment.RadianceMap == nullptr )
		{
			SharedPtr<Entity> SkylightEntity = nullptr;

			auto view = m_pScene->GetAllEntitiesWith< SkylightComponent >();

			for( const auto& e : view )
			{
				SkylightEntity = e;
			}

			if( SkylightEntity )
			{
				auto& Skylight = SkylightEntity->GetComponent< SkylightComponent >();

				if( !Skylight.DynamicSky )
					return;

				if( Skylight.DynamicSky && !m_RendererData.SceneEnvironment.IrradianceMap && !m_RendererData.SceneEnvironment.RadianceMap )
				{
					m_RendererData.SceneEnvironment.Turbidity = Skylight.Turbidity;
					m_RendererData.SceneEnvironment.Azimuth = Skylight.Azimuth;
					m_RendererData.SceneEnvironment.Inclination = Skylight.Inclination;

					// We can call this directly, we should be on the render thread.
					Ref<TextureCube> map = CreateDymanicSky();

					m_RendererData.SceneEnvironment.IrradianceMap = map;
					m_RendererData.SceneEnvironment.RadianceMap = map;
				}
			}
		}
	}

	void SceneRenderer::UpdateCascades( const glm::vec3& Direction )
	{
		const auto& viewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		float cascadeSplits[ SHADOW_CASCADE_COUNT ];

		constexpr float kNearClip = 0.1F;
		constexpr float kFarClip = 1000.0F;
		constexpr float kClipRange = kFarClip - kNearClip;

		constexpr float kMinZ = kNearClip;
		constexpr float kMaxZ = kNearClip + kClipRange;

		constexpr float kRange = kMaxZ - kMinZ;
		constexpr float kRatio = kMaxZ / kMinZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for( uint32_t i = 0; i < SHADOW_CASCADE_COUNT; ++i )
		{
			const float p = ( i + 1 ) / static_cast< float >( SHADOW_CASCADE_COUNT );
			const float log = kMinZ * std::pow( kRatio, p );
			const float uniform = kMinZ + kRange * p;
			const float d = m_RendererData.CascadeSplitLambda * ( log - uniform ) + uniform;
			cascadeSplits[ i ] = ( d - kNearClip ) / kClipRange;
		}

		cascadeSplits[ 3 ] = 0.3f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for( uint32_t i = 0; i < SHADOW_CASCADE_COUNT; ++i )
		{
			float splitDist = cascadeSplits[ i ];

			glm::vec3 frustumCorners[ 8 ] =
			{
				glm::vec3( -1.0f,  1.0f, -1.0f ),
				glm::vec3( 1.0f,  1.0f, -1.0f ),
				glm::vec3( 1.0f, -1.0f, -1.0f ),
				glm::vec3( -1.0f, -1.0f, -1.0f ),
				glm::vec3( -1.0f,  1.0f,  1.0f ),
				glm::vec3( 1.0f,  1.0f,  1.0f ),
				glm::vec3( 1.0f, -1.0f,  1.0f ),
				glm::vec3( -1.0f, -1.0f,  1.0f ),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse( viewProjection );
			for( uint32_t i = 0; i < 8; ++i )
			{
				glm::vec4 invCorner = invCam * glm::vec4( frustumCorners[ i ], 1.0f );
				frustumCorners[ i ] = invCorner / invCorner.w;
			}

			for( uint32_t i = 0; i < 4; ++i )
			{
				glm::vec3 dist = frustumCorners[ i + 4 ] - frustumCorners[ i ];
				frustumCorners[ i + 4 ] = frustumCorners[ i ] + ( dist * splitDist );
				frustumCorners[ i ] = frustumCorners[ i ] + ( dist * lastSplitDist );
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3( 0.0f );
			for( uint32_t i = 0; i < 8; ++i )
				frustumCenter += frustumCorners[ i ];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for( uint32_t i = 0; i < 8; ++i )
			{
				float distance = glm::length( frustumCorners[ i ] - frustumCenter );
				radius = glm::max( radius, distance );
			}
			radius = std::ceil( radius * 16.0f ) / 16.0f;

			glm::vec3 maxExtents = glm::vec3( radius );
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -Direction;
			glm::mat4 lightViewMatrix = glm::lookAt( frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3( 0.0f, 0.0f, 1.0f ) );
			glm::mat4 lightOrthoMatrix = glm::ortho( minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + -15.0f, maxExtents.z - minExtents.z + 15.0f );

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = kShadowMapSize;
			glm::vec4 shadowOrigin = ( shadowMatrix * glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) * ShadowMapResolution * 0.5f;
			glm::vec4 roundedOrigin = glm::round( shadowOrigin );
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[ 3 ] += roundOffset;

			// Store split distance and matrix in cascade
			m_RendererData.ShadowCascades[ i ].SplitDepth = ( kNearClip + splitDist * kClipRange ) * -1.0f;
			m_RendererData.ShadowCascades[ i ].ViewProjection = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[ i ];
		}
	}

	void SceneRenderer::CreateGridComponents()
	{
		// Create fullscreen quad.
		auto [vertex, index] = Renderer::Get()->CreateFullscreenQuad();
		
		m_RendererData.QuadIndexBuffer = index;
		m_RendererData.QuadVertexBuffer = vertex;

		if( !m_RendererData.GridShader )
		{
			m_RendererData.GridShader = ShaderLibrary::Get().FindOrLoad( "Grid", "content/shaders/Grid.glsl" );
		
			for( auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
			{
				m_RendererData.GridMaterials[ i ] = Ref<Material>::Create( m_RendererData.GridShader, "Grid" );
			}
		}

		if( !m_RendererData.GridPipeline )
		{
			// Grid pipeline spec.
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Grid";
			PipelineSpec.Shader = m_RendererData.GridShader;
			PipelineSpec.RenderPass = m_RendererData.GeometryPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::None;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};

			m_RendererData.GridPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Skybox
	//////////////////////////////////////////////////////////////////////////

	void SceneRenderer::CreateSkyboxComponents()
	{
		// Create skybox shader.
		if( !m_RendererData.SkyboxShader && !m_RendererData.PreethamShader )
		{
			m_RendererData.SkyboxShader = ShaderLibrary::Get().FindOrLoad( "Skybox", "content/shaders/Skybox.glsl" );
			m_RendererData.PreethamShader = ShaderLibrary::Get().FindOrLoad( "Skybox-Compute", "content/shaders/Skybox-Compute.glsl" );

			m_RendererData.SkyboxMaterial = Ref<Material>::Create( m_RendererData.SkyboxShader, "SkyboxComposite" );
			m_RendererData.PreethamMaterial = Ref<Material>::Create( m_RendererData.PreethamShader, "PreethamMat" );
		}

		if( !m_RendererData.SkyboxPipeline )
		{
			// Create pipeline.
			PipelineSpecification PipelineSpec = {};
			PipelineSpec.Name = "Skybox";
			PipelineSpec.Shader = m_RendererData.SkyboxShader;
			PipelineSpec.RenderPass = m_RendererData.GeometryPass;
			PipelineSpec.UseDepthTest = true;
			PipelineSpec.CullMode = CullMode::Back;
			PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
			PipelineSpec.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			m_RendererData.SkyboxPipeline = Ref<Pipeline>::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitGeneralUseComponents()
	{
		SamplerSpecification samplerSpec;
		samplerSpec.MinFilter = samplerSpec.MagFilter = SamplerFilter::Linear;
		samplerSpec.MipFilter = SamplerFilter::Linear;
		samplerSpec.AddressingMode = AddressingMode::ClampToEdge;
		samplerSpec.CompareOperation = CompareOp::Less;
		samplerSpec.MinLod = 0.0f;
		samplerSpec.MaxLod = FLT_MAX;
		samplerSpec.DebugName = "GenUse-Linear";

		{
			// We don't know the max anisotropy level, so we'll need to get it from the properties of the physical device.
			// We do this as this is the best way to get the max anisotropy level as it can be different on other devices.
			VkPhysicalDeviceProperties Properties = {};
			vkGetPhysicalDeviceProperties( VulkanContext::Get()->GetPhysicalDevice(), &Properties );

			samplerSpec.MaxAnisotropy = glm::min( Properties.limits.maxSamplerAnisotropy, 8.0f );
		}

		m_RendererData.GeneralUseLinearSampler = Ref<Sampler>::Create( samplerSpec );

		samplerSpec.DebugName = "GenUse-Nearest-Point";
		samplerSpec.MinFilter = samplerSpec.MagFilter = samplerSpec.MipFilter = SamplerFilter::Nearest;
		m_RendererData.GeneralUsePointSampler = Ref<Sampler>::Create( samplerSpec );
	}

	void SceneRenderer::ImGuiRender()
	{
#if !defined(SAT_DIST)
		SAT_PF_EVENT();

		ImGui::Text( "Viewport size, %" PRIu32 ", %" PRIu32, m_RendererData.Width, m_RendererData.Height );
		ImGui::Text( "Viewport pixels, %" PRIu32, m_RendererData.Width * m_RendererData.Height );

		ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );

		if( Auxiliary::TreeNode( "Stats", true ) )
		{
			const auto FrameTimings = Renderer::Get()->GetFrameTimings();

			float shadowPassTime = 0.0f;

			if( m_RendererData.EnableShadows )
			{
				for( uint32_t i = 0; i < SHADOW_CASCADE_COUNT; ++i )
				{
					shadowPassTime += m_RendererData.ShadowMapTimers[ i ].ElapsedMilliseconds();
				}
			}

			ImGui::Text( "Renderer::BeginFrame: %.3f ms", FrameTimings.first );

			ImGui::Text( "SceneRenderer::PreDepthPass: %.3f ms", m_RendererData.PreDepthTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::ShadowMapPass: %.3f ms", shadowPassTime );

			ImGui::Text( "SceneRenderer::LightCulling: %.4f ms", m_RendererData.LightCullingTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::Atmosphere: %.3f ms", m_RendererData.AtmosphereTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::AtmosphereVC: %.3f ms", m_RendererData.AtmosphereVCTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::GeometryPass: %.3f ms", m_RendererData.GeometryPassTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::BloomPass: %.3f ms", m_RendererData.BloomTimer.ElapsedMilliseconds() );

			switch( m_AOTechnique )
			{
				default:
				case AOTechnique::None:
					ImGui::Text( "SceneRenderer::NullAO: 0.000 ms" );
					break;

				case Saturn::AOTechnique::SSAO:
					ImGui::Text( "SceneRenderer::SSAOPass: %.3f ms", m_RendererData.SSAOTimer.ElapsedMilliseconds() );
					break;

				case AOTechnique::HBAO:
					ImGui::Text( "SceneRenderer::HBAOPass: %.3f ms", m_RendererData.HBAOTimer.ElapsedMilliseconds() );
					break;

				case AOTechnique::GTAO:
					ImGui::Text( "SceneRenderer::GTAOPass: %.3f ms", m_RendererData.GTAOTimer.ElapsedMilliseconds() );
					break;
			}

			ImGui::Text( "SceneRenderer::SceneComposite: %.2f ms", m_RendererData.SceneCompPPTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::SMAAPass: %.2f ms", m_RendererData.SMAAPassTimer.ElapsedMilliseconds() );

			if( m_AluraRenderer )
				ImGui::Text( "AluraRenderer::RenderProper: %.2f ms", m_AluraRenderer->GetFrameTime() );

			ImGui::Text( "Renderer::EndFrame - Queue Present: %.2f ms", Renderer::Get()->GetQueuePresentTime() );
			ImGui::Text( "Renderer::EndFrame - Queue Wait: %.2f ms", Renderer::Get()->GetQueueWaitTime() );

			ImGui::Text( "Renderer::EndFrame - Total: %.2f ms", FrameTimings.second );

			ImGui::Text( "Total (RenderThread::Execute): %.2f ms", RenderThread::Get().GetWaitTime() );
			ImGui::Text( "Total : %.2f ms", Application::Get()->Time().Milliseconds() );

			Auxiliary::EndTreeNode();
		}

		// TEMP: Move to skylight entity.
		if( Auxiliary::TreeNode( "Environment", false ) )
		{
			ImGui::DragFloat( "Skybox Lod", &m_RendererData.SkyboxLod, 0.1f, 0.0f, 1000.0f );
			ImGui::DragFloat( "Intensity", &m_RendererData.Intensity, 0.1f, 0.0f, 1000.0f );

			Auxiliary::EndTreeNode();
		}

		if( Auxiliary::TreeNode( "Scene renderer data", true ) )
		{
			if( Auxiliary::TreeNode( "Ambient Occlusion", true ) )
			{
				ImGui::BeginHorizontal( "##technique" );
				
				ImGui::Text( "AO Technique" );

				ImGui::Spring();

				const auto oldAOTechnique = m_AOTechnique;

				const char* pDisplayNames[ 4 ] = { "None", "SSAO", "HBAO", "GTAO" };

				if( ImGui::BeginCombo( "##combotechao", pDisplayNames[ ( uint8_t ) m_AOTechnique ] ) )
				{
					if( ImGui::Selectable( "SSAO" ) )
					{
						m_AOTechnique = AOTechnique::SSAO;
					}

#if SAT_RENDERER_WITH_HBAO == 1
					if( ImGui::Selectable( "HBAO" ) )
					{
						m_AOTechnique = AOTechnique::HBAO;
					}
#endif

					if( ImGui::Selectable( "GTAO" ) )
					{
						m_AOTechnique = AOTechnique::GTAO;
					}

					if( ImGui::Selectable( "None" ) )
					{
						m_AOTechnique = AOTechnique::None;
					}

					ImGui::EndCombo();
				}

				ImGui::EndHorizontal();

				switch( m_AOTechnique )
				{
					case AOTechnique::HBAO:
					case AOTechnique::SSAO:
					{
						ImGui::Text( "Nothing here yet!" );
					} break;
					
					case AOTechnique::GTAO:
					{
						Auxiliary::DrawFloatControl( "Effect Radius", 
							m_RendererData.GTAOEffectRadius, 
							0.1f, 10.0f, true, 175.0f );

						Auxiliary::DrawFloatControl( "Effect Falloff range", 
							m_RendererData.GTAOEffectFalloffRange, 
							0.1f, 10.0f, true, 175.0f );

						Auxiliary::DrawFloatControl( "Radius Multiplier", 
							m_RendererData.GTAORadiusMultiplier, 
							0.1f, 10.0f, true, 175.0f );
					} break;

					case AOTechnique::None:
					default:
						break;
				}

				if( oldAOTechnique != m_AOTechnique )
				{
					InitAO( oldAOTechnique );
				}

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Shadow settings", true ) )
			{
				Auxiliary::DrawBoolControl( "Enable", m_RendererData.EnableShadows, true, 175.0f );

				Auxiliary::DrawFloatControl( "Cascade Split Lambda", m_RendererData.CascadeSplitLambda, 0.1f, 10.0f, true, 175.0f );
				Auxiliary::DrawFloatControl( "Cascade Near Plane", m_RendererData.CascadeNearPlaneOffset, 0.1f, 10000.0f, true, 175.0f );
				Auxiliary::DrawFloatControl( "Cascade Far Plane", m_RendererData.CascadeFarPlaneOffset, 0.1f, -10000.0f, true, 175.0f );

				static int index = 0;
				auto framebuffer = m_RendererData.ShadowCascades[ index ].Framebuffer->GetDepthAttachmentResource();

				const float size = ImGui::GetContentRegionAvail().x;
				ImGui::SliderInt( "##cascade_dt", &index, 0, 3 );

				Auxiliary::Image( framebuffer, ( uint32_t ) index, { size, size }, { 0, 1 }, { 1, 0 } );

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Bloom settings", false ) )
			{
				static int index = 0;
				static int MipIndex = 0;
				auto& img = m_RendererData.BloomTextures[ index ].Texture;

				if( Auxiliary::DrawBoolControl( "Enable", m_RendererData.EnableBloom ) ) 
				{
					DisableOrEnableBloom();
				}

				Auxiliary::DisabledFlag disabledIf( !m_RendererData.EnableBloom );

				Auxiliary::DrawIntControl( "Stage", index, 0, 2 );
				Auxiliary::DrawIntControl( "Mip", MipIndex, 0, img->GetMipMapLevels() - 2 );
				Auxiliary::DrawFloatControl( "Bloom Threshold", m_RendererData.BloomThreshold, 0.0f, 100.0f );

				const float size = ImGui::GetContentRegionAvail().x;
				Auxiliary::Image( img, MipIndex, { size, size }, { 0, 1 }, { 1, 0 } );

				disabledIf.Pop();

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Accessibility", false ) )
			{
				ImGui::BeginHorizontal( "##colrblind" );

				ImGui::Text( "Colour blind correction" );

				ImGui::Spring();

				const auto oldMode = m_ColourBlindMode;

				const char* pDisplayNames[ 4 ] = { "None", "Protanope", "Deuteranope", "Tritanope" };

				if( ImGui::BeginCombo( "##comboclb", pDisplayNames[ ( uint8_t ) m_ColourBlindMode ] ) )
				{
					if( ImGui::Selectable( "Protanope" ) )
					{
						m_ColourBlindMode = ColourBlindMode::Protanope;
					}

					if( ImGui::Selectable( "Deuteranope" ) )
					{
						m_ColourBlindMode = ColourBlindMode::Deuteranope;
					}

					if( ImGui::Selectable( "Tritanope" ) )
					{
						m_ColourBlindMode = ColourBlindMode::Tritanope;
					}

					if( ImGui::Selectable( "None" ) )
					{
						m_ColourBlindMode = ColourBlindMode::None;
					}

					ImGui::EndCombo();
				}

				ImGui::EndHorizontal();

				Auxiliary::EndTreeNode();
			}

			Auxiliary::EndTreeNode();
		}
#endif
	}

	void SceneRenderer::SetCurrentScene( Scene* pScene )
	{
		if( pScene == nullptr ) 
		{
			m_pScene = nullptr;
			return;
		}

		m_pScene = pScene;

		// Reset now, just incase the new scene doesn't have skylight.
		m_RendererData.SceneEnvironment.Turbidity = 0.0f;
		m_RendererData.SceneEnvironment.Azimuth = 0.0f;
		m_RendererData.SceneEnvironment.Inclination = 0.0f;

		// Find the skylight entity and set the turbidity, azimuth, inclination.
		auto view = m_pScene->GetAllEntitiesWith<SkylightComponent>();

		for( auto& entity : view )
		{
			auto& skylight = entity->GetComponent<SkylightComponent>();

			m_RendererData.SceneEnvironment.Turbidity = skylight.Turbidity;
			m_RendererData.SceneEnvironment.Azimuth = skylight.Azimuth;
			m_RendererData.SceneEnvironment.Inclination = skylight.Inclination;
		}
	}

	static AABB TransformAABB( const AABB& rAABB, const glm::mat4& rTransform )
	{
		// Get all 8 corners of the AABB
		glm::vec4 corners[ 8 ] = 
		{
			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Min.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Min.y, rAABB.Max.z, 1.0f },

			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Min.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Min.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4{ rAABB.Max.x, rAABB.Min.y, rAABB.Min.z, 1.0f }
		};

		glm::vec3 newMin( corners[ 0 ] );
		glm::vec3 newMax( corners[ 0 ] );

		for( int i = 0; i < 8; ++i )
		{
			const glm::vec3 transformed = glm::vec3( corners[ i ] );
			newMin = glm::min( newMin, transformed );
			newMax = glm::max( newMax, transformed );
		}

		return { newMin, newMax };
	}

	void SceneRenderer::SubmitStaticMesh( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform )
	{
		SAT_PF_EVENT();

		const auto& id = mesh->ID;

		uint32_t instanceOffset = 0;

		const auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); ++i )
		{
			const glm::mat4 submeshTransform = transform * submeshes[ i ].Transform;

			const AABB submeshAABB = submeshes[ i ].BoundingBox;
			const AABB transformedAABB = TransformAABB( submeshAABB, submeshTransform );

			if( m_RendererData.CurrentCamera.pCamera->CameraFrustumIntersectsAABB( transformedAABB ) )
			{
				StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i };

				// Submit for rendering
				auto& rDrawCommand = m_DrawList[ key ];
				rDrawCommand.Mesh = mesh;
				rDrawCommand.SubmeshIndex = ( uint32_t ) i;
				instanceOffset = rDrawCommand.Instances;
				++rDrawCommand.Instances;
				rDrawCommand.InstanceOffset = instanceOffset;

				auto& rShadowDrawCommand = m_ShadowMapDrawList[ key ];
				rShadowDrawCommand.Mesh = mesh;
				rShadowDrawCommand.SubmeshIndex = ( uint32_t ) i;
				++rShadowDrawCommand.Instances;
				rShadowDrawCommand.InstanceOffset = instanceOffset;

				auto& data = m_RendererData.MeshTransforms[ key ].Data.emplace_back();
				data.TransfromBufferR[ 0 ] = {
					submeshTransform[ 0 ][ 0 ], submeshTransform[ 1 ][ 0 ], submeshTransform[ 2 ][ 0 ], submeshTransform[ 3 ][ 0 ]
				};
				data.TransfromBufferR[ 1 ] = {
					submeshTransform[ 0 ][ 1 ], submeshTransform[ 1 ][ 1 ], submeshTransform[ 2 ][ 1 ], submeshTransform[ 3 ][ 1 ]
				};
				data.TransfromBufferR[ 2 ] = {
					submeshTransform[ 0 ][ 2 ], submeshTransform[ 1 ][ 2 ], submeshTransform[ 2 ][ 2 ], submeshTransform[ 3 ][ 2 ]
				};
				data.TransfromBufferR[ 3 ] = {
					submeshTransform[ 0 ][ 3 ], submeshTransform[ 1 ][ 3 ], submeshTransform[ 2 ][ 3 ], submeshTransform[ 3 ][ 3 ]
				};
			}
		}
	}

	void SceneRenderer::SubmitDynamicMesh( SharedPtr<Entity> entity, Ref<SkeletalMesh> mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms )
	{
		SAT_PF_EVENT();

		const auto& id = mesh->ID;

		auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); ++i )
		{
			const glm::mat4 submeshTransform = transform * submeshes[ i ].Transform;

			const AABB submeshAABB = submeshes[ i ].BoundingBox;
			const AABB transformedAABB = TransformAABB( submeshAABB, submeshTransform );

			// TODO: Need to work out why the fuck the AABB is fucked for Skeletal Meshes.
			if( /*m_RendererData.CurrentCamera.pCamera->CameraFrustumIntersectsAABB( transformedAABB )*/ true )
			{
				StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i };

				// Submit for rendering
				auto& command = m_DynamicDrawList[ key ];
				command.Mesh = mesh;
				command.SubmeshIndex = ( uint32_t ) i;
				++command.Instances;

				SendBoneDataToMap( mesh, key, boneTransforms );

				auto& shadow = m_DynamicShadowMapDrawList[ key ];
				shadow.Mesh = mesh;
				shadow.SubmeshIndex = ( uint32_t ) i;
				++shadow.Instances;

				auto& data = m_RendererData.MeshTransforms[ key ].Data.emplace_back();
				data.TransfromBufferR[ 0 ] = {
					submeshTransform[ 0 ][ 0 ], submeshTransform[ 1 ][ 0 ], submeshTransform[ 2 ][ 0 ], submeshTransform[ 3 ][ 0 ]
				};
				data.TransfromBufferR[ 1 ] = {
					submeshTransform[ 0 ][ 1 ], submeshTransform[ 1 ][ 1 ], submeshTransform[ 2 ][ 1 ], submeshTransform[ 3 ][ 1 ]
				};
				data.TransfromBufferR[ 2 ] = {
					submeshTransform[ 0 ][ 2 ], submeshTransform[ 1 ][ 2 ], submeshTransform[ 2 ][ 2 ], submeshTransform[ 3 ][ 2 ]
				};
				data.TransfromBufferR[ 3 ] = {
					submeshTransform[ 0 ][ 3 ], submeshTransform[ 1 ][ 3 ], submeshTransform[ 2 ][ 3 ], submeshTransform[ 3 ][ 3 ]
				};
			}
		}
	}

	void SceneRenderer::SubmitPhysicsCollider( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform )
	{
		SAT_PF_EVENT();

		const auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); ++i )
		{
			StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i };

			auto& command = m_PhysicsColliderDrawList[ key ];
			command.Mesh = mesh;
			command.SubmeshIndex = ( uint32_t ) i;
			++command.Instances;

			auto& data = m_RendererData.MeshTransforms[ key ].Data.emplace_back();
			data.TransfromBufferR[ 0 ] = {
				transform[ 0 ][ 0 ], transform[ 1 ][ 0 ], transform[ 2 ][ 0 ], transform[ 3 ][ 0 ]
			};
			data.TransfromBufferR[ 1 ] = {
				transform[ 0 ][ 1 ], transform[ 1 ][ 1 ], transform[ 2 ][ 1 ], transform[ 3 ][ 1 ]
			};
			data.TransfromBufferR[ 2 ] = {
				transform[ 0 ][ 2 ], transform[ 1 ][ 2 ], transform[ 2 ][ 2 ], transform[ 3 ][ 2 ]
			};
			data.TransfromBufferR[ 3 ] = {
				transform[ 0 ][ 3 ], transform[ 1 ][ 3 ], transform[ 2 ][ 3 ], transform[ 3 ][ 3 ]
			};
		}
	}

	void SceneRenderer::SubmitSelectedStaticMesh( SharedPtr<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform )
	{
		SAT_PF_EVENT();

		const auto& rSubmeshes = mesh->Submeshes();
		for( size_t i = 0; i < rSubmeshes.size(); ++i )
		{
			const glm::mat4 submeshTransform = transform * rSubmeshes[ i ].Transform;

			const StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i, true };

			auto& command = m_SelectedStaticMeshDrawList[ key ];
			command.Mesh = mesh;
			command.SubmeshIndex = ( uint32_t ) i;
			++command.Instances;

			auto& rData = m_RendererData.MeshTransforms[ key ].Data.emplace_back();
			rData.TransfromBufferR[ 0 ] = {
				submeshTransform[ 0 ][ 0 ], submeshTransform[ 1 ][ 0 ], submeshTransform[ 2 ][ 0 ], submeshTransform[ 3 ][ 0 ]
			};
			rData.TransfromBufferR[ 1 ] = {
				submeshTransform[ 0 ][ 1 ], submeshTransform[ 1 ][ 1 ], submeshTransform[ 2 ][ 1 ], submeshTransform[ 3 ][ 1 ]
			};
			rData.TransfromBufferR[ 2 ] = {
				submeshTransform[ 0 ][ 2 ], submeshTransform[ 1 ][ 2 ], submeshTransform[ 2 ][ 2 ], submeshTransform[ 3 ][ 2 ]
			};
			rData.TransfromBufferR[ 3 ] = {
				submeshTransform[ 0 ][ 3 ], submeshTransform[ 1 ][ 3 ], submeshTransform[ 2 ][ 3 ], submeshTransform[ 3 ][ 3 ]
			};
		}
	}

	void SceneRenderer::SubmitSelectedDynamicMesh( SharedPtr<Entity> entity, Ref< SkeletalMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform )
	{
		SAT_PF_EVENT();

		const auto& rSubmeshes = mesh->Submeshes();
		for( size_t i = 0; i < rSubmeshes.size(); ++i )
		{
			const glm::mat4 submeshTransform = transform * rSubmeshes[ i ].Transform;

			const StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i, true };

			auto& command = m_DynamicSelectedMeshDrawList[ key ];
			command.Mesh = mesh;
			command.SubmeshIndex = ( uint32_t ) i;
			++command.Instances;

			auto& rData = m_RendererData.MeshTransforms[ key ].Data.emplace_back();
			rData.TransfromBufferR[ 0 ] = {
				submeshTransform[ 0 ][ 0 ], submeshTransform[ 1 ][ 0 ], submeshTransform[ 2 ][ 0 ], submeshTransform[ 3 ][ 0 ]
			};
			rData.TransfromBufferR[ 1 ] = {
				submeshTransform[ 0 ][ 1 ], submeshTransform[ 1 ][ 1 ], submeshTransform[ 2 ][ 1 ], submeshTransform[ 3 ][ 1 ]
			};
			rData.TransfromBufferR[ 2 ] = {
				submeshTransform[ 0 ][ 2 ], submeshTransform[ 1 ][ 2 ], submeshTransform[ 2 ][ 2 ], submeshTransform[ 3 ][ 2 ]
			};
			rData.TransfromBufferR[ 3 ] = {
				submeshTransform[ 0 ][ 3 ], submeshTransform[ 1 ][ 3 ], submeshTransform[ 2 ][ 3 ], submeshTransform[ 3 ][ 3 ]
			};
		}
	}

	void SceneRenderer::SetViewportSize( uint32_t w, uint32_t h )
	{
		if( m_RendererData.Width != w || m_RendererData.Height != h )
		{
			m_RendererData.Width = w;
			m_RendererData.Height = h;
			m_RendererData.Resized = true;
		}

		if( m_Renderer2D )
			m_Renderer2D->SetViewportSize( w, h );

		if( m_AluraRenderer )
			m_AluraRenderer->SetViewportSize( w, h );
	}

	void SceneRenderer::Recreate()
	{
		InitPreDepth();
		InitGeometryPass();

		InitAO( m_AOTechnique, true );

		CreateBloomMaterials();

		constexpr uint32_t kForwardPlusTileSize = 16;
		glm::uvec2 Viewport = { m_RendererData.Width, m_RendererData.Height };
		glm::uvec2 Size = Viewport;
		Size += kForwardPlusTileSize - Viewport % kForwardPlusTileSize;

		m_RendererData.LightCullingWorkGroups = { Size / kForwardPlusTileSize, 1 };
		const float size = m_RendererData.LightCullingWorkGroups.x * m_RendererData.LightCullingWorkGroups.y * 4.0f * 1024.0f;
		m_RendererData.StorageBufferSet->Resize( 0, 14, ( size_t ) size );

		// Cloud buffers depend on the viewport size, recreate before SceneComposite binds them.
		InitVolumetricCloudsRaymarch();

		InitSceneComposite();
		InitLateComposite();
		
		InitSMAAPass();

		InitSelectionPass();
		InitJumpFlood();
		InitTexturePass();

		CreateSkyboxComponents();
		CreateGridComponents();

		m_RendererData.TransmittanceLutMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.CloudsRaymarchMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.SkyViewLutMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.MultiScatteringLutMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );

		if( /*HasFlag( SceneRendererFlag_MasterInstance )*/ !HasFlag( SceneRendererFlag_NoRenderer2D ) )
		{
			m_Renderer2D->SetInitialRenderPass( m_RendererData.LateCompositePass, m_RendererData.LateCompositeFramebuffer );
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
	// RENDERER SHADER 
	//////////////////////////////////////////////////////////////////////////

	void SceneRenderer::GeometryPass()
	{
		SAT_PF_EVENT();

		m_RendererData.GeometryPassTimer.Reset();

		VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };

		// Begin geometry pass.
		m_RendererData.GeometryPass->BeginPass( m_RendererData.CommandBuffer, m_RendererData.GeometryFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetScissor( m_RendererData.CommandBuffer, 0, 1, &Scissor );
		vkCmdSetViewport( m_RendererData.CommandBuffer, 0, 1, &Viewport );

		//////////////////////////////////////////////////////////////////////////
		// Actual geometry pass.
		//////////////////////////////////////////////////////////////////////////

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Skybox" );

		RenderSkybox();

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

#if !defined(SAT_DIST)
		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Grid" );

		RenderGrid();

		CmdEndDebugLabel( m_RendererData.CommandBuffer );
#endif

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Static meshes" );
		
		// Set environment resource.
		Renderer::Get()->SetSceneEnvironment( m_RendererData.ShadowCascades[ 0 ].Framebuffer->GetDepthAttachmentResource(), m_RendererData.SceneEnvironment, m_RendererData.BRDFLUT_Texture, m_RendererData.CloudShadowMap );

		RenderStaticMeshes();

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Dynamic meshes" );
		
		RenderDynamicMeshes();

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		//////////////////////////////////////////////////////////////////////////

		// End geometry pass.
		m_RendererData.GeometryPass->EndPass();

		m_RendererData.GeometryPassTimer.Stop();
	}

	void SceneRenderer::RenderStaticMeshes()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		// u_Matrices
		UBStaticMeshMatrices u_Matrices = {};
		u_Matrices.View = m_RendererData.CurrentCamera.ViewMatrix;
		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		UBLightData u_LightData = {};
//		std::unique_ptr<UBPointLights> u_Lights = std::make_unique<UBPointLights>();

		UBPointLights u_Lights = {};

		u_Lights.nbLights = ( uint32_t ) m_pScene->m_Lights.PointLights.size();
		std::memcpy( u_Lights.Lights, m_pScene->m_Lights.PointLights.data(), m_pScene->m_Lights.GetPointLightSize() );

		UBSceneData u_SceneData = {};
		UBShadowData u_ShadowData = {};

		struct UBDebugData
		{
			int TilesCountX;
		} u_DebugData = {};

		u_DebugData.TilesCountX = ( int ) m_RendererData.LightCullingWorkGroups.x;

		const auto& rDirLight = m_pScene->m_Lights.DirectionalLights[ 0 ];
		const auto invView = glm::inverse( u_Matrices.View );

		u_SceneData.CameraPosition = invView[ 3 ];
		u_SceneData.Lights = { .Direction = rDirLight.Direction, .Radiance = rDirLight.Radiance, .Multiplier = rDirLight.Intensity };

		if( m_RendererData.EnableShadows )
		{
			for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
			{
				u_ShadowData.CascadeSplits[ i ] = m_RendererData.ShadowCascades[ i ].SplitDepth;
				u_LightData.LightMatrix[ i ] = m_RendererData.ShadowCascades[ i ].ViewProjection;
			}
		}

		m_RendererData.UniformBufferSet->Get( 0, 0, frame )->UploadData( &u_Matrices, sizeof( u_Matrices ) );

//		m_RendererData.UniformBufferSet->Get( 0, 1, frame )->UploadData( &u_LightData, sizeof( u_LightData ) );

		m_RendererData.UniformBufferSet->Get( 0, 2, frame )->UploadData( &u_SceneData, sizeof( u_SceneData ) );
		m_RendererData.UniformBufferSet->Get( 0, 3, frame )->UploadData( &u_ShadowData, sizeof( u_ShadowData ) );
		m_RendererData.UniformBufferSet->Get( 0, 12, frame )->UploadData( &u_DebugData, sizeof( u_DebugData ) );
		
		// Alignment is 16 bytes
		m_RendererData.UniformBufferSet->Get( 0, 13, frame )->UploadData( &u_Lights, 16ull + sizeof( u_Lights ) * u_Lights.nbLights );

		for( auto&& [key, Cmd] : m_DrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			// Render Submesh
			Renderer::Get()->SubmitMesh(
				m_RendererData.CommandBuffer,
				m_RendererData.StaticMeshPipeline,
				Cmd.Mesh,
				m_RendererData.StorageBufferSet,
				m_RendererData.UniformBufferSet,
				key.Registry,
				Cmd.SubmeshIndex,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset,
				m_GeometryPipelineCache );
		}
	}

	void SceneRenderer::RenderDynamicMeshes()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		m_RendererData.DynamicMeshMaterial->Update( {} );

		uint32_t index = 0;
		for( auto&& [key, Cmd] : m_DynamicDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			// Render Submesh
			Renderer::Get()->SubmitDynamicMesh(
				m_RendererData.CommandBuffer,
				m_RendererData.DynamicMeshPipeline,
				Cmd.Mesh,
				m_RendererData.StorageBufferSet,
				m_RendererData.UniformBufferSet,
				key.Registry,
				Cmd.SubmeshIndex,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset, index, m_RendererData.DynamicMeshMaterial );

			index += Cmd.Instances;
		}
	}

	void SceneRenderer::DirShadowMapPass()
	{
		SAT_PF_EVENT();

		if( !m_RendererData.EnableShadows )
			return;

		const auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
		const VkExtent2D Extent = { ( uint32_t ) kShadowMapSize, ( uint32_t ) kShadowMapSize };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		std::array<VkClearValue, 2> ClearColors{};
		ClearColors[ 0 ].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		RenderPassBeginInfo.renderArea.extent = Extent;
		RenderPassBeginInfo.pClearValues = ClearColors.data();
		RenderPassBeginInfo.clearValueCount = ( uint32_t ) ClearColors.size();

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = kShadowMapSize;
		Viewport.height = kShadowMapSize;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		//////////////////////////////////////////////////////////////////////////

		UpdateCascades( m_pScene->m_Lights.DirectionalLights[ 0 ].Direction );

		// u_LightData
		UBLightData u_LightData{};

		for( size_t i = 0; i < SHADOW_CASCADE_COUNT; ++i )
		{
			u_LightData.LightMatrix[ i ] = m_RendererData.ShadowCascades[ i ].ViewProjection;
		}

		m_RendererData.UniformBufferSet->Get( 0, 1, frame )->UploadData( &u_LightData, sizeof( u_LightData ) );

		m_RendererData.DirShadowMapDynamicMaterialSet2->Update( {} );

		for( auto i = 0; i < SHADOW_CASCADE_COUNT; ++i )
		{
			m_RendererData.ShadowMapTimers[ i ].Reset();

			RenderPassBeginInfo.framebuffer = m_RendererData.ShadowCascades[ i ].Framebuffer->GetVulkanFramebuffer();
			RenderPassBeginInfo.renderPass = m_RendererData.DirShadowMapPasses[ i ]->GetVulkanPass();

			// Begin directional shadow map pass.
			CmdBeginDebugLabel( CommandBuffer, "ShadowMap" );
			vkCmdBeginRenderPass( CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

			vkCmdSetViewport( m_RendererData.CommandBuffer, 0, 1, &Viewport );
			vkCmdSetScissor( m_RendererData.CommandBuffer, 0, 1, &Scissor );

			CmdBeginDebugLabel( CommandBuffer, "ShadowMap-Static" );
			for( auto&& [key, Cmd] : m_ShadowMapDrawList )
			{
				// Pass in the cascade index.
				Buffer AdditionalData( sizeof( uint32_t ), &i );

				const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

				Renderer::Get()->RenderMeshWithoutMaterial( 
					CommandBuffer, 
					m_RendererData.DirShadowMapPipelines[ i ],
					Cmd.Mesh, 
					m_RendererData.DirShadowMapMaterial,
					m_RendererData.UniformBufferSet,
					nullptr,
					Cmd.Instances, 
					m_RendererData.SubmeshTransformData[ frame ].VertexBuffer, 
					rTransformData.Offset, 
					Cmd.SubmeshIndex, 
					AdditionalData );
			}
			CmdEndDebugLabel( CommandBuffer );

			CmdBeginDebugLabel( CommandBuffer, "ShadowMap-Dynamic" );
			uint32_t index = 0;
			for( auto&& [key, Cmd] : m_DynamicShadowMapDrawList )
			{
				// Pass in the cascade index.
				struct PC
				{
					uint32_t CascadeIndex = 0u;
				} u_Data;
				u_Data.CascadeIndex = i;

				Buffer AdditionalData( sizeof( PC ), &u_Data );

				const auto& rTransformData = m_RendererData.MeshTransforms[ key ];
				Renderer::Get()->RenderDynamicMeshWithoutMaterial(
					CommandBuffer,
					m_RendererData.DirShadowMapDynamicPipelines[ i ],
					Cmd.Mesh,
					m_RendererData.DirShadowMapMaterial,
					m_RendererData.UniformBufferSet,
					m_RendererData.StorageBufferSet,
					Cmd.Instances,
					m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
					rTransformData.Offset,
					Cmd.SubmeshIndex, index, m_RendererData.DirShadowMapDynamicMaterialSet2, AdditionalData );

				index += Cmd.Instances;
			}
			CmdEndDebugLabel( CommandBuffer );

			vkCmdEndRenderPass( CommandBuffer );
			CmdEndDebugLabel( CommandBuffer );

			m_RendererData.ShadowMapTimers[ i ].Stop();
		}
	}

	void SceneRenderer::PreDepthPass()
	{
		SAT_PF_EVENT();

		m_RendererData.PreDepthTimer.Reset();

		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		const VkExtent2D Extent = { m_RendererData.Width,m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.PreDepthPass->BeginPass( CommandBuffer, m_RendererData.PreDepthFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		// u_Matrices
		struct UBMatricesPreDepth
		{
			glm::mat4 ViewProjection;
		} u_Matrices{};

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		// We cannot write to the UniformBufferSet as we don't use the same UB as the other shaders
		m_RendererData.PreDepthMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		m_RendererData.PreDepthDynamicMaterialSet2->Update( {} );

		CmdBeginDebugLabel( CommandBuffer, "PreDepth-Static" );

		for( auto&& [key, Cmd] : m_DrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];
			Renderer::Get()->RenderMeshWithoutMaterial(
				CommandBuffer,
				m_RendererData.PreDepthPipeline,
				Cmd.Mesh,
				m_RendererData.PreDepthMaterial,
				m_RendererData.UniformBufferSet,
				m_RendererData.StorageBufferSet,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset,
				Cmd.SubmeshIndex );
		}

		CmdEndDebugLabel( CommandBuffer );

		CmdBeginDebugLabel( CommandBuffer, "PreDepth-Dynamic" );

		uint32_t index = 0;
		for( auto&& [key, Cmd] : m_DynamicDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];
			Renderer::Get()->RenderDynamicMeshWithoutMaterial(
				CommandBuffer,
				m_RendererData.PreDepthDynamicPipeline,
				Cmd.Mesh,
				m_RendererData.PreDepthDynamicMaterial,
				m_RendererData.UniformBufferSet,
				m_RendererData.StorageBufferSet,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset,
				Cmd.SubmeshIndex, index, m_RendererData.PreDepthDynamicMaterialSet2 );

			index += Cmd.Instances;
		}

		CmdEndDebugLabel( CommandBuffer );

		m_RendererData.PreDepthPass->EndPass();
		m_RendererData.PreDepthTimer.Stop();
	}

	void SceneRenderer::SceneCompositePass()
	{
		SAT_PF_EVENT();

		m_RendererData.SceneCompPPTimer.Reset();

		const VkExtent2D Extent = { m_RendererData.Width,m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// Begin scene composite pass.
		m_RendererData.SceneComposite->BeginPass( CommandBuffer, m_RendererData.SceneCompositeFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		struct UParams
		{
			uint32_t Flags = 0u;
			uint32_t ColorBlindMode = 0u;
		} pc_Params{};

		pc_Params.Flags = m_RendererData.SceneCompositeFlags;
		pc_Params.ColorBlindMode = ( uint32_t ) m_ColourBlindMode;

		Buffer pc( sizeof( UParams ), &pc_Params );

		vkCmdPushConstants( 
			CommandBuffer, 
			m_RendererData.SceneCompositePipeline->GetPipelineLayout(),
			VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			( uint32_t ) pc.Size, pc.Data );

		// Actual scene composite pass.
		Renderer::Get()->SubmitFullscreenQuad2(
			CommandBuffer, 
			m_RendererData.SceneCompositePipeline,
			m_RendererData.SceneCompositeMaterial );

		// End scene composite pass.
		m_RendererData.SceneComposite->EndPass();

		m_RendererData.SceneCompPPTimer.Stop();
	}

	void SceneRenderer::LateCompPhysicsOutline()
	{
		if( !m_PhysicsColliderDrawList.size() )
			return;

		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		const VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.LateCompositePass->BeginPass( CommandBuffer, m_RendererData.LateCompositeFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		struct UB_Matrices
		{
			glm::mat4 ViewProjection;
		} u_Matrices{};

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		m_RendererData.PhysicsOutlineMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		for( auto& [key, Cmd] : m_PhysicsColliderDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			Renderer::Get()->RenderMeshWithoutMaterial(
				CommandBuffer,
				m_RendererData.PhysicsOutlinePipeline,
				Cmd.Mesh,
				m_RendererData.PhysicsOutlineMaterial,
				m_RendererData.UniformBufferSet,
				m_RendererData.StorageBufferSet,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset,
				Cmd.SubmeshIndex );
		}

		m_RendererData.LateCompositePass->EndPass();
	}

	void SceneRenderer::JumpFloodLatePass()
	{
		SAT_PF_EVENT();

		const VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		// Begin odd pass
		m_RendererData.JumpFloodOddPass->BeginPass( CommandBuffer, m_RendererData.JumpFloodOddFB->GetVulkanFramebuffer(), Extent );

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		Renderer::Get()->SubmitFullscreenQuad(
			CommandBuffer,
			m_RendererData.JumpFloodOddPipeline,
			m_RendererData.JumpFloodOddMaterial,
			m_RendererData.QuadIndexBuffer,
			m_RendererData.QuadVertexBuffer
		);

		m_RendererData.JumpFloodOddPass->EndPass();
	}

	void SceneRenderer::SMAACompositionPass()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		const VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.SMAACompPass->BeginPass( CommandBuffer, m_RendererData.SMAACompFB->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		Renderer::Get()->SubmitFullscreenQuad(
			CommandBuffer,
			m_RendererData.SMAACompPipeline,
			m_RendererData.SMAACompMaterial, 
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		m_RendererData.SMAACompPass->EndPass();
	}

	void SceneRenderer::SMAAEdgePass()
	{
		struct u_Params
		{
			glm::vec4 Metrics{};
			float Threshold = 0.0f;
			float LocalContrastAdaptation = 0.0f;
		} pc_Params;

		pc_Params.Metrics = glm::vec4(
			1.0f / static_cast< float >( m_RendererData.Width ),
			1.0f / static_cast< float >( m_RendererData.Height ),
			m_RendererData.Width, m_RendererData.Height );

		pc_Params.Threshold = 0.1f;
		pc_Params.LocalContrastAdaptation = 2.0f;

		m_RendererData.SMAAEdgeDetectionPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		glm::uvec3 workGroups{ 1 };
		workGroups.x = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Width ) / 8.0f );
		workGroups.y = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Height ) / 8.0f );

		Buffer pc( sizeof( u_Params ), &pc_Params );

		m_RendererData.SMAAEdgeDetectionPipeline->ExecuteWithExternalPC( m_RendererData.SMAAEdgingMaterial, pc,
			workGroups.x,
			workGroups.y,
			workGroups.z );
	}

	void SceneRenderer::SMAABlendingPass()
	{
		struct u_Params
		{
			glm::vec4 Metrics{};
			float Threshold = 0.0f;
			float CornerRoundingNorm = 0.0f;
			float LocalContrastAdaptation = 0.0f;
		} pc_Params;

		pc_Params.Metrics = glm::vec4(
			1.0f / static_cast< float >( m_RendererData.Width ),
			1.0f / static_cast< float >( m_RendererData.Height ),
			m_RendererData.Width, m_RendererData.Height );

		pc_Params.Threshold = 0.1f;
		pc_Params.CornerRoundingNorm = 0.25f;
		pc_Params.LocalContrastAdaptation = 2.0f;

		m_RendererData.SMAAFinalPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		glm::uvec3 workGroups{ 1 };
		workGroups.x = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Width ) / 32.0f );
		workGroups.y = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Height ) / 32.0f );

		Buffer pc( sizeof( u_Params ), &pc_Params );

		m_RendererData.SMAAFinalPipeline->ExecuteWithExternalPC( m_RendererData.SMAAFinalMaterial, pc,
			workGroups.x,
			workGroups.y,
			workGroups.z );
	}

	void SceneRenderer::SMAAPass()
	{
		SAT_PF_EVENT();
		
		m_RendererData.SMAAPassTimer.Reset();

		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		CmdBeginDebugLabel( CommandBuffer, "EdgeDetection" );
		SMAAEdgePass();
		CmdEndDebugLabel( CommandBuffer );

		CmdBeginDebugLabel( CommandBuffer, "Weights,Blending" );
		SMAABlendingPass();
		CmdEndDebugLabel( CommandBuffer );

		VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_RendererData.SMAAFinalOutImage->GetImage();
		barrier.subresourceRange = 
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};

		VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		CmdBeginDebugLabel( CommandBuffer, "Sync" );
		vkCmdPipelineBarrier2( CommandBuffer, &dependencyInfo );
		CmdEndDebugLabel( CommandBuffer );

		CmdBeginDebugLabel( CommandBuffer, "CompositionPass" );
		SMAACompositionPass();
		CmdEndDebugLabel( CommandBuffer );

		m_RendererData.SMAAPassTimer.Stop();
	}

	void SceneRenderer::TexturePass()
	{
		SAT_PF_EVENT();

		Ref<Pass> pass = VulkanContext::Get()->GetDefaultPass();
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		// On Retina displays the swapchain is larger than the scene, 
		// which is sized in window points.
		const VkExtent2D Extent = VulkanContext::Get()->GetSwapchain().GetExtent();
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// Begin scene composite pass.
		pass->BeginPass( CommandBuffer, VulkanContext::Get()->GetSwapchain().GetFramebuffers()[ frame ], Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = ( float ) Extent.height;
		Viewport.width = ( float ) Extent.width;
		Viewport.height = -( float ) Extent.height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		// Actual scene composite pass.
		Renderer::Get()->SubmitFullscreenQuad(
			CommandBuffer, m_RendererData.TexturePassPipeline,
			m_RendererData.TexturePassMaterial,
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		// End scene composite pass.
		pass->EndPass();
	}

	void SceneRenderer::LightCullingPass()
	{
		SAT_PF_EVENT();

		m_RendererData.LightCullingTimer.Reset();

		if( m_RendererData.LightCullingWorkGroups.x == 0 ) 
		{
			constexpr uint32_t kTileSize = 16;
			glm::uvec2 Viewport = { m_RendererData.Width, m_RendererData.Height };
			glm::uvec2 Size = Viewport;
			Size += kTileSize - Viewport % kTileSize;

			m_RendererData.LightCullingWorkGroups = { Size / kTileSize, 1 };

			const float size = m_RendererData.LightCullingWorkGroups.x * m_RendererData.LightCullingWorkGroups.y * 4.0f * 1024.0f;
			m_RendererData.StorageBufferSet->Resize( 0, 14, ( size_t ) size );
		}

		// UBs
		UBPointLights u_Lights;

		struct
		{
			glm::vec2 FullResolution;
		} u_ScreenData{};

		struct
		{
			glm::mat4 ViewProjection;
			glm::mat4 Projection;
			glm::mat4 View;
			glm::mat4 InvP;
		} u_Matrices{};

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;
		u_Matrices.Projection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix();
		u_Matrices.View = glm::inverse( m_RendererData.CurrentCamera.ViewMatrix );
		u_Matrices.InvP = glm::inverse( u_Matrices.Projection );

		u_ScreenData.FullResolution = { m_RendererData.Width, m_RendererData.Height };

		u_Lights.nbLights = ( uint32_t ) m_pScene->m_Lights.PointLights.size();

		std::memcpy( u_Lights.Lights, m_pScene->m_Lights.PointLights.data(), m_pScene->m_Lights.GetPointLightSize() );

		m_RendererData.LightCullingMaterial->UploadDataToUB( 0, &u_Lights, 16ull + sizeof( PointLight ) * u_Lights.nbLights );
		m_RendererData.LightCullingMaterial->UploadDataToUB( 3, &u_ScreenData, sizeof( u_ScreenData ) );
		m_RendererData.LightCullingMaterial->UploadDataToUB( 4, &u_Matrices, sizeof( u_Matrices ) );

		// Write storage buffer
		Ref<StorageBuffer> SB = m_RendererData.StorageBufferSet->Get( 0, 14, Renderer::Get()->GetCurrentFrame() );

		// Light culling here
		auto& CullingPipeline = m_RendererData.LightCullingPipeline;

		CullingPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );
		m_RendererData.LightCullingMaterial->SetSB( 14, SB );

		CullingPipeline->Execute(
			m_RendererData.LightCullingMaterial,
			( uint32_t ) m_RendererData.LightCullingWorkGroups.x,
			( uint32_t ) m_RendererData.LightCullingWorkGroups.y,
			( uint32_t ) m_RendererData.LightCullingWorkGroups.z );

		VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier( CullingPipeline->GetCommandBuffer(),
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			1, &barrier,
			0, nullptr,
			0, nullptr );

		CullingPipeline->Unbind();

		m_RendererData.LightCullingTimer.Stop();
	}

	void SceneRenderer::AtmospherePass()
	{
		SAT_PF_EVENT();

		m_RendererData.AtmosphereTimer.Reset();

		struct USkyData
		{
			glm::vec3 SunDir;
		} u_SkyData{};
		
		struct UCameraData
		{
			glm::mat4 InverseCameraViewProj;
		} u_CameraData;

		u_CameraData.InverseCameraViewProj = glm::inverse( m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix );

		struct UAtmosParam
		{
			AtmosphereParameters Params;
		} u_AtmosphereParameters{};

		u_AtmosphereParameters.Params = CreateDefaultAtmosphereParameters();

		const auto& rDirLight = m_pScene->m_Lights.DirectionalLights[ 0 ];
		u_SkyData.SunDir = rDirLight.Direction;

		const glm::vec3 groundAlbedo = glm::clamp( rDirLight.Sky.GroundAlbedo, glm::vec3( 0.0f ), glm::vec3( 1.0f ) );

		// Only rebuild the transmittance and multiple scattering LUTs if their inputs changed.
		const bool needsToRebuildTransmittance = !m_RendererData.AtmosphereTransmittanceValid;
		const bool needsToRebuildMultiScattering = needsToRebuildTransmittance
			|| !m_RendererData.AtmosphereMultiScatteringValid
			|| groundAlbedo != m_RendererData.AtmosphereLutGroundAlbedo;

		if( needsToRebuildTransmittance )
		{
			m_RendererData.TransmittanceLutMaterial->UploadDataToUB( 6, &u_AtmosphereParameters, sizeof( UAtmosParam ) );
			m_RendererData.TransmittanceLutMaterial->UploadDataToUB( 7, &u_CameraData, sizeof( UCameraData ) );

			m_RendererData.TransmittanceLutMaterial->SetPC( "u_SkyData.SunDir", u_SkyData.SunDir );

			m_RendererData.TransmittanceLutPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

			m_RendererData.TransmittanceLutPipeline->Execute(
				m_RendererData.TransmittanceLutMaterial,
				( uint32_t ) glm::ceil( ( float ) m_RendererData.TransmittanceLutOut->GetWidth() / 8.F ),
				( uint32_t ) glm::ceil( ( float ) m_RendererData.TransmittanceLutOut->GetHeight() / 8.0F ),
				1u );

			m_RendererData.TransmittanceLutPipeline->Unbind();

			m_RendererData.AtmosphereTransmittanceValid = true;
		}

		// Sky-view LUT.
		{
			glm::vec3 sunDirection, sunColor;
			GetSceneSun( rDirLight, sunDirection, sunColor );

			const glm::vec3 cameraPosMeters = glm::vec3( glm::inverse( m_RendererData.CurrentCamera.ViewMatrix )[ 3 ] );

			struct SkyViewLutData
			{
				glm::vec4 CameraPosKm;
				glm::vec4 SunDirection;
			} skyViewData{};

			skyViewData.CameraPosKm = glm::vec4( cameraPosMeters * 0.001f + glm::vec3( 0.0f, u_AtmosphereParameters.Params.BottomRadius, 0.0f ), 1.0f );
			skyViewData.SunDirection = glm::vec4( sunDirection, 0.0f );

			// The LUT's ground (direct sun) uses the same albedo as the sky draw's sky light term.
			u_AtmosphereParameters.Params.GroundAlbedo = groundAlbedo;

			m_RendererData.SkyViewLutMaterial->UploadDataToUB( 6, &u_AtmosphereParameters, sizeof( UAtmosParam ) );
			m_RendererData.SkyViewLutMaterial->UploadDataToUB( 7, &u_CameraData, sizeof( UCameraData ) );
			m_RendererData.SkyViewLutMaterial->UploadDataToUB( 8, &skyViewData, sizeof( SkyViewLutData ) );

			const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

			auto barrier = [ & ]( VkImage image,
				VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
				VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess )
			{
				VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
				barrier.srcStageMask = srcStage;
				barrier.srcAccessMask = srcAccess;
				barrier.dstStageMask = dstStage;
				barrier.dstAccessMask = dstAccess;
				barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;
				barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				return barrier;
			};

			auto submit = [ & ]( std::initializer_list<VkImageMemoryBarrier2> barriers )
			{
				VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
				dependencyInfo.imageMemoryBarrierCount = ( uint32_t ) barriers.size();
				dependencyInfo.pImageMemoryBarriers = barriers.begin();
				vkCmdPipelineBarrier2( CommandBuffer, &dependencyInfo );
			};

			const VkImage transmittanceImage = m_RendererData.TransmittanceLutOut->GetImage();
			const VkImage multiScatteringImage = m_RendererData.MultiScatteringLutOut->GetImage();
			const VkImage skyViewImage = m_RendererData.SkyViewLutOut->GetImage();

			submit( {
				// The multiple scattering and sky-view LUTs sample the transmittance LUT written just above.
				barrier( transmittanceImage,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),

				// Last frame's sky-view LUT and clouds read the multiple scattering LUT.
				barrier( multiScatteringImage,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ),

				// Last frame's sky draw and clouds read the sky-view LUT.
				barrier( skyViewImage,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ),
			} );

			// Multiple scattering LUT. Depends only on the atmosphere and ground albedo (not the
			// sun direction or camera).
			if( needsToRebuildMultiScattering )
			{
				m_RendererData.MultiScatteringLutMaterial->UploadDataToUB( 6, &u_AtmosphereParameters, sizeof( UAtmosParam ) );
				m_RendererData.MultiScatteringLutMaterial->UploadDataToUB( 7, &u_CameraData, sizeof( UCameraData ) );

				m_RendererData.MultiScatteringLutPipeline->BindWithCommandBuffer( CommandBuffer );

				m_RendererData.MultiScatteringLutPipeline->Execute(
					m_RendererData.MultiScatteringLutMaterial,
					( uint32_t ) glm::ceil( ( float ) m_RendererData.MultiScatteringLutOut->GetWidth() / 8.F ),
					( uint32_t ) glm::ceil( ( float ) m_RendererData.MultiScatteringLutOut->GetHeight() / 8.0F ),
					1u );

				m_RendererData.MultiScatteringLutPipeline->Unbind();

				// Read by the sky-view LUT below and the clouds' aerial perspective.
				submit( {
					barrier( multiScatteringImage,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),
				} );

				m_RendererData.AtmosphereMultiScatteringValid = true;
				m_RendererData.AtmosphereLutGroundAlbedo = groundAlbedo;
			}

			m_RendererData.SkyViewLutPipeline->BindWithCommandBuffer( CommandBuffer );

			m_RendererData.SkyViewLutPipeline->Execute(
				m_RendererData.SkyViewLutMaterial,
				( uint32_t ) glm::ceil( ( float ) m_RendererData.SkyViewLutOut->GetWidth() / 8.F ),
				( uint32_t ) glm::ceil( ( float ) m_RendererData.SkyViewLutOut->GetHeight() / 8.0F ),
				1u );

			m_RendererData.SkyViewLutPipeline->Unbind();

			// Read by the clouds (compute) and the sky draw (fragment).
			submit( {
				barrier( skyViewImage,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),
			} );
		}

		m_RendererData.AtmosphereTimer.Stop();
	}

	struct alignas( 16 ) CloudsData
	{
		glm::vec3 CameraPosKm;                  // Planet centred, km
		float     Time;

		glm::vec3 SunDirection;                 // Towards the sun, normalised
		float     CloudSpeed;

		glm::vec3 SunColor;                     // Colour * intensity
		float     CloudCoverage;

		glm::vec3 CloudDirection;               // Wind direction
		float     CloudDensity;

		glm::vec3 CloudAlbedo;
		float     CloudBasicNoiseScale;

		glm::vec3 SkyAmbientColor;
		float     CloudDetailNoiseScale;

		glm::vec3 GroundBounceColor;
		float     CloudWeatherUVScale;

		float     CloudMaxTracingDistance;      // km
		float     CloudTracingStartMaxDistance; // km
		float     CloudLightBasicStep;          // km
		float     CloudPhaseForward;
		float     CloudPhaseBackward;
		float     CloudPhaseMixFactor;
		float     CloudMultiScatterExtinction;
		float     CloudMultiScatterScatter;
		float     CloudShadingSunLightScale;
		float     CloudAmbientScale;
		float     CloudPowderPow;
		float     CloudPowderScale;

		uint32_t  CloudMarchingStepNum;
		uint32_t  CloudLightStepNum;
		uint32_t  CloudEnableGroundContribution;
		uint32_t  FrameIndex;

		float     CloudBottomKm;
		float     CloudTopKm;
		float     CloudType;
		float     CloudErosion;

		float     CloudCurlStrength;            // km
		int32_t   TraceOffsetX;                 // Pixel of each 2x2 block traced this frame
		int32_t   TraceOffsetY;
		float     _Pad0;

		glm::vec2 ReconstructSize;
		glm::vec2 _Pad1;

		glm::vec3 SkyLightColor;                // Sun illuminance used by the physical sky
		int32_t   UseSkyAmbient;                // Ambient from the sky-view LUT instead of SkyAmbientColor
	};

	static_assert( sizeof( CloudsData ) == 240, "CloudsData must match the layout in CloudsDensity.glsl" );

	// Matching with CloudsReconstruct.glsl.
	struct CloudsReconstructData
	{
		glm::mat4 InverseViewProj;
		glm::mat4 PrevViewProj;
		glm::vec4 CameraPosMeters;

		int32_t   TraceOffsetX;
		int32_t   TraceOffsetY;
		int32_t   HistoryValid;
		float     BlendTraced;
		float     BlendOther;
		float     _Pad0;
		float     _Pad1;
		float     _Pad2;
	};

	static_assert( sizeof( CloudsReconstructData ) == 176, "CloudsReconstructData must match the layout in CloudsReconstruct.glsl" );

	inline CloudsData MakeCloudsData(
		const VolumetricCloudSettings& rSettings,
		float timeSeconds, uint32_t frameIndex,
		const glm::vec3& rCameraPosGroundKm, float planetRadiusKm,
		const glm::vec3& rSunDirection, const glm::vec3& rSunColor )
	{
		CloudsData c{};

		c.CameraPosKm = rCameraPosGroundKm + glm::vec3( 0.0f, planetRadiusKm, 0.0f );
		c.Time = timeSeconds;
		c.FrameIndex = frameIndex; 

		c.SunDirection = rSunDirection;
		c.SunColor = rSunColor * rSettings.SunIlluminance;

		// Wind
		c.CloudDirection = glm::length( rSettings.WindDirection ) > 1e-4f ? glm::normalize( rSettings.WindDirection ) : glm::vec3( 1.0f, 0.0f, 0.0f );
		c.CloudSpeed = rSettings.WindSpeed;

		// Layer (kms above the ground).
		c.CloudBottomKm = glm::max( rSettings.BottomKm, 0.0f );
		c.CloudTopKm = glm::max( rSettings.TopKm, c.CloudBottomKm + 0.1f );

		// Shape
		c.CloudCoverage = rSettings.Coverage;
		c.CloudType = rSettings.CloudType;
		c.CloudDensity = rSettings.Density;
		c.CloudErosion = rSettings.Erosion;
		c.CloudAlbedo = { 1.0f, 1.0f, 1.0f };
		c.CloudBasicNoiseScale = rSettings.BasicNoiseScale;
		c.CloudDetailNoiseScale = rSettings.DetailNoiseScale;
		c.CloudWeatherUVScale = rSettings.WeatherScale;
		c.CloudCurlStrength = rSettings.CurlStrength;

		// Ambient
		c.SkyAmbientColor = rSettings.SkyAmbientColor;
		c.GroundBounceColor = rSettings.GroundBounceColor;
		c.CloudAmbientScale = rSettings.AmbientScale;
		c.CloudEnableGroundContribution = rSettings.GroundContribution ? 1 : 0;

		// Marching
		c.CloudMaxTracingDistance = glm::max( rSettings.MaxTracingDistanceKm, 1.0f );
		c.CloudTracingStartMaxDistance = glm::max( rSettings.TracingStartMaxDistanceKm, 1.0f );
		c.CloudMarchingStepNum = glm::max( rSettings.MarchSteps, 1u );
		c.CloudLightStepNum = glm::max( rSettings.LightSteps, 1u );
		c.CloudLightBasicStep = glm::max( rSettings.LightMarchLengthKm, 0.01f );

		// Lighting
		c.CloudPhaseForward = rSettings.PhaseForward;
		c.CloudPhaseBackward = rSettings.PhaseBackward;
		c.CloudPhaseMixFactor = rSettings.PhaseMix;
		c.CloudMultiScatterExtinction = rSettings.MultiScatterExtinction;
		c.CloudMultiScatterScatter = rSettings.MultiScatterScatter;
		c.CloudShadingSunLightScale = 1.0f;
		c.CloudPowderPow = 1.0f;
		c.CloudPowderScale = rSettings.PowderScale;

		return c;
	}

	void SceneRenderer::VolumetricCloudsPass()
	{
		SAT_PF_EVENT();

		m_RendererData.AtmosphereVCTimer.Reset();

		struct UCameraData
		{
			glm::mat4 InverseCameraViewProj;
		} u_CameraData;

		const glm::mat4 viewProj = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;
		u_CameraData.InverseCameraViewProj = glm::inverse( viewProj );

		struct UAtmosParam
		{
			AtmosphereParameters Params;
		} u_AtmosphereParameters{};

		u_AtmosphereParameters.Params = CreateDefaultAtmosphereParameters();

		static const glm::ivec2 kTraceOffsets[ 4 ] = { { 0, 0 }, { 1, 1 }, { 1, 0 }, { 0, 1 } };

		const uint32_t frameIndex = m_RendererData.CloudsFrameIndex++;
		const glm::ivec2 traceOffset = kTraceOffsets[ frameIndex % 4u ];

		// Sun from the scene's directional light.
		const auto& rDirLight = m_pScene->m_Lights.DirectionalLights[ 0 ];

		glm::vec3 sunDirection, sunColor;
		GetSceneSun( rDirLight, sunDirection, sunColor );

		const VolumetricCloudSettings& rSettings = rDirLight.Clouds;

		const float timeSeconds = ( float ) Application::Get()->GetWindow()->GetTime();

		const glm::vec3 cameraPosMeters = glm::vec3( glm::inverse( m_RendererData.CurrentCamera.ViewMatrix )[ 3 ] );

		auto cloudData = MakeCloudsData(
			rSettings,
			timeSeconds,
			frameIndex,
			cameraPosMeters * glm::vec3( 0.001f ),
			u_AtmosphereParameters.Params.BottomRadius,
			sunDirection,
			sunColor );

		cloudData.TraceOffsetX = traceOffset.x;
		cloudData.TraceOffsetY = traceOffset.y;
		cloudData.ReconstructSize = glm::vec2( m_RendererData.CloudsReconstructOut->GetWidth(), m_RendererData.CloudsReconstructOut->GetHeight() );

		cloudData.SkyLightColor = sunColor * rDirLight.Sky.SunIlluminance * rDirLight.Sky.Intensity;
		cloudData.UseSkyAmbient = rDirLight.Sky.Enabled ? 1 : 0;

		CloudsReconstructData reconstruct{};
		reconstruct.InverseViewProj = u_CameraData.InverseCameraViewProj;
		reconstruct.PrevViewProj = m_RendererData.CloudsPrevViewProj;
		reconstruct.CameraPosMeters = glm::vec4( cameraPosMeters, 1.0f );
		reconstruct.TraceOffsetX = traceOffset.x;
		reconstruct.TraceOffsetY = traceOffset.y;
		reconstruct.HistoryValid = m_RendererData.CloudsHistoryValid ? 1 : 0;
		reconstruct.BlendTraced = 0.25f;
		reconstruct.BlendOther = 0.1f;

		m_RendererData.CloudsReconstructMaterial->UploadDataToUB( 4, &reconstruct, sizeof( CloudsReconstructData ) );

		m_RendererData.CloudsRaymarchMaterial->UploadDataToUB( 6, &u_AtmosphereParameters, sizeof( UAtmosParam ) );
		m_RendererData.CloudsRaymarchMaterial->UploadDataToUB( 7, &u_CameraData, sizeof( UCameraData ) );
		m_RendererData.CloudsRaymarchMaterial->UploadDataToUB( 8, ( void* ) &cloudData, sizeof( CloudsData ) );

		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// All cloud images stay in GENERAL 
		// so every barrier only orders the
		// access and not it's layout.
		auto imageBarrier = []( VkImage image,
			VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
			VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess )
		{
			VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
			barrier.srcStageMask = srcStage;
			barrier.srcAccessMask = srcAccess;
			barrier.dstStageMask = dstStage;
			barrier.dstAccessMask = dstAccess;
			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			return barrier;
		};

		auto barriers = [ & ]( std::initializer_list<VkImageMemoryBarrier2> barriers )
		{
			VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			dependencyInfo.imageMemoryBarrierCount = ( uint32_t ) barriers.size();
			dependencyInfo.pImageMemoryBarriers = barriers.begin();

			vkCmdPipelineBarrier2( CommandBuffer, &dependencyInfo );
		};

		const VkImage traceImage       = m_RendererData.CloudsRaymarchOut->GetImage();
		const VkImage distanceImage    = m_RendererData.CloudsDistanceOut->GetImage();
		const VkImage reconstructImage = m_RendererData.CloudsReconstructOut->GetImage();
		const VkImage historyImage     = m_RendererData.CloudsHistory->GetImage();
		const VkImage shadowImage      = m_RendererData.CloudShadowMap->GetImage();

		// Fills an image that is read by fragment shaders (SceneComposite / geometry pass)
		// with a constant, used when clouds or cloud shadows are switched off.
		auto clearForFragmentRead = [ & ]( VkImage image, float r, float g, float b, float a )
		{
			barriers( {
				imageBarrier( image,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT ),
			} );

			VkClearColorValue color{};
			color.float32[ 0 ] = r;
			color.float32[ 1 ] = g;
			color.float32[ 2 ] = b;
			color.float32[ 3 ] = a;

			const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdClearColorImage( CommandBuffer, image, VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range );

			barriers( {
				imageBarrier( image,
					VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),
			} );
		};

		if( !rSettings.Enabled )
		{
			clearForFragmentRead( reconstructImage, 0.0f, 0.0f, 0.0f, 1.0f );
			clearForFragmentRead( shadowImage, 1.0f, 1.0f, 1.0f, 1.0f );

			// The history is stale once the clouds come back.
			m_RendererData.CloudsHistoryValid = false;

			m_RendererData.AtmosphereVCTimer.Stop();
			return;
		}

		// 1. Trace (quarter res)
		barriers( {
			// Transmittance LUT was written by the compute dispatch in AtmospherePass.
			imageBarrier( m_RendererData.TransmittanceLutOut->GetImage(),
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),

			// Last frame's reconstruction read the trace outputs, don't overwrite them early.
			imageBarrier( traceImage,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ),
			imageBarrier( distanceImage,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ),
		} );

		m_RendererData.CloudsRaymarchPipeline->BindWithCommandBuffer( CommandBuffer );

		m_RendererData.CloudsRaymarchPipeline->Execute(
			m_RendererData.CloudsRaymarchMaterial,
			( uint32_t ) glm::ceil( ( float ) m_RendererData.CloudsRaymarchOut->GetWidth() / 8.F ),
			( uint32_t ) glm::ceil( ( float ) m_RendererData.CloudsRaymarchOut->GetHeight() / 8.0F ),
			1u );

		m_RendererData.CloudsRaymarchPipeline->Unbind();

		// 2. Temporal reconstruction (half res)
		barriers( {
			imageBarrier( traceImage,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),
			imageBarrier( distanceImage,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),

			// History was written by last frame's copy.
			imageBarrier( historyImage,
				VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),

			// Last frame's SceneComposite and copy read the output, don't overwrite it early.
			imageBarrier( reconstructImage,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ),
		} );

		m_RendererData.CloudsReconstructPipeline->BindWithCommandBuffer( CommandBuffer );

		m_RendererData.CloudsReconstructPipeline->Execute(
			m_RendererData.CloudsReconstructMaterial,
			( uint32_t ) glm::ceil( ( float ) m_RendererData.CloudsReconstructOut->GetWidth() / 8.F ),
			( uint32_t ) glm::ceil( ( float ) m_RendererData.CloudsReconstructOut->GetHeight() / 8.0F ),
			1u );

		m_RendererData.CloudsReconstructPipeline->Unbind();

		// 3. Copy the result into the history for next frame. A copy keeps every
		//    descriptor binding static instead of ping-ponging two images.
		barriers( {
			imageBarrier( reconstructImage,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),
			imageBarrier( historyImage,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT ),
		} );

		{
			VkImageCopy region{};
			region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.extent = { m_RendererData.CloudsReconstructOut->GetWidth(), m_RendererData.CloudsReconstructOut->GetHeight(), 1 };

			vkCmdCopyImage( CommandBuffer,
				reconstructImage, VK_IMAGE_LAYOUT_GENERAL,
				historyImage, VK_IMAGE_LAYOUT_GENERAL,
				1, &region );
		}

		m_RendererData.CloudsPrevViewProj = viewProj;
		m_RendererData.CloudsHistoryValid = true;

		// 4. Cloud shadow map, sampled by the geometry pass (Lighting.glsl, CloudShadow()).
		if( !rSettings.CastShadows )
		{
			clearForFragmentRead( shadowImage, 1.0f, 1.0f, 1.0f, 1.0f );
		}
		else
		{
			struct CloudShadowData
			{
				glm::vec4 CameraPosMeters;
				float     PlanetRadiusKm;
				float     ShadowStrength;
				int32_t   StepCount;
				float     MinLight;
			} shadowData{};

			static_assert( sizeof( CloudShadowData ) == 32, "CloudShadowData must match the std140 layout in CloudsShadow.glsl" );

			// We use the sample value the geometry pass.
			shadowData.CameraPosMeters = glm::vec4( cameraPosMeters, 1.0f );
			shadowData.PlanetRadiusKm = u_AtmosphereParameters.Params.BottomRadius;
			shadowData.ShadowStrength = glm::max( rSettings.ShadowStrength, 0.0f );
			shadowData.StepCount = 16;
			shadowData.MinLight = glm::clamp( rSettings.ShadowMinLight, 0.0f, 1.0f );

			m_RendererData.CloudShadowMaterial->UploadDataToUB( 8, ( void* ) &cloudData, sizeof( CloudsData ) );
			m_RendererData.CloudShadowMaterial->UploadDataToUB( 11, &shadowData, sizeof( CloudShadowData ) );

			barriers( {
				imageBarrier( shadowImage,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT ),
			} );

			m_RendererData.CloudShadowPipeline->BindWithCommandBuffer( CommandBuffer );

			m_RendererData.CloudShadowPipeline->Execute(
				m_RendererData.CloudShadowMaterial,
				( uint32_t ) glm::ceil( ( float ) m_RendererData.CloudShadowMap->GetWidth() / 8.F ),
				( uint32_t ) glm::ceil( ( float ) m_RendererData.CloudShadowMap->GetHeight() / 8.0F ),
				1u );

			m_RendererData.CloudShadowPipeline->Unbind();

			barriers( {
				imageBarrier( shadowImage,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
					VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT ),
			} );
		}

		m_RendererData.AtmosphereVCTimer.Stop();
	}

	void SceneRenderer::BloomPass()
	{
		SAT_PF_EVENT();

		m_RendererData.BloomTimer.Reset();

		struct u_Settings
		{
			float Threshold;
			float Knee;
			float TK;
			float DK;
			float QK;
			float Lod;
			uint8_t Stage;
		} pc_Settings{};

		pc_Settings.Threshold = m_RendererData.BloomThreshold;
		pc_Settings.Knee = 0.1f;
		pc_Settings.TK = pc_Settings.Threshold - pc_Settings.Knee;
		pc_Settings.DK = pc_Settings.Knee * 2.0F;
		pc_Settings.QK = 0.25f / pc_Settings.Knee;
		pc_Settings.Lod = 0.0f;

		glm::uvec3 workGroups{ 0 };

		m_RendererData.BloomComputePipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		// Prefilter
		pc_Settings.Stage = ( uint8_t ) BloomStage::Prefilter;
		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Prefilter" );
		{
			// Get anything over the bloom threshold
			workGroups = {
				m_RendererData.BloomTextures[ 0 ].Texture->Width() / m_RendererData.BloomWorkSize,
				m_RendererData.BloomTextures[ 0 ].Texture->Height() / m_RendererData.BloomWorkSize,
				1u };

			Buffer pc( sizeof( u_Settings ), &pc_Settings );

			m_RendererData.BloomComputePipeline->ExecuteWithExternalPC( m_RendererData.BloomPrefilterMaterial, pc,
				workGroups.x,
				workGroups.y,
				workGroups.z );
		}
		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		// Downsample chain.
		pc_Settings.Stage = ( uint8_t ) BloomStage::Downsample;
		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Dowsample chain" );

		uint32_t mips = m_RendererData.BloomTextures[ 0 ].Texture->GetMipMapLevels() - 2;
		for( uint32_t i = 1; i < mips; ++i )
		{
			const auto [mipW, mipH] = m_RendererData.BloomTextures[ 0 ].Texture->GetMipSize( i );

			workGroups.x = ( uint32_t ) glm::ceil( mipW / ( float ) m_RendererData.BloomWorkSize );
			workGroups.y = ( uint32_t ) glm::ceil( mipH / ( float ) m_RendererData.BloomWorkSize );

			pc_Settings.Lod = static_cast< float >( i - 1.0f );

			// Render
			Buffer pc( sizeof( u_Settings ), &pc_Settings );

			m_RendererData.BloomComputePipeline->ExecuteWithExternalPC( m_RendererData.BloomDownsampleAMaterials[ i ], pc,
				workGroups.x,
				workGroups.y,
				workGroups.z );

			// B pass
			pc_Settings.Lod = ( float ) i;

			// Render
			pc = Buffer( sizeof( u_Settings ), &pc_Settings );

			m_RendererData.BloomComputePipeline->ExecuteWithExternalPC( m_RendererData.BloomDownsampleBMaterials[ i ], pc,
				workGroups.x,
				workGroups.y,
				workGroups.z );
		}
		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		// Upsample chain.
		pc_Settings.Stage = ( uint8_t ) BloomStage::FirstUpsample;
		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Initial Upsample" );
		{
			--pc_Settings.Lod;

			const auto [mipW, mipH] = m_RendererData.BloomTextures[ 2 ].Texture->GetMipSize( mips - 2 );

			workGroups.x = ( uint32_t ) glm::ceil( mipW / ( float ) m_RendererData.BloomWorkSize );
			workGroups.y = ( uint32_t ) glm::ceil( mipH / ( float ) m_RendererData.BloomWorkSize );

			Buffer pc( sizeof( u_Settings ), &pc_Settings );

			m_RendererData.BloomComputePipeline->ExecuteWithExternalPC( m_RendererData.BloomFirstUpsampleMaterial, pc,
				workGroups.x,
				workGroups.y,
				workGroups.z );
		}
		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		pc_Settings.Stage = ( uint8_t ) BloomStage::Upsample;
		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Upsample chain" );
		{
			for( int32_t i = mips - 3; i >= 0; --i )
			{
				const auto [mipW, mipH] = m_RendererData.BloomTextures[ 2 ].Texture->GetMipSize( i );

				workGroups.x = ( uint32_t ) glm::ceil( mipW / ( float ) m_RendererData.BloomWorkSize );
				workGroups.y = ( uint32_t ) glm::ceil( mipH / ( float ) m_RendererData.BloomWorkSize );

				pc_Settings.Lod = ( float ) i;
				Buffer pc( sizeof( u_Settings ), &pc_Settings );

				m_RendererData.BloomComputePipeline->ExecuteWithExternalPC( m_RendererData.BloomUpsampleMaterials[ i ], pc,
					workGroups.x,
					workGroups.y,
					workGroups.z );
			}
		}
		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		m_RendererData.BloomTimer.Stop();
	}

	void SceneRenderer::SSAOPass()
	{
		SAT_PF_EVENT();

		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.SSAOTimer.Reset();
		m_RendererData.SSAORenderPass->BeginPass( CommandBuffer, m_RendererData.SSAOFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		struct UB_Matrices
		{
			glm::mat4 Projection;
			glm::mat4 InvProjection;
		} u_Matrices{};

		u_Matrices.Projection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix();
		u_Matrices.InvProjection = glm::inverse( m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() );

		m_RendererData.SSAOMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		Renderer::Get()->SubmitFullscreenQuad(
			CommandBuffer, 
			m_RendererData.SSAOPipeline,
			m_RendererData.SSAOMaterial,
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		m_RendererData.SSAORenderPass->EndPass();
		m_RendererData.SSAOTimer.Stop();

		CmdBeginDebugLabel( CommandBuffer, "SSAO-Blur" );
		SSAOBlurPass();
		CmdEndDebugLabel( CommandBuffer );
	}

	void SceneRenderer::SSAOBlurPass()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.AOBlurTimer.Reset();
		m_RendererData.AOBlurRenderPass->BeginPass( CommandBuffer, m_RendererData.AOBlurFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		Renderer::Get()->SubmitFullscreenQuad(
			CommandBuffer,
			m_RendererData.AOBlurPipeline,
			m_RendererData.AOBlurMaterial,
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		m_RendererData.AOBlurRenderPass->EndPass();
		m_RendererData.AOBlurTimer.Stop();
	}

	void SceneRenderer::GTAOPass()
	{
		SAT_PF_EVENT();

		m_RendererData.GTAOTimer.Reset();

		const auto commandBuffer = Renderer::Get()->ActiveCommandBuffer();

		CmdBeginDebugLabel( commandBuffer, "GTAO-Prefilter" );
		GTAOPrefilterPass();
		CmdEndDebugLabel( commandBuffer );

		CmdBeginDebugLabel( commandBuffer, "GTAO-MainPass" );
		GTAOMainPass();
		CmdEndDebugLabel( commandBuffer );

		CmdBeginDebugLabel( commandBuffer, "GTAO-DenoiseEntry" );
		GTAODenoisePass();
		CmdEndDebugLabel( commandBuffer );

		m_RendererData.GTAOTimer.Stop();
	}

	void SceneRenderer::GTAOPrefilterPass()
	{
		struct u_Params
		{
			glm::vec2 DepthUnpackConsts{};
			glm::vec2 ViewportPixelSize{};
			float EffectRadius = 0.0f;
			float EffectFalloffRange = 0.0f;
			float RadiusMultiplier = 0.0f;
		} pc_Params;

		const auto& projectionMatrix = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix();

		pc_Params.DepthUnpackConsts = { projectionMatrix[ 3 ][ 2 ], projectionMatrix[ 2 ][ 2 ] };

		pc_Params.ViewportPixelSize = { m_RendererData.Width, m_RendererData.Height };
		pc_Params.EffectRadius = m_RendererData.GTAOEffectRadius;
		pc_Params.EffectFalloffRange = m_RendererData.GTAOEffectFalloffRange;
		pc_Params.RadiusMultiplier = m_RendererData.GTAORadiusMultiplier;

		m_RendererData.GTAOPrefilterPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		glm::uvec3 workGroups{ 1 };
		workGroups.x = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Width ) / 16.0f );
		workGroups.y = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Height ) / 16.0f );

		Buffer pc( sizeof( u_Params ), &pc_Params );

		m_RendererData.GTAOPrefilterPipeline->ExecuteWithExternalPC( m_RendererData.GTAOPrefilterMaterial, pc,
			workGroups.x,
			workGroups.y,
			workGroups.z );
	}

	void SceneRenderer::GTAOMainPass()
	{
		struct u_Params
		{
			float EffectRadius = 0.0f;
			float EffectFalloffRange = 0.0f;
			float RadiusMultiplier = 0.0f;

			float _pad = 0.0f;

			glm::vec2 NDCToViewMul_x_PixelSize{};
			float FinalValuePower = 0.0f;
			float SampleDistributionPower = 0.0f;
			float ThinOccluderCompensation = 0.0f;
			float DepthMipSamplingOffset = 0.0f;
			float SliceCount = 0.0f;
			float StepsPerSlice = 0.0f;
		} pc_Params;

		pc_Params.EffectRadius = m_RendererData.GTAOEffectRadius;
		pc_Params.EffectFalloffRange = m_RendererData.GTAOEffectFalloffRange;
		pc_Params.RadiusMultiplier = m_RendererData.GTAORadiusMultiplier;
		pc_Params.FinalValuePower = 2.2f;
		pc_Params.SampleDistributionPower = 2.0f;
		pc_Params.ThinOccluderCompensation = 0.0f;
		pc_Params.DepthMipSamplingOffset = 3.3f;
		pc_Params.SliceCount = 3.0f;
		pc_Params.StepsPerSlice = 3.0f;

		struct UNdcData
		{
			glm::ivec2 ViewportSize{};
			glm::vec2 ViewportPixelSize{};
			glm::vec2 NDCToViewMul{};
			glm::vec2 NDCToViewAdd{};
			glm::vec2 DepthUnpackConsts{};
		} u_ExtraData{};
		
		const auto& rProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix();
		const float halfTanFovX = 1.0f / rProjection[ 0 ][ 0 ];
		const float halfTanFovY = 1.0f / rProjection[ 1 ][ 1 ];

		u_ExtraData.ViewportSize = { ( int ) m_RendererData.Width, ( int ) m_RendererData.Height };
		u_ExtraData.ViewportPixelSize = { 1.0f / ( float ) m_RendererData.Width, 1.0f / ( float ) m_RendererData.Height };
		u_ExtraData.NDCToViewMul = { halfTanFovX * 2.0f, halfTanFovY * -2.0f };
		u_ExtraData.NDCToViewAdd = { halfTanFovX * -1.0f, halfTanFovY * 1.0f };
		u_ExtraData.DepthUnpackConsts = { rProjection[ 3 ][ 2 ], rProjection[ 2 ][ 2 ] };

		pc_Params.NDCToViewMul_x_PixelSize = u_ExtraData.NDCToViewMul * u_ExtraData.ViewportPixelSize;

		m_RendererData.GTAOMainMaterial->UploadDataToUB( 5u, &u_ExtraData, sizeof( UNdcData ) );

		m_RendererData.GTAOMainPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		glm::uvec3 workGroups{ 1 };
		workGroups.x = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Width ) / 8.0f );
		workGroups.y = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Height ) / 8.0f );

		Buffer pc( sizeof( u_Params ), &pc_Params );

		m_RendererData.GTAOMainPipeline->ExecuteWithExternalPC( m_RendererData.GTAOMainMaterial, pc,
			workGroups.x,
			workGroups.y,
			workGroups.z );
	}

	void SceneRenderer::GTAODenoisePass()
	{
		const auto commandBuffer = Renderer::Get()->ActiveCommandBuffer();

		struct u_Params
		{
			glm::ivec2 ViewportSize{};
			float DenoiseBeta = 0.0f;
			uint32_t HalfRes = 0u;
			uint32_t FinalApply = 0u;
		} pc_Params;

		pc_Params.ViewportSize = { m_RendererData.Width, m_RendererData.Height };
		pc_Params.DenoiseBeta = 1.20f;
		pc_Params.HalfRes = 0u;

		for( size_t i = 0; i < m_RendererData.GTAODenoisePassCount; ++i )
		{
			CmdBeginDebugLabel( commandBuffer, "Denoise" );

			const auto& rInformation = m_RendererData.GTAODenoisePassesInformation[ i ];

			m_RendererData.GTAODenoisePipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

			glm::uvec3 workGroups{ 1 };
			workGroups.x = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Width ) / 16.0f );
			workGroups.y = ( uint32_t ) glm::ceil( static_cast< float >( m_RendererData.Height ) / 8.0f );

			if( i == m_RendererData.GTAODenoisePassCount - 1 )
				pc_Params.FinalApply = 1u;

			Buffer pc( sizeof( u_Params ), &pc_Params );

			m_RendererData.GTAODenoisePipeline->ExecuteWithExternalPC( rInformation.Material, pc,
				workGroups.x,
				workGroups.y,
				workGroups.z );

			CmdEndDebugLabel( commandBuffer );
		}
	}

	void SceneRenderer::SelectedGeometryPass()
	{
		SAT_PF_EVENT();

		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.SelectedGeometryPass->BeginPass( CommandBuffer, m_RendererData.SelectedGeometryFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		// Render
		struct UB_Matrices
		{
			glm::mat4 ViewProjection;
		} u_Matrices{};

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		m_RendererData.SelectedGeometryMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		for( auto& [key, Cmd] : m_SelectedStaticMeshDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			Renderer::Get()->RenderMeshWithoutMaterial(
				CommandBuffer,
				m_RendererData.SelectedGeometryPipeline,
				Cmd.Mesh,
				m_RendererData.SelectedGeometryMaterial,
				m_RendererData.UniformBufferSet,
				m_RendererData.StorageBufferSet,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset,
				Cmd.SubmeshIndex );
		}

		uint32_t index = 0;
		for( auto& [key, Cmd] : m_DynamicSelectedMeshDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			Renderer::Get()->RenderDynamicMeshWithoutMaterial(
				CommandBuffer,
				m_RendererData.SelectedGeometryDynamicPipeline,
				Cmd.Mesh,
				m_RendererData.SelectedGeometryMaterial,
				m_RendererData.UniformBufferSet,
				m_RendererData.StorageBufferSet,
				Cmd.Instances,
				m_RendererData.SubmeshTransformData[ frame ].VertexBuffer,
				rTransformData.Offset,
				Cmd.SubmeshIndex, index, m_RendererData.PreDepthDynamicMaterialSet2 );

			index += Cmd.Instances;
		}

		m_RendererData.SelectedGeometryPass->EndPass();
	}

	void SceneRenderer::JumpFloodPass()
	{
		SAT_PF_EVENT();

		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.JumpFloodFirstPass->BeginPass( CommandBuffer, m_RendererData.JumpFloodFirstPassFB->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = ( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		Renderer::Get()->SubmitFullscreenQuad( 
			CommandBuffer, 
			m_RendererData.JumpFloodFirstPipeline, 
			m_RendererData.JumpFloodFirstMaterial, 
			m_RendererData.QuadIndexBuffer, 
			m_RendererData.QuadVertexBuffer 
		);

		m_RendererData.JumpFloodFirstPass->EndPass();

		const int steps = 2;
		int step = ( int ) glm::round( glm::pow<int>( steps - 1, 2 ) );
		glm::vec2 texelSize = { 1.0f / ( float ) m_RendererData.JumpFloodEvenFB->GetWidth(), 1.0f / ( float ) m_RendererData.JumpFloodEvenFB->GetHeight() };

		struct JmpFPushConst
		{
			glm::vec2 TexelSize{};
			int Step = 0;
		} pc_JmpFlood;

		pc_JmpFlood.TexelSize = texelSize;
		pc_JmpFlood.Step = step;

		Buffer PushConstData( sizeof( JmpFPushConst ), &pc_JmpFlood );

		// Begin even pass
		m_RendererData.JumpFloodEvenPass->BeginPass( CommandBuffer, m_RendererData.JumpFloodEvenFB->GetVulkanFramebuffer(), Extent );

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		Renderer::Get()->SubmitFullscreenQuadPushConst(
			CommandBuffer,
			m_RendererData.JumpFloodEvenPipeline,
			m_RendererData.JumpFloodEvenMaterial,
			m_RendererData.QuadIndexBuffer,
			m_RendererData.QuadVertexBuffer,
			PushConstData
		);

		m_RendererData.JumpFloodEvenPass->EndPass();
	}

	void SceneRenderer::AddScheduledFunction( ScheduledFunc&& rrFunc )
	{
		m_ScheduledFunctions.push_back( rrFunc );
	}

#if !defined(SAT_DIST)
	void SceneRenderer::OnShaderReloaded( const std::string& rName )
	{
		const Ref<Shader> shader = ShaderLibrary::Get().Find( rName );
		auto& rReference = Renderer::Get()->FindOrCreateShaderReference( shader->GetShaderHash() );

		for( auto& rPipeline : rReference.Pipelines )
		{
			rPipeline->Recreate();
		}
	}
#endif

	void SceneRenderer::DisableOrEnableBloom()
	{
		if( m_RendererData.EnableBloom )
		{
			m_RendererData.SceneCompositeFlags &= ~SceneCompositeFlag_NoBloom;
			m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ].Texture );
		}
		else
		{
			m_RendererData.SceneCompositeFlags |= SceneCompositeFlag_NoBloom;
			m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomTexture", Renderer::Get()->GetPinkTexture() );
		}
	}

	void SceneRenderer::DisableAO()
	{
		const auto oldTechnique = m_AOTechnique;

		m_AOTechnique = AOTechnique::None;
		InitAO( oldTechnique );
	}

	void SceneRenderer::CreateBloomMaterials()
	{
		const glm::uvec2 viewportSize = { m_RendererData.Width, m_RendererData.Height };

		glm::uvec2 bs = ( viewportSize + 1u ) / 2u;
		bs += m_RendererData.BloomWorkSize - bs % m_RendererData.BloomWorkSize;

		m_RendererData.BloomTextures.fill( {} );

		// Create bloom textures
		for( uint32_t t = 0; t < 3; t++ )
		{
			auto& rTextureInfo = m_RendererData.BloomTextures[ t ];

			rTextureInfo.Texture = Ref<Texture2D>::Create( ImageFormat::RGBA32F, bs.x, bs.y, nullptr, true, AddressingMode::ClampToEdge );
			rTextureInfo.Texture->SetDebugName( "Bloom Texture/" + std::to_string( t ) );

			for( size_t i = 0; i < rTextureInfo.Texture->GetMipMapLevels(); ++i )
			{
				auto ds = rTextureInfo.Texture->GetDescriptorInfo();
				ds.imageView = rTextureInfo.Texture->GetOrCreateMipImageView( ( uint32_t ) i );

				SetDebugUtilsObjectName( std::format( "Bloom Texture/{0}, mip/{1}", t, i ), ( uint64_t ) ds.imageView, VK_OBJECT_TYPE_IMAGE_VIEW );

				rTextureInfo.ImageInfos.push_back( ds );
			}
		}

		// Materials
		if( !m_RendererData.BloomPrefilterMaterial )
			m_RendererData.BloomPrefilterMaterial = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom-Prefliter" );

		// Set prefliter data.
		m_RendererData.BloomPrefilterMaterial->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 0 ].Texture, m_RendererData.BloomTextures[ 0 ].ImageInfos[ 0 ] );
		m_RendererData.BloomPrefilterMaterial->SetResource( "u_InputTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );
		m_RendererData.BloomPrefilterMaterial->SetResource( "u_BloomTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );

		// Downsample
		uint32_t mips = m_RendererData.BloomTextures[ 0 ].Texture->GetMipMapLevels() - 2;
		m_RendererData.BloomDownsampleAMaterials.resize( mips );
		m_RendererData.BloomDownsampleBMaterials.resize( mips );

		for( uint32_t i = 0; i < mips; ++i )
		{
			Ref<Material> aMaterial = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom Downsample A" );

			aMaterial->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 1 ].Texture, m_RendererData.BloomTextures[ 1 ].ImageInfos[ i ] );
			aMaterial->SetResource( "u_InputTexture", m_RendererData.BloomTextures[ 0 ].Texture );
			aMaterial->SetResource( "u_BloomTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );

			Ref<Material> bMaterial = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom Downsample B" );

			bMaterial->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 0 ].Texture, m_RendererData.BloomTextures[ 0 ].ImageInfos[ i ] );
			bMaterial->SetResource( "u_InputTexture", m_RendererData.BloomTextures[ 1 ].Texture );
			bMaterial->SetResource( "u_BloomTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );
		
			m_RendererData.BloomDownsampleAMaterials[ i ] = aMaterial;
			m_RendererData.BloomDownsampleBMaterials[ i ] = bMaterial;
		}

		// Upsampling
		m_RendererData.BloomFirstUpsampleMaterial = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom First Upsample" );

		m_RendererData.BloomFirstUpsampleMaterial->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 2 ].Texture, m_RendererData.BloomTextures[ 2 ].ImageInfos[ mips - 2 ] );
		m_RendererData.BloomFirstUpsampleMaterial->SetResource( "u_InputTexture", m_RendererData.BloomTextures[ 0 ].Texture );
		m_RendererData.BloomFirstUpsampleMaterial->SetResource( "u_BloomTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );

		m_RendererData.BloomUpsampleMaterials.resize( ( mips - 3 ) + 1 );
		for( int32_t i = mips - 3; i >= 0; --i )
		{
			m_RendererData.BloomUpsampleMaterials[ i ] = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom Upsample" );

			m_RendererData.BloomUpsampleMaterials[ i ]->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 1 ].Texture, m_RendererData.BloomTextures[ 2 ].ImageInfos[ i ] );
			m_RendererData.BloomUpsampleMaterials[ i ]->SetResource( "u_InputTexture", m_RendererData.BloomTextures[ 0 ].Texture );
			m_RendererData.BloomUpsampleMaterials[ i ]->SetResource( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ].Texture );
		}
	}

	void SceneRenderer::BindSceneCompositeAOTexture()
	{
		switch( m_AOTechnique )
		{
			default:
			case AOTechnique::None:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_AOTexture", Renderer::Get()->GetPinkTexture() );
				break;

			case AOTechnique::SSAO:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_AOTexture", m_RendererData.AOBlurFramebuffer->GetColorAttachmentsResources()[ 0 ] );
				break;

			case AOTechnique::HBAO:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_AOTexture", Renderer::Get()->GetPinkTexture() );
				break;

			case AOTechnique::GTAO:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_AOTexture", m_RendererData.GTAODenoisePassesInformation.back().OutImage );
				break;
		}
	}

	void SceneRenderer::SendBoneDataToMap( Ref<SkeletalMesh> mesh, const StaticMeshKey& rKey, const std::vector<glm::mat4>& rBoneTransforms )
	{
		SAT_PF_EVENT();

		// [DRAW CALL 0], 64 bones, 3 instances
		//  instance 0
		//  [bone transform data] from 0-63
		//  fill remaining to 99
		//  instance 1
		//  [bone transform data] from 100-199
		//  fill remaining to 99
		//  instance 2
		//  [bone transform data] from 200-299
		//  fill remaining to 99
		// [DRAW CALL 1], 64 bones, 3 instances
		// same as above but start at 300...

		// Every submesh is to have 100 (or elsewhere in SK_MAX_BONES) bone transforms.
		// However, not every mesh actually has 100 bones for example, the mesh that this code was debugged with has 64 bones
		// So, thats why we have to do a workaround to ensure that the bones transforms is correct because if we don't set them (and use the Meshes' bone count) we will end up reading garbage data.
		// This could be solved if we simply was able to fill the whole buffer with glm::mat4{0.0f} 

		auto& rBoneTransformMap = m_RendererData.BoneTransformMap[ rKey ];

		const size_t boneCount = rBoneTransformMap.Data.size();
		const size_t stride = SK_MAX_BONES + boneCount;

		rBoneTransformMap.Stride = ( uint32_t ) stride;

		if( rBoneTransformMap.Data.empty() )
		{
			rBoneTransformMap.Data.resize( SK_MAX_BONES );
		}

		auto skeleton = mesh->GetSkeletonAsset();
		for( size_t s = 0; s < skeleton->GetBoneInfo().size(); ++s )
		{
			rBoneTransformMap.Data[ s ] = skeleton->GetTransform() * rBoneTransforms[ skeleton->GetBoneInfo().at( s ).BoneIndex ] * skeleton->GetBoneInfo().at( s ).InverseBindPose;
		}
	}

	Ref<TextureCube> SceneRenderer::CreateDymanicSky()
	{
		constexpr uint32_t kCubemapSize = 512;
		constexpr uint32_t kIrradianceMap = 32;

		Ref<TextureCube> Environment = Ref<TextureCube>::Create( ImageFormat::RGBA32F, kCubemapSize, kCubemapSize );

		Ref<Shader> skyShader = ShaderLibrary::Get().Find( "Skybox-Compute" );
		Ref<ComputePipeline> pipeline = Ref<ComputePipeline>::Create( skyShader );

		const glm::vec3 params = { m_RendererData.SceneEnvironment.Turbidity, m_RendererData.SceneEnvironment.Azimuth, m_RendererData.SceneEnvironment.Inclination };

		m_RendererData.PreethamMaterial->SetResource( "o_CubeMap", Environment );
		m_RendererData.PreethamMaterial->SetPC( "pc_Params.Params", params );

		pipeline->Bind();
		pipeline->Execute( m_RendererData.PreethamMaterial, kCubemapSize / kIrradianceMap, kCubemapSize / kIrradianceMap, 6 );
		pipeline->Unbind();

		Environment->CreateMips();

		pipeline = nullptr;

		return Environment;
	}

	void SceneRenderer::SetDynamicSky( float Turbidity, float Azimuth, float Inclination )
	{
		AddScheduledFunction([&, turbidity = Turbidity, azimuth = Azimuth, inclination = Inclination]()
			{
				m_RendererData.SceneEnvironment.Turbidity = turbidity;
				m_RendererData.SceneEnvironment.Azimuth = azimuth;
				m_RendererData.SceneEnvironment.Inclination = inclination;

				m_RendererData.SceneEnvironment.IrradianceMap = nullptr;
				m_RendererData.SceneEnvironment.RadianceMap = nullptr;

				Ref<TextureCube> map = CreateDymanicSky();

				m_RendererData.SceneEnvironment.IrradianceMap = map;
				m_RendererData.SceneEnvironment.RadianceMap = map;

			} );
	}

	bool SceneRenderer::HasFlag( SceneRendererFlags flag ) const
	{
		return ( m_Flags & flag ) != 0;
	}

	Ref<Renderer2D> SceneRenderer::GetRenderer2D() const
	{
		return m_Renderer2D;
	}

	Ref<AluraRenderer> SceneRenderer::GetAluraRenderer() const
	{
		return m_AluraRenderer;
	}

	void SceneRenderer::InitBuffers()
	{
		SAT_PF_EVENT();

		// Create our buffers for instance data.
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		uint32_t off = 0;
		for( auto& [id, buffer] : m_RendererData.MeshTransforms )
		{
			buffer.Offset = off * sizeof( TransformBufferData );
			for( const auto& transform : buffer.Data )
			{
				m_RendererData.SubmeshTransformData[ frame ].pData[ off ] = transform;
				++off;
			}
		}

		m_RendererData.SubmeshTransformData[ frame ].VertexBuffer->SetData( m_RendererData.SubmeshTransformData[ frame ].pData, off * sizeof( TransformBufferData ) );
	
		off = 0;
		for( auto& [id, buffer] : m_RendererData.BoneTransformMap )
		{
			buffer.Offset = off;

			std::memcpy( &m_RendererData.BoneTransformData[ off ], buffer.Data.data(), buffer.Data.size() * sizeof( glm::mat4 ) );
			off += ( uint32_t ) buffer.Data.size();
		}

		if( off > 0 )
		{
			// upload
			auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
			auto bufferAloc = pAllocator->GetAllocationFromBuffer( m_RendererData.SBBoneTransforms->Get( 2, 15, frame )->GetBuffer() );

			void* pBufferData = pAllocator->MapMemory<void>( bufferAloc );
			std::memcpy( pBufferData, ( const uint8_t* ) m_RendererData.BoneTransformData, ( uint32_t ) off * sizeof( glm::mat4 ) );
			pAllocator->UnmapMemory( bufferAloc );
		}
	}

	void SceneRenderer::InitRenderer2D()
	{
		if( !HasFlag( SceneRendererFlag_NoRenderer2D ) )
		{
			m_Renderer2D = Ref<Renderer2D>::Create();
			m_Renderer2D->Init( m_RendererData.LateCompositePass, m_RendererData.LateCompositeFramebuffer );
		}
	}

	void SceneRenderer::InitAlura()
	{
		if( !HasFlag( SceneRendererFlag_NoAlura ) )
		{
			m_AluraRenderer = Ref<AluraRenderer>::Create();
			m_AluraRenderer->Init( m_RendererData.LateCompositePass, m_RendererData.LateCompositeFramebuffer );
		}
	}

	void SceneRenderer::InitPipelineCache()
	{
		m_GeometryPipelineCache = Ref<PipelineCache>::Create();
	}

	class ScopedDebugLabel
	{
	public:
		ScopedDebugLabel( VkCommandBuffer _CommandBuffer, const char* pName ) 
			: CommandBuffer( _CommandBuffer )
		{
			CmdBeginDebugLabel( CommandBuffer, pName );
		}
		
		~ScopedDebugLabel() 
		{
			CmdEndDebugLabel( CommandBuffer );
		}

	private:
		VkCommandBuffer CommandBuffer;
	};

	// Pre Render phase 1
	void SceneRenderer::PreRender()
	{
		if( m_Renderer2D )
			m_Renderer2D->PreRender();

		if( m_AluraRenderer )
			m_AluraRenderer->PreRender();
	}

	void SceneRenderer::RenderScene()
	{
		SAT_PF_EVENT();

		if( !m_pScene )
		{
			FlushDrawList();
			return;
		}

		if( m_RendererData.Resized )
		{
			Recreate();

			m_RendererData.Resized = false;
		}

		m_RendererData.CommandBuffer = Renderer::Get()->ActiveCommandBuffer();

		for( auto&& func : m_ScheduledFunctions )
			func();

		// Pre Render phase 2
		InitBuffers();

		// Passes

		DirShadowMapPass();

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "PreDepth" );
			PreDepthPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "LightCulling" );
			LightCullingPass();
		}
	
		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Atmosphere" );
			AtmospherePass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "AtmosphereVC" );
			VolumetricCloudsPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Geometry" );
			GeometryPass();
		}

		if( m_RendererData.EnableBloom )
		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Bloom" );
			BloomPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "SelectedGeometry" );
			SelectedGeometryPass();
		}

		switch( m_AOTechnique )
		{
			default:
			case AOTechnique::None:
			case AOTechnique::HBAO:
				break;

			case AOTechnique::SSAO:
			{
				ScopedDebugLabel label( m_RendererData.CommandBuffer, "SSAO" );
				SSAOPass();
			} break;

			case AOTechnique::GTAO:
			{
				ScopedDebugLabel label( m_RendererData.CommandBuffer, "GTAO" );
				GTAOPass();
			} break;
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "JumpFlood" );
			JumpFloodPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Scene Composite/PostProcessing" );
			SceneCompositePass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Late Composite/SceneRenderer" );
			LateCompPhysicsOutline();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Late Composite/JmpFlood" );
			JumpFloodLatePass();
		}

		if( m_Renderer2D )
		{
			m_Renderer2D->Render();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "SMAA" );
			SMAAPass();
		}

		if( m_AluraRenderer )
		{
			m_AluraRenderer->Render();
		}

		if( m_RendererData.IsSwapchainTarget )
		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Scene Composite/Texture Pass" );
			TexturePass();
		}

		FlushDrawList();
	}

	void SceneRenderer::FlushDrawList()
	{
		m_DrawList.clear();
		m_DynamicDrawList.clear();
		m_ShadowMapDrawList.clear();
		m_DynamicShadowMapDrawList.clear();
		m_PhysicsColliderDrawList.clear();
		m_SelectedStaticMeshDrawList.clear();
		m_DynamicSelectedMeshDrawList.clear();
		m_ScheduledFunctions.clear();
		m_RendererData.MeshTransforms.clear();
		m_RendererData.BoneTransformMap.clear();
	}

	void SceneRenderer::HandleKeCommand( const std::string& rKey, const std::string& rValue )
	{
		if( rKey == "FPS" )
		{
			if( rValue == "TRUE" )
			{
				// todo
			}
			else // anything else assume false
			{
				// todo
			}
		}
		else if( rKey == "AO" )
		{
			if( rValue == "SSAO" )
			{
				if( m_AOTechnique != AOTechnique::SSAO )
				{
					const auto oldTechnique = m_AOTechnique;

					m_AOTechnique = AOTechnique::SSAO;
					InitAO( oldTechnique );
				}
			}
			else if( rValue == "GTAO" )
			{
				if( m_AOTechnique != AOTechnique::GTAO ) 
				{
					const auto oldTechnique = m_AOTechnique;
					
					m_AOTechnique = AOTechnique::GTAO;
					InitAO( oldTechnique );
				}
			}
			else if( rValue == "NONE" )
			{
				if( m_AOTechnique != AOTechnique::None ) 
				{
					DisableAO();
				}
			}
		}
		else if( rKey == "SHADOWS" )
		{
			if( rValue == "TRUE" )
			{
				m_RendererData.EnableShadows = true;
			}
			else
			{
				m_RendererData.EnableShadows = false;
			}
		}
		else if( rKey == "BLOOM" )
		{
			if( rValue == "TRUE" )
			{
				m_RendererData.EnableBloom = true;
				m_RendererData.SceneCompositeFlags &= ~SceneCompositeFlag_NoBloom;
			}
			else
			{
				m_RendererData.EnableBloom = false;
				m_RendererData.SceneCompositeFlags |= SceneCompositeFlag_NoBloom;
			}
		}
	}

	void SceneRenderer::SetCamera( const RendererCamera& Camera )
	{
		m_RendererData.CurrentCamera = Camera;

		if( m_Renderer2D )
			m_Renderer2D->SetCamera( Camera );
	}

	//////////////////////////////////////////////////////////////////////////
	// RendererData
	//////////////////////////////////////////////////////////////////////////

	void RendererData::Terminate()
	{
		if( !Application::Get()->HasFlag( ApplicationFlag_CreateSceneRenderer_DEPRECATED ) )
			return;

		// This whole thing should not exist!
		// But it does because when I wrote the SceneRenderer class I was 14 years old, 
		// and didn't care about doing a proper shutdown...
		// Which is why some objects have Destroy()/Terminate() functions and others are just reset.

		ClearSSAOResources();
		ClearHBAOResources();
		ClearGTAOResources();

		// Vertex and Index buffers
		QuadVertexBuffer->Destroy();
		QuadIndexBuffer->Destroy();

		// Framebuffers
		GeometryFramebuffer       = nullptr;
		SceneCompositeFramebuffer = nullptr;
		PreDepthFramebuffer       = nullptr;
		LateCompositeFramebuffer  = nullptr;
		SMAACompFB				  = nullptr;

		BloomTextures.fill( {} );

		for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
			ShadowCascades[ i ].Framebuffer = nullptr;

		JumpFloodOddFB = nullptr;
		JumpFloodEvenFB = nullptr;
		JumpFloodFirstPassFB = nullptr;

		ShadowCascades.clear();

		// Render Passes
		for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
			DirShadowMapPasses[ i ]->Terminate();

		GeometryPass->Terminate();
		SceneComposite->Terminate();

		GeometryPass = nullptr;
		SceneComposite = nullptr;

		PreDepthPass->Terminate();

		LateCompositePass->Terminate();
		LateCompositePass = nullptr;

		SelectedGeometryPass->Terminate();
		SelectedGeometryPass = nullptr;

		JumpFloodFirstPass->Terminate();
		JumpFloodOddPass->Terminate();
		JumpFloodEvenPass->Terminate();

		JumpFloodOddPass = nullptr;
		JumpFloodFirstPass = nullptr;
		JumpFloodEvenPass = nullptr;

		SMAACompPass->Terminate();
		SMAACompPass = nullptr;

		// Pipelines
		SceneCompositePipeline = nullptr;

		for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
			DirShadowMapPipelines[ i ] = nullptr;

		StaticMeshPipeline      = nullptr;
		GridPipeline            = nullptr;
		SkyboxPipeline          = nullptr;
		PreDepthPipeline        = nullptr;
		LightCullingPipeline    = nullptr;
		BloomComputePipeline    = nullptr;
		PhysicsOutlinePipeline  = nullptr;

		// Shaders
		GridShader              = nullptr;
		SkyboxShader            = nullptr;
		StaticMeshShader        = nullptr;
		SceneCompositeShader    = nullptr;
		DirShadowMapShader      = nullptr;
		PreethamShader          = nullptr;
		SSAOBlurShader		    = nullptr;
		PreDepthShader          = nullptr;
		LightCullingShader      = nullptr;
		BloomShader             = nullptr;
		PhysicsOutlineShader    = nullptr;

		// Vertex & Index Buffer
		QuadVertexBuffer        = nullptr;
		QuadIndexBuffer         = nullptr;

		// Textures
		BRDFLUT_Texture         = nullptr;
		BloomDirtTexture        = nullptr;

		for( auto& buffer : SubmeshTransformData )
			delete[] buffer.pData;
		
		delete[] BoneTransformData;

		// Storage buffer set
		StorageBufferSet = nullptr;

		SubmeshTransformData.clear();

		MeshTransforms.clear();
	}

	void RendererData::ClearSSAOResources()
	{
		if( SSAOPipeline )
		{
			SSAOPipeline->Terminate();
			SSAOPipeline = nullptr;
		}

		if( SSAORenderPass )
		{
			SSAORenderPass->Terminate();
			SSAORenderPass = nullptr;
		}

		SSAOShader = nullptr;
		SSAOBlurShader = nullptr;
		SSAOFramebuffer = nullptr;
		SSAONoiseImage = nullptr;
		SSAOMaterial = nullptr;

		if( AOBlurRenderPass )
		{
			AOBlurRenderPass->Terminate(); 
			AOBlurRenderPass = nullptr;
		}
	
		if( AOBlurPipeline )
		{
			AOBlurPipeline->Terminate();
			AOBlurPipeline = nullptr;
		}

		AOBlurFramebuffer = nullptr;
		AOBlurMaterial = nullptr;
	}

	void RendererData::ClearHBAOResources()
	{

	}

	void RendererData::ClearGTAOResources()
	{
		GTAOImageInfos.clear();
		GTAODenoisePassesInformation.clear();
	}

}
