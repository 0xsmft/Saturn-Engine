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
#include "SceneRenderer.h"

#include "Saturn/Core/Renderer/RenderThread.h"
#include "Saturn/Core/Random.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "Texture.h"
#include "Mesh.h"
#include "Material.h"
#include "ComputePipeline.h"
#include "Renderer2D.h"
#include "DefaultMeshes.h"
#include "AluraRenderer.h"

#include "Saturn/Animation/SkeletonAsset.h"

#include "Saturn/Core/Buffer.h"
#include "Saturn/Core/Profiler.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#endif

#include <glm/gtx/matrix_decompose.hpp>

constexpr auto M_PI = 3.14159265358979323846;
constexpr auto SHADOW_MAP_SIZE = 4096.0f;

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	SceneRenderer::SceneRenderer( SceneRendererFlags flags )
		: m_Flags( flags )
	{
		Init();
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

		if( !Application::Get()->HasFlag( ApplicationFlag_CreateSceneRenderer_DEPRECATED ) )
			return;

		m_RendererData.StorageBufferSet = Ref<StorageBufferSet>::Create( 0, 0 );
		m_RendererData.StorageBufferSet->Create( 0, 14 ); // Create Light culling buffer.

		m_RendererData.SBBoneTransforms = Ref<StorageBufferSet>::Create( 0, 0 );
		m_RendererData.SBBoneTransforms->Create( 2, 15, false ); // Create bone transform buffers.
		m_RendererData.SBBoneTransforms->Resize( 2, 15, sizeof( glm::mat4 ) * 10240 );

		// 1024 max animated meshes
		m_RendererData.BoneTransformData = new glm::mat4[ 1 * 1024 ]{};

		for( size_t i = 0; i < 1024; i++ )
		{
			m_RendererData.BoneTransformData[ i ] = glm::mat4( 1.0f ); // identity
		}

		m_RendererData.IsSwapchainTarget = HasFlag( SceneRendererFlag_SwapchainTarget );

		m_RendererData.UniformBufferSet = Ref<UniformBufferSet>::Create();
		// Fill out UBS
		//																	SIZE -> BINDING
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBStaticMeshMatrices ), 0u );
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBLightData ), 1u );
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBSceneData ), 2u );
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBShadowData ), 3u );
		m_RendererData.UniformBufferSet->CreateBuffer( 4llu, 12u ); // Debug Data
		m_RendererData.UniformBufferSet->CreateBuffer( sizeof( UBPointLights ), 13u );

		InitPreDepth();

		InitGeometryPass();

		// Create grid.
		CreateGridComponents();

		// Create skybox.
		CreateSkyboxComponents();

		InitDirShadowMap();

		InitBloom();

		InitSceneComposite();

		InitLateComposite();

		InitSelectionPass();

		InitJumpFlood();

		InitTexturePass();

		switch( m_AOTechnique )
		{
			case AOTechnique::SSAO:
				InitSSAO();
				break;

			case AOTechnique::HBAO:
				InitHBAO();
				break;
		
			case AOTechnique::None:
			default:
				break;
		}

		m_RendererData.SceneEnvironment = Ref<EnvironmentMap>::Create();

		// TODO: Package BRDF texture into AssetBundle in dist
		m_RendererData.BRDFLUT_Texture = Ref<Texture2D>::Create( "content/textures/BRDF_LUT.tga", AddressingMode::Repeat, false );

		constexpr size_t TransformCount = static_cast<size_t>( 1024 ) * 10;
		m_RendererData.SubmeshTransformData.resize( MAX_FRAMES_IN_FLIGHT );
		
		for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			m_RendererData.SubmeshTransformData[ i ].VertexBuffer = Ref<VertexBuffer>::Create( sizeof( TransformBufferData ) * TransformCount );
			m_RendererData.SubmeshTransformData[ i ].pData = new TransformBufferData[ TransformCount ];
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
		if( m_RendererData.GeometryPass )
			m_RendererData.GeometryPass->Recreate();
		else
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "Geometry Pass";
			PassSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16F, ImageFormat::RGBA16F, ImageFormat::DEPTH24STENCIL8 };
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
		FBSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16F, ImageFormat::RGBA16F };

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

		if( m_RendererData.StaticMeshPipeline )
			m_RendererData.StaticMeshPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

		m_RendererData.StaticMeshPipeline = Ref< Pipeline >::Create( PipelineSpec );

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

		if( m_RendererData.DynamicMeshPipeline )
			m_RendererData.DynamicMeshPipeline = nullptr;

		PipelineSpec.Name = "Dynamic Meshes";
		PipelineSpec.Shader = m_RendererData.DynamicMeshShader;
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

		m_RendererData.DynamicMeshPipeline = Ref< Pipeline >::Create( PipelineSpec );
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
		PipelineSpec.Width = ( uint32_t ) SHADOW_MAP_SIZE;
		PipelineSpec.Height = ( uint32_t ) SHADOW_MAP_SIZE;
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
		Ref<Image2D> shadowImage = Ref<Image2D>::Create( ImageFormat::DEPTH32F, ( uint32_t ) SHADOW_MAP_SIZE, ( uint32_t ) SHADOW_MAP_SIZE, 4, 1 );
		shadowImage->SetDebugName( "Layered shadow image" );

		FramebufferSpecification FBSpec = {};
		FBSpec.Width = ( uint32_t ) SHADOW_MAP_SIZE;
		FBSpec.Height = ( uint32_t ) SHADOW_MAP_SIZE;
		FBSpec.ArrayLevels = SHADOW_CASCADE_COUNT;
		FBSpec.ExistingImages[0] = shadowImage;

		PassSpecification PassSpec = {};
		PassSpec.Name = "Dir Shadow Map";
		PassSpec.Attachments = { ImageFormat::Depth };

		for( size_t i = 0; i < SHADOW_CASCADE_COUNT; i++ )
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
		for( size_t i = 0; i < SHADOW_CASCADE_COUNT; i++ )
		{
			PipelineSpec.RenderPass = m_RendererData.DirShadowMapPasses[ i ];
			m_RendererData.DirShadowMapDynamicPipelines[ i ] = Ref< Pipeline >::Create( PipelineSpec );
		}
	}

	void SceneRenderer::InitPreDepth()
	{
		if( m_RendererData.PreDepthPass ) 
		{
			m_RendererData.PreDepthPass->Recreate();
		}
		else
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

		if( m_RendererData.PreDepthPipeline ) 
		{
			m_RendererData.PreDepthPipeline = nullptr;
		}

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

		for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_RendererData.PreDepthDynamicMaterialSet2->SetSB( 15u, m_RendererData.SBBoneTransforms->Get( 2u, 15u, ( uint32_t ) i ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// Light culling
		//////////////////////////////////////////////////////////////////////////
		if( m_RendererData.LightCullingPipeline )
			m_RendererData.LightCullingPipeline = nullptr;

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

			PassSpec.Attachments = { ImageFormat::RGBA8, ImageFormat::Depth };

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

			FBSpec.Attachments = { ImageFormat::RGBA8, ImageFormat::Depth };

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

//		BindSceneCompositeAOTexture();
		
		m_RendererData.SceneCompositeMaterial->SetResource( "u_DepthTexture", m_RendererData.GeometryFramebuffer->GetDepthAttachmentResource() );

		if( m_RendererData.SceneCompositePipeline )
			m_RendererData.SceneCompositePipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
		PipelineSpec.Name = "Scene Composite";
		PipelineSpec.Shader = m_RendererData.SceneCompositeShader;
		PipelineSpec.RenderPass = m_RendererData.SceneComposite;
		PipelineSpec.UseDepthTest = true;
		PipelineSpec.CullMode = CullMode::None;
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;

		m_RendererData.SceneCompositePipeline = Ref<Pipeline>::Create( PipelineSpec );
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

			NewSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];
			NewSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource();

			m_RendererData.LateCompositeFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height, NewSpec );
		}
		else	
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.LateCompositePass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;

			FBSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];
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

		if( m_RendererData.PhysicsOutlinePipeline )
			m_RendererData.PhysicsOutlinePipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

	void SceneRenderer::InitBloom()
	{
		if( !m_RendererData.BloomShader )
		{
			m_RendererData.BloomShader = ShaderLibrary::Get().FindOrLoad( "Bloom", "content/shaders/Bloom.glsl" );
		}

		m_RendererData.BloomComputePipeline = Ref<ComputePipeline>::Create( m_RendererData.BloomShader );

		CreateBloomMaterials();

		m_RendererData.BloomDirtTexture = Renderer::Get()->GetPinkTexture();
		m_RendererData.BloomDS = m_RendererData.BloomShader->CreateDescriptorSet( 0 );
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

		if( m_RendererData.TexturePassPipeline )
			m_RendererData.TexturePassPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

	void SceneRenderer::InitSSAO()
	{
		if( !m_RendererData.AOShader )
		{
			m_RendererData.AOShader = ShaderLibrary::Get().FindOrLoad( "SSAO", "content/shaders/SSAO.glsl" );
			m_RendererData.SSAOMaterial = Ref<Material>::Create( m_RendererData.AOShader, "SSAO" );
		}

		m_RendererData.SSAOMaterial->SetResource( "u_DepthTexture", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource() );
		m_RendererData.SSAOMaterial->SetResource( "u_ViewNormalTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 1 ] );

		if( !m_RendererData.SSAONoiseGenerated || !m_RendererData.SSAONoiseImage )
		{
			std::vector<glm::vec4> ssaoNoise( 4 * 4 );
			for( size_t i = 0; i < ssaoNoise.size(); i++ )
			{
				ssaoNoise[ i ] = glm::vec4(
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					0.0F, 0.0F );
			}

			m_RendererData.SSAONoiseGenerated = true;

			m_RendererData.SSAONoiseImage = Ref<Texture2D>::Create( ImageFormat::RGBA32F, 4, 4, ssaoNoise.data() );

			m_RendererData.SSAOMaterial->SetResource( "u_NoiseTexture", m_RendererData.SSAONoiseImage );

			// Sample Kernel
			std::vector<glm::vec4> ssaoKernel( 32 );
			for( size_t i = 0; i < ssaoKernel.size(); i++ )
			{
				glm::vec3 sample(
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					Random::RandomFloatInRange( 0.0F, 1.0F ) * 2.0F - 1.0F,
					Random::RandomFloatInRange( 0.0F, 1.0F ) );

				sample = glm::normalize( sample );
				sample *= Random::RandomFloatInRange( 0.0F, 1.0F );

				float scale = float( i ) / 32.0f;
				scale = std::lerp( 0.1f, 1.0f, scale * scale );

				ssaoKernel[ i ] = glm::vec4( sample * scale, 0.0f );
			}

			// u_Matrices
			struct USSAOData
			{
				glm::vec4 Samples[ 32 ];
				float R = 0.5f;
			} u_Data{};

			std::memcpy( u_Data.Samples, ssaoKernel.data(), ssaoKernel.size() * sizeof( glm::vec4 ) );

			// We cannot write to the UniformBufferSet as we don't use the same UB as the other shaders
			m_RendererData.SSAOMaterial->UploadDataToUB( 1, &u_Data, sizeof( u_Data ) );
		}

		if( m_RendererData.SSAORenderPass )
		{
			m_RendererData.SSAORenderPass->Recreate();
		}
		else
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "SSAO RP";
			PassSpec.Attachments = { ImageFormat::RED8 };

			m_RendererData.SSAORenderPass = Ref<Pass>::Create( PassSpec );
		}

		if( m_RendererData.SSAOFramebuffer )
		{
			m_RendererData.SSAOFramebuffer->Recreate( m_RendererData.Width / 2, m_RendererData.Height / 2 );
		}
		else
		{
			FramebufferSpecification FBSpec;
			FBSpec.RenderPass = m_RendererData.SSAORenderPass;
			FBSpec.Width = m_RendererData.Width / 2;
			FBSpec.Height = m_RendererData.Height / 2;
			FBSpec.Attachments = { ImageFormat::RED8 };
			FBSpec.CreateDepth = false;

			m_RendererData.SSAOFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		if( m_RendererData.SSAOPipeline )
			m_RendererData.SSAOPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width / 2;
		PipelineSpec.Height = m_RendererData.Height / 2;
		PipelineSpec.Name = "SSAO";
		PipelineSpec.Shader = m_RendererData.AOShader;
		PipelineSpec.RenderPass = m_RendererData.SSAORenderPass;
		PipelineSpec.UseDepthTest = true;
		PipelineSpec.CullMode = CullMode::None;
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		};

		m_RendererData.SSAOPipeline = Ref<Pipeline>::Create( PipelineSpec );
	}

	void SceneRenderer::InitHBAO()
	{
	}

	void SceneRenderer::InitSelectionPass()
	{
		if( m_RendererData.SelectedGeometryPass )
			m_RendererData.SelectedGeometryPass->Recreate();
		else
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

		if( m_RendererData.SelectedGeometryPipeline )
			m_RendererData.SelectedGeometryPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

		if( !m_RendererData.SelectedGeometryDynamicPipeline )
			m_RendererData.SelectedGeometryDynamicPipeline = nullptr;

		PipelineSpec.Name = "Selected Geometry (Dynamic)";
		PipelineSpec.Shader = m_RendererData.SelectionDynamicShader;
		PipelineSpec.AdditionalLayoutAtEnd = {
			{ ShaderDataType::Int4,   "a_BoneIndices" },
			{ ShaderDataType::Float4, "a_BoneWeights" }
		};

		m_RendererData.SelectedGeometryDynamicPipeline = Ref<Pipeline>::Create( PipelineSpec );
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
		if( m_RendererData.JumpFloodFirstPass )
			m_RendererData.JumpFloodFirstPass->Recreate();
		else
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

		if( m_RendererData.JumpFloodFirstPipeline )
			m_RendererData.JumpFloodFirstPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

		m_RendererData.JumpFloodFirstMaterial->SetResource( "u_InputTexture", m_RendererData.SelectedGeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );
	}

	void SceneRenderer::InitJmpfEvenPass()
	{
		if( m_RendererData.JumpFloodEvenPass )
			m_RendererData.JumpFloodEvenPass->Recreate();
		else
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

		if( m_RendererData.JumpFloodEvenPipeline )
			m_RendererData.JumpFloodEvenPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

		m_RendererData.JumpFloodEvenMaterial->SetResource( "u_InputTexture", m_RendererData.JumpFloodFirstPassFB->GetColorAttachmentsResources()[ 0 ] );
	}

	void SceneRenderer::InitJmpfOddPass()
	{
		if( m_RendererData.JumpFloodOddPass )
			m_RendererData.JumpFloodOddPass->Recreate();
		else
		{
			// Create the scene composite render pass.
			PassSpecification PassSpec = {};
			PassSpec.Name = "Jump Flood Odd pass";
			PassSpec.LoadColor = true;
			PassSpec.Attachments = { ImageFormat::RGBA32F };

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

		if( m_RendererData.JumpFloodOddPipeline )
			m_RendererData.JumpFloodOddPipeline = nullptr;

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

		m_RendererData.JumpFloodOddMaterial->SetResource( "u_InputTexture", m_RendererData.JumpFloodEvenFB->GetColorAttachmentsResources()[ 0 ] );
	}

	void SceneRenderer::RenderGrid()
	{
#if !defined(SAT_DIST)
		SAT_PF_EVENT();

		// Should we show the grid?
		const auto& rVisOptions = m_pScene->GetVisualisationOptions();
		const bool isRuntimeActive = m_pScene->IsRuntimeActive();
		if( !( isRuntimeActive ? rVisOptions.ShowGridOnRuntime : rVisOptions.ShowGrid ) )
		{
			return;
		}
#else
		return;
#endif

		// Set UB Data.
		const glm::mat4 trans = glm::rotate( glm::mat4( 1.0f ), glm::radians( 90.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( 16.0f ) );

		UBGridMatrices GridMatricesObject = {};
		GridMatricesObject.Transform = trans;
		GridMatricesObject.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		GridMatricesObject.Res = 0.025f;
		GridMatricesObject.Scale = 16.025f;

		m_RendererData.GridMaterial->UploadDataToUB( 0, &GridMatricesObject, sizeof( GridMatricesObject ) );

		Renderer::Get()->SubmitFullscreenQuad(
			m_RendererData.CommandBuffer,
			m_RendererData.GridPipeline, 
			m_RendererData.GridMaterial, 
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

		// We have no skybox.
		if( rSceneEnvironment->Azimuth == 0 && rSceneEnvironment->Inclination == 0 && rSceneEnvironment->Turbidity == 0 )
		{
			// I don't really like this.
			// TODO: Come back to this.
			if( rSceneEnvironment->IrradianceMap && rSceneEnvironment->RadianceMap )
			{
				rSceneEnvironment->RadianceMap = nullptr;
				rSceneEnvironment->IrradianceMap = nullptr;
			}

			return;
		}

		// Skybox values where set, but check if out textures exist or update them accordingly.
		CheckInvalidSkybox();

		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;
		m_RendererData.SkyboxMaterial->SetResource( "u_CubeTexture", m_RendererData.SceneEnvironment->IrradianceMap );

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

	void SceneRenderer::CheckInvalidSkybox()
	{
		if( !m_pScene )
			return;

		// Invalid skybox, maybe null from loading a new scene? This only happens on the first frames so this is a hack.
		if( m_RendererData.SceneEnvironment->IrradianceMap == nullptr && m_RendererData.SceneEnvironment->RadianceMap == nullptr )
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

				if( Skylight.DynamicSky && !m_RendererData.SceneEnvironment->IrradianceMap && !m_RendererData.SceneEnvironment->RadianceMap )
				{
					m_RendererData.SceneEnvironment->Turbidity = Skylight.Turbidity;
					m_RendererData.SceneEnvironment->Azimuth = Skylight.Azimuth;
					m_RendererData.SceneEnvironment->Inclination = Skylight.Inclination;

					// We can call this directly, we should be on the render thread.
					Ref<TextureCube> map = CreateDymanicSky();

					m_RendererData.SceneEnvironment->IrradianceMap = map;
					m_RendererData.SceneEnvironment->RadianceMap = map;
				}
			}
		}
	}

	void SceneRenderer::UpdateCascades( const glm::vec3& Direction )
	{
		const auto& viewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		float cascadeSplits[ SHADOW_CASCADE_COUNT ];

		constexpr float NEAR_CLIP = 0.1F;
		constexpr float FAR_CLIP = 1000.0F;
		constexpr float CLIP_RANGE = FAR_CLIP - NEAR_CLIP;

		constexpr float minZ = NEAR_CLIP;
		constexpr float maxZ = NEAR_CLIP + CLIP_RANGE;

		constexpr float RANGE = maxZ - minZ;
		constexpr float RATIO = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for( uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++ )
		{
			const float p = ( i + 1 ) / static_cast< float >( SHADOW_CASCADE_COUNT );
			const float log = minZ * std::pow( RATIO, p );
			const float uniform = minZ + RANGE * p;
			const float d = m_RendererData.CascadeSplitLambda * ( log - uniform ) + uniform;
			cascadeSplits[ i ] = ( d - NEAR_CLIP ) / CLIP_RANGE;
		}

		cascadeSplits[ 3 ] = 0.3f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for( uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++ )
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
			float ShadowMapResolution = SHADOW_MAP_SIZE;
			glm::vec4 shadowOrigin = ( shadowMatrix * glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) * ShadowMapResolution * 0.5f;
			glm::vec4 roundedOrigin = glm::round( shadowOrigin );
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[ 3 ] += roundOffset;

			// Store split distance and matrix in cascade
			m_RendererData.ShadowCascades[ i ].SplitDepth = ( NEAR_CLIP + splitDist * CLIP_RANGE ) * -1.0f;
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
		
			m_RendererData.GridMaterial = Ref<Material>::Create( m_RendererData.GridShader, "Grid" );
		}

		if( m_RendererData.GridPipeline )
			m_RendererData.GridPipeline = nullptr;

		// Grid pipeline spec.
		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

	//////////////////////////////////////////////////////////////////////////
	// Skybox
	//////////////////////////////////////////////////////////////////////////

	void SceneRenderer::CreateSkyboxComponents()
	{
		// Create skybox shader.
		if( !m_RendererData.SkyboxShader && !m_RendererData.PreethamShader )
		{
			m_RendererData.SkyboxShader = ShaderLibrary::Get().FindOrLoad( "Skybox", "content/shaders/Skybox.glsl" );
			m_RendererData.PreethamShader = ShaderLibrary::Get().FindOrLoad( "Skybox_Compute", "content/shaders/Skybox_Compute.glsl" );

			m_RendererData.SkyboxMaterial = Ref<Material>::Create( m_RendererData.SkyboxShader, "SkyboxComposite" );
			m_RendererData.PreethamMaterial = Ref<Material>::Create( m_RendererData.PreethamShader, "PreethamMat" );
		}

		if( m_RendererData.SkyboxPipeline )
			m_RendererData.SkyboxPipeline = nullptr;

		// Create pipeline.
		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
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

	void SceneRenderer::ImGuiRender()
	{
#if !defined(SAT_DIST)
		SAT_PF_EVENT();

		ImGui::Text( "Viewport size, %i, %i", ( int ) m_RendererData.Width, ( int ) m_RendererData.Height );

		ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );

		if( Auxiliary::TreeNode( "Stats", true ) )
		{
			const auto FrameTimings = Renderer::Get()->GetFrameTimings();

			float shadowPassTime = 0.0f;

			if( m_RendererData.EnableShadows )
			{
				for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
				{
					shadowPassTime += m_RendererData.ShadowMapTimers[ i ].ElapsedMilliseconds();
				}
			}

			ImGui::Text( "Renderer::BeginFrame: %.2f ms", FrameTimings.first );

			ImGui::Text( "SceneRenderer::PreDepthPass: %.2f ms", m_RendererData.PreDepthTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::ShadowMapPass: %.2f ms", shadowPassTime );

			ImGui::Text( "SceneRenderer::LightCulling: %.4f ms", m_RendererData.LightCullingTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::GeometryPass: %.2f ms", m_RendererData.GeometryPassTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::BloomPass: %.3f ms", m_RendererData.BloomTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::SceneComposite: %.2f ms", m_RendererData.SceneCompPPTimer.ElapsedMilliseconds() );

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
			if( Auxiliary::TreeNode( "Shadow settings", true ) )
			{
				ImGui::DragFloat( "Cascade Split Lambda", &m_RendererData.CascadeSplitLambda, 1.0f, 0.01f, 1.0f );
				ImGui::DragFloat( "Cascade Near plane", &m_RendererData.CascadeNearPlaneOffset, 1.0f, -1000.0f, 1000.0f );
				ImGui::DragFloat( "Cascade Far plane", &m_RendererData.CascadeFarPlaneOffset, 1.0f, -1000.0f, 1000.0f );

				ImGui::Checkbox( "Enable shadows", &m_RendererData.EnableShadows );

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

				ImGui::SliderInt( "##bloom_tex", &index, 0, 2 );
				ImGui::SliderInt( "##mip", &MipIndex, 0, img->GetMipMapLevels() - 2 );

				const float size = ImGui::GetContentRegionAvail().x;
				Auxiliary::Image( img, MipIndex, { size, size }, { 0, 1 }, { 1, 0 } );

				ImGui::SliderFloat( "##dirtint", &m_RendererData.BloomDirtIntensity, 0, 1000.0f );

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

		if( m_RendererData.SceneEnvironment )
		{
			m_RendererData.SceneEnvironment->Turbidity = 0.0f;
			m_RendererData.SceneEnvironment->Azimuth = 0.0f;
			m_RendererData.SceneEnvironment->Inclination = 0.0f;
		}

		// Find the skylight entity and set the turbidity, azimuth, inclination.
		auto view = m_pScene->GetAllEntitiesWith<SkylightComponent>();

		for( auto& entity : view )
		{
			auto& skylight = entity->GetComponent<SkylightComponent>();

			if( !m_RendererData.SceneEnvironment )
				m_RendererData.SceneEnvironment = Ref<EnvironmentMap>::Create();

			m_RendererData.SceneEnvironment->Turbidity = skylight.Turbidity;
			m_RendererData.SceneEnvironment->Azimuth = skylight.Azimuth;
			m_RendererData.SceneEnvironment->Inclination = skylight.Inclination;
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

		for( int i = 0; i < 8; i++ )
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
				auto& command = m_DrawList[ key ];
				command.Mesh = mesh;
				command.SubmeshIndex = ( uint32_t ) i;
				instanceOffset = command.Instances;
				++command.Instances;
				command.InstanceOffset = instanceOffset;

				auto& shadow = m_ShadowMapDrawList[ key ];
				shadow.Mesh = mesh;
				shadow.SubmeshIndex = ( uint32_t ) i;
				++shadow.Instances;
				shadow.InstanceOffset = instanceOffset;

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
		for( size_t i = 0; i < submeshes.size(); i++ )
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
		for( size_t i = 0; i < submeshes.size(); i++ )
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

		switch( m_AOTechnique )
		{
			case AOTechnique::SSAO:
				InitSSAO();
				break;

			case AOTechnique::HBAO:
				InitHBAO();
				break;

			case AOTechnique::None:
			default:
				break;
		}

		CreateBloomMaterials();

		constexpr uint32_t TILE_SIZE = 16;
		glm::uvec2 Viewport = { m_RendererData.Width, m_RendererData.Height };
		glm::uvec2 Size = Viewport;
		Size += TILE_SIZE - Viewport % TILE_SIZE;

		m_RendererData.LightCullingWorkGroups = { Size / TILE_SIZE, 1 };
		const float size = m_RendererData.LightCullingWorkGroups.x * m_RendererData.LightCullingWorkGroups.y * 4.0f * 1024.0f;
		m_RendererData.StorageBufferSet->Resize( 0, 14, ( size_t ) size );

		InitSceneComposite();
		InitLateComposite();
		InitPhysicsOutline();
		InitSelectionPass();
		InitJumpFlood();

		InitTexturePass();

		CreateSkyboxComponents();
		CreateGridComponents();

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

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Grid" );

		RenderGrid();

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Static meshes" );
		
		// Set environment resource.
		Renderer::Get()->SetSceneEnvironment( m_RendererData.ShadowCascades[ 0 ].Framebuffer->GetDepthAttachmentResource(), m_RendererData.SceneEnvironment, m_RendererData.BRDFLUT_Texture );

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

		// This uses 24,592 bytes of stack... oh well...
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

		const auto dirLight = m_pScene->m_Lights.DirectionalLights[ 0 ];
		const auto invView = glm::inverse( u_Matrices.View );

		u_SceneData.CameraPosition = invView[ 3 ];
		u_SceneData.Lights = { .Direction = dirLight.Direction, .Radiance = dirLight.Radiance, .Multiplier = dirLight.Intensity };

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
				rTransformData.Offset );
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
		const VkExtent2D Extent = { ( uint32_t ) SHADOW_MAP_SIZE, ( uint32_t ) SHADOW_MAP_SIZE };
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
		Viewport.width = SHADOW_MAP_SIZE;
		Viewport.height = SHADOW_MAP_SIZE;
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

		for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
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

	void SceneRenderer::TexturePass()
	{
		SAT_PF_EVENT();

		Ref<Pass> pass = VulkanContext::Get()->GetDefaultPass();
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		const VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// Begin scene composite pass.
		pass->BeginPass( CommandBuffer, VulkanContext::Get()->GetSwapchain().GetFramebuffers()[ frame ], Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = ( float ) m_RendererData.Height;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = -( float ) m_RendererData.Height;
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
			constexpr uint32_t TILE_SIZE = 16;
			glm::uvec2 Viewport = { m_RendererData.Width, m_RendererData.Height };
			glm::uvec2 Size = Viewport;
			Size += TILE_SIZE - Viewport % TILE_SIZE;

			m_RendererData.LightCullingWorkGroups = { Size / TILE_SIZE, 1 };

			const float size = m_RendererData.LightCullingWorkGroups.x * m_RendererData.LightCullingWorkGroups.y * 4.0f * 1024.0f;
			m_RendererData.StorageBufferSet->Resize( 0, 14, ( size_t ) size );
		}

		// UBs
		// Heres the big fucker again, 24,592 bytes of stack.
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

		m_RendererData.LightCullingMaterial->UploadDataToUB( 0, &u_Lights, 16ull + sizeof PointLight * u_Lights.nbLights );
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

	// TODO: This function needs a rework.
	//       We are creating a new descriptor set every frame but we are recycling it at the end of frame in the Renderer.
	//	     We could just free them in this function?
	//       And also we aren't freeing them after the stages are complete.
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
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width / 2, m_RendererData.Height / 2 };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.SSAORenderPass->BeginPass( CommandBuffer, m_RendererData.SSAOFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_RendererData.Width / 2;
		Viewport.height = ( float ) m_RendererData.Height / 2;
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
		u_Matrices.InvProjection = glm::inverse( u_Matrices.Projection );

		m_RendererData.SSAOMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		Renderer::Get()->SubmitFullscreenQuad(
			CommandBuffer, 
			m_RendererData.SSAOPipeline,
			m_RendererData.SSAOMaterial,
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		m_RendererData.SSAORenderPass->EndPass();
	}

	void SceneRenderer::SelectedGeometryPass()
	{
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

	void SceneRenderer::CreateBloomMaterials()
	{
		const glm::uvec2 viewportSize = { m_RendererData.Width, m_RendererData.Height };

		glm::uvec2 bs = ( viewportSize + 1u ) / 2u;
		bs += m_RendererData.BloomWorkSize - bs % m_RendererData.BloomWorkSize;

		m_RendererData.BloomTextures.fill( {} );

		// Create bloom textures
		for( uint32_t i = 0; i < 3; i++ )
		{
			auto& rTextureInfo = m_RendererData.BloomTextures[ i ];

			rTextureInfo.Texture = Ref<Texture2D>::Create( ImageFormat::RGBA32F, bs.x, bs.y, nullptr, true );
			rTextureInfo.Texture->SetDebugName( "Bloom Texture/" + std::to_string( t ) );

			for( size_t i = 0; i < rTextureInfo.Texture->GetMipMapLevels(); i++ )
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
			m_RendererData.BloomDownsampleAMaterials[ i ] = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom Downsample A" );

			m_RendererData.BloomDownsampleAMaterials[ i ]->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 1 ].Texture, m_RendererData.BloomTextures[ 1 ].ImageInfos[ i ] );
			m_RendererData.BloomDownsampleAMaterials[ i ]->SetResource( "u_InputTexture", m_RendererData.BloomTextures[ 0 ].Texture );
			m_RendererData.BloomDownsampleAMaterials[ i ]->SetResource( "u_BloomTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );

			m_RendererData.BloomDownsampleBMaterials[ i ] = Ref<Material>::Create( m_RendererData.BloomShader, "Bloom Downsample B" );

			m_RendererData.BloomDownsampleBMaterials[ i ]->SetResourceWithVulkanInfo( "o_Image", m_RendererData.BloomTextures[ 0 ].Texture, m_RendererData.BloomTextures[ 0 ].ImageInfos[ i ] );
			m_RendererData.BloomDownsampleBMaterials[ i ]->SetResource( "u_InputTexture", m_RendererData.BloomTextures[ 1 ].Texture );
			m_RendererData.BloomDownsampleBMaterials[ i ]->SetResource( "u_BloomTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ] );
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
		// TODO: TEMP u_BloomDirtTexture -> u_AOTexture
		switch( m_AOTechnique )
		{
			default:
			case AOTechnique::None:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomDirtTexture", Renderer::Get()->GetPinkTexture() );
				break;

			case AOTechnique::SSAO:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomDirtTexture", m_RendererData.AOBlurFramebuffer->GetColorAttachmentsResources()[ 0 ] );
				break;

			case AOTechnique::HBAO:
				m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomDirtTexture", Renderer::Get()->GetPinkTexture() );
				break;
		}
	}

	void SceneRenderer::SendBoneDataToMap( Ref<SkeletalMesh> mesh, const StaticMeshKey& rKey, const std::vector<glm::mat4>& rBoneTransforms )
	{
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

		// Every submesh is to have 100 bone transforms.
		// However, not every mesh actually has 100 bones for example, the mesh that this code was debugged with has 64 bones
		// So, thats why we have to do a workaround to ensure that the bones transforms is correct because if we don't set them (and use the Meshes' bone count) we will end up reading garbage data.
		// This could be solved if we simply was able to fill the whole buffer with glm::mat4{0.0f} 

		auto& rBoneTransformMap = m_RendererData.BoneTransformMap[ rKey ];

		const size_t boneCount = rBoneTransformMap.Data.size();
		const size_t stride = 100 + boneCount;

		rBoneTransformMap.Stride = ( uint32_t ) stride;

		if( rBoneTransformMap.Data.empty() )
		{
			rBoneTransformMap.Data.resize( 100 );
		}

		auto skeleton = mesh->GetSkeletonAsset();
		for( size_t s = 0; s < skeleton->GetBoneInfo().size(); ++s )
		{
			rBoneTransformMap.Data[ s ] = skeleton->GetTransform() * rBoneTransforms[ skeleton->GetBoneInfo().at( s ).BoneIndex ] * skeleton->GetBoneInfo().at( s ).InverseBindPose;
		}
	}

	Ref<TextureCube> SceneRenderer::CreateDymanicSky()
	{
		constexpr uint32_t cubemapSize = 512;
		constexpr uint32_t irradianceMap = 32;

		Ref<TextureCube> Environment = Ref<TextureCube>::Create( ImageFormat::RGBA32F, cubemapSize, cubemapSize );

		Ref<Shader> skyShader = ShaderLibrary::Get().Find( "Skybox_Compute" );
		Ref<ComputePipeline> pipeline = Ref<ComputePipeline>::Create( skyShader );

		const glm::vec3 params = { m_RendererData.SceneEnvironment->Turbidity, m_RendererData.SceneEnvironment->Azimuth, m_RendererData.SceneEnvironment->Inclination };

		m_RendererData.PreethamMaterial->SetResource( "o_CubeMap", Environment );
		m_RendererData.PreethamMaterial->SetPC( "pc_Params.Params", params );

		pipeline->Bind();
		pipeline->Execute( m_RendererData.PreethamMaterial, cubemapSize / irradianceMap, cubemapSize / irradianceMap, 6 );
		pipeline->Unbind();

		Environment->CreateMips();

		pipeline = nullptr;

		return Environment;
	}

	void SceneRenderer::SetDynamicSky( float Turbidity, float Azimuth, float Inclination )
	{
		AddScheduledFunction([&, turbidity = Turbidity, azimuth = Azimuth, inclination = Inclination]()
			{
				m_RendererData.SceneEnvironment->Turbidity = turbidity;
				m_RendererData.SceneEnvironment->Azimuth = azimuth;
				m_RendererData.SceneEnvironment->Inclination = inclination;

				m_RendererData.SceneEnvironment->IrradianceMap = nullptr;
				m_RendererData.SceneEnvironment->RadianceMap = nullptr;

				Ref<TextureCube> map = CreateDymanicSky();

				m_RendererData.SceneEnvironment->IrradianceMap = map;
				m_RendererData.SceneEnvironment->RadianceMap = map;

			} );
	}

	bool SceneRenderer::HasFlag( SceneRendererFlags flag ) const
	{
		return ( m_Flags & flag ) != 0;
	}

	void SceneRenderer::ChangeAOTechnique( AOTechnique newTechique )
	{
		// TODO: ChangeAOTechnique
	}

	void SceneRenderer::Screenshot( const std::filesystem::path& rPath, const glm::vec2& rSize )
	{
	//	m_RendererData.SceneCompositeFramebuffer->Capture( rPath, 0, rSize );
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
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Geometry" );
			GeometryPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Bloom" );
			BloomPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "SelectedGeometry" );
			SelectedGeometryPass();
		}

		/*
		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "SSAO" );
			SSAOPass();
		}
		*/

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "JumpFlood" );
			JumpFloodPass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Scene Composite/Post Processing" );
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


		// Vertex and Index buffers
		QuadVertexBuffer->Destroy();
		QuadIndexBuffer->Destroy();

		// Framebuffers
		GeometryFramebuffer       = nullptr;
		SceneCompositeFramebuffer = nullptr;
		PreDepthFramebuffer       = nullptr;
		LateCompositeFramebuffer  = nullptr;

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
		AOCompositeShader       = nullptr;
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

		SceneEnvironment        = nullptr;

		for( auto& buffer : SubmeshTransformData )
			delete[] buffer.pData;

		delete[] BoneTransformData;

		// Storage buffer set
		StorageBufferSet = nullptr;

		SubmeshTransformData.clear();

		MeshTransforms.clear();
	}

}
