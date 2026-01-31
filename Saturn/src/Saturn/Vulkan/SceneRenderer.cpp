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
			m_RendererData.Width = Application::Get().GetWindow()->GetWidth();
			m_RendererData.Height = Application::Get().GetWindow()->GetHeight();
		}

		//////////////////////////////////////////////////////////////////////////
		// Geometry 
		//////////////////////////////////////////////////////////////////////////

		if( !Application::Get().HasFlag( ApplicationFlag_CreateSceneRenderer ) )
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

		InitTexturePass();

//		InitSelection();

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
		Renderer::Get().AddShaderReloadCB( SAT_BIND_EVENT_FN( OnShaderReloaded ) );
#endif
	}

	Ref<Image2D> SceneRenderer::CompositeImage()
	{
		return m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];
	}

	void SceneRenderer::Terminate()
	{
#if !defined(SAT_DIST)
		Renderer::Get().ClearShaderReferences();
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

		if( !m_RendererData.DirShadowMapShader )
		{
			m_RendererData.DirShadowMapShader = ShaderLibrary::Get().FindOrLoad( "ShadowMap", "content/shaders/ShadowMap.glsl" );

			m_RendererData.DirShadowMapMaterial = Ref<Material>::Create( m_RendererData.DirShadowMapShader, "ShdMap" );
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

		m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomTexture", Renderer::Get().GetPinkTexture());
		m_RendererData.SceneCompositeMaterial->SetResource( "u_BloomDirtTexture", Renderer::Get().GetPinkTexture() );
		
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
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		};

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

		const glm::uvec2 viewportSize = { m_RendererData.Width, m_RendererData.Height };

		glm::uvec2 bs = ( viewportSize + 1u ) / 2u;
		bs += m_RendererData.BloomWorkSize - bs % m_RendererData.BloomWorkSize;

		for( uint32_t i = 0; i < 3; i++ )
		{
			m_RendererData.BloomTextures[ i ] = Ref<Texture2D>::Create( ImageFormat::RGBA32F, bs.x, bs.y, nullptr, true );
			m_RendererData.BloomTextures[ i ]->SetDebugName( "Bloom Texture: " + std::to_string( i ) );
		}

		m_RendererData.BloomDirtTexture = Renderer::Get().GetPinkTexture();

		m_RendererData.BloomDS = m_RendererData.BloomShader->CreateDescriptorSet( 0 );

//		m_RendererData.BloomDirtTexture = Ref<Texture2D>::Create( "content/textures/editor/BloomDirtTextureUE.png", AddressingMode::Repeat );
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
		PipelineSpec.RenderPass = VulkanContext::Get().GetDefaultPass();
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
	}

	void SceneRenderer::InitHBAO()
	{
	}

	/*
	void SceneRenderer::InitSelection()
	{
		if( !m_RendererData.SelectionShader )
		{
			m_RendererData.SelectionShader = ShaderLibrary::Get().FindOrLoad( "Selection", "content/shaders/Selection.glsl" );
		}

		m_RendererData.SelectionMaterial = Ref<Material>::Create( m_RendererData.SelectionShader, "PhysicsOutline" );

		if( m_RendererData.SelectionRenderPass )
			m_RendererData.SelectionRenderPass->Recreate();
		else
		{
			PassSpecification PassSpec = {};
			PassSpec.Name = "Selection pass";
			PassSpec.LoadDepth = true;

			// Depth = PreDepth.
			PassSpec.Attachments = { ImageFormat::RGBA8, ImageFormat::DEPTH24STENCIL8 };

			m_RendererData.SelectionRenderPass = Ref< Pass >::Create( PassSpec );
		}

		if( m_RendererData.SelectionFramebuffer )
		{
			FramebufferSpecification NewSpec;
			NewSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource();

			m_RendererData.SelectionFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height, NewSpec );
		}
		else
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.SelectionRenderPass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;

			FBSpec.Attachments = { ImageFormat::RGBA8 };
			FBSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentResource();

			m_RendererData.SelectionFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		PipelineSpecification PipelineSpec = {};
		PipelineSpec.Width = m_RendererData.Width;
		PipelineSpec.Height = m_RendererData.Height;
		PipelineSpec.Name = "SelectionPipeline";
		PipelineSpec.Shader = m_RendererData.SelectionShader;
		PipelineSpec.RenderPass = m_RendererData.SelectionRenderPass;
		PipelineSpec.UseDepthTest = true;
		PipelineSpec.CullMode = CullMode::None;
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tanget" },
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};
		PipelineSpec.InstanceLayout = {
			{ ShaderDataType::Float4, "a_TransformBufferR1" },
			{ ShaderDataType::Float4, "a_TransformBufferR2" },
			{ ShaderDataType::Float4, "a_TransformBufferR3" },
			{ ShaderDataType::Float4, "a_TransformBufferR4" }
		};

		m_RendererData.SelectionPipeline = Ref<Pipeline>::Create( PipelineSpec );
	}
	*/
	void SceneRenderer::RenderGrid()
	{
		SAT_PF_EVENT();

		if( !HasFlag( SceneRendererFlag_RenderGrid ) )
			return;

		// Set UB Data.
		glm::mat4 trans = glm::rotate( glm::mat4( 1.0f ), glm::radians( 90.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( 16.0f ) );

		UBGridMatrices GridMatricesObject = {};
		GridMatricesObject.Transform = trans;
		GridMatricesObject.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		GridMatricesObject.Res = 0.025f;
		GridMatricesObject.Scale = 16.025f;

		m_RendererData.GridMaterial->UploadDataToUB( 0, &GridMatricesObject, sizeof( GridMatricesObject ) );

		Renderer::Get().SubmitFullscreenQuad(
			m_RendererData.CommandBuffer, m_RendererData.GridPipeline, m_RendererData.GridMaterial, m_RendererData.UniformBufferSet, m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );
	}

	void SceneRenderer::RenderSkybox()
	{
		SAT_PF_EVENT();

		// Is this really needed?
		if( !m_pScene )
			return;

		auto& sceneEnvironment = m_RendererData.SceneEnvironment;

		// We have no skybox.
		if( sceneEnvironment->Azimuth == 0 && sceneEnvironment->Inclination == 0 && sceneEnvironment->Turbidity == 0 )
		{
			// I don't really like this.
			// TODO: Come back to this.
			if( sceneEnvironment->IrradianceMap && sceneEnvironment->RadianceMap )
			{
				sceneEnvironment->RadianceMap = nullptr;
				sceneEnvironment->IrradianceMap = nullptr;
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

		Renderer::Get().SubmitFullscreenQuad( CommandBuffer, 
			m_RendererData.SkyboxPipeline, 
			m_RendererData.SkyboxMaterial,
			m_RendererData.UniformBufferSet,
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
			glm::vec4 shadowOrigin = ( shadowMatrix * glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f ) ) * ShadowMapResolution / 2.0f;
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
		auto pAllocator = VulkanContext::Get().GetVulkanAllocator();

		// Create fullscreen quad.
		auto [vertex, index] = Renderer::Get().CreateFullscreenQuad();
		
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
		auto pAllocator = VulkanContext::Get().GetVulkanAllocator();

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
		SAT_PF_EVENT();

		ImGui::Text( "Viewport size, %i, %i", ( int ) m_RendererData.Width, ( int ) m_RendererData.Height );

		ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );

		if( Auxiliary::TreeNode( "Stats", true ) )
		{
			const auto FrameTimings = Renderer::Get().GetFrameTimings();

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

			ImGui::Text( "Renderer::EndFrame - Queue Present: %.2f ms", Renderer::Get().GetQueuePresentTime() );
			ImGui::Text( "Renderer::EndFrame - Queue Wait: %.2f ms", Renderer::Get().GetQueueWaitTime() );

			ImGui::Text( "Renderer::EndFrame - Total: %.2f ms", FrameTimings.second );

			ImGui::Text( "Total (RenderThread::Execute): %.2f ms", RenderThread::Get().GetWaitTime() );
			ImGui::Text( "Total : %.2f ms", Application::Get().Time().Milliseconds() );

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
				auto& img = m_RendererData.BloomTextures[ index ];

				ImGui::SliderInt( "##bloom_tex", &index, 0, 2 );
				ImGui::SliderInt( "##mip", &MipIndex, 0, img->GetMipMapLevels() - 2 );

				const float size = ImGui::GetContentRegionAvail().x;
				Auxiliary::Image( img, MipIndex, { size, size }, { 0, 1 }, { 1, 0 } );

				ImGui::SliderFloat( "##dirtint", &m_RendererData.BloomDirtIntensity, 0, 1000.0f );

				Auxiliary::EndTreeNode();
			}

			Auxiliary::EndTreeNode();
		}
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

		auto& id = mesh->ID;

		uint32_t instanceOffset = 0;

		auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); ++i )
		{
			glm::mat4 submeshTransform = transform * submeshes[ i ].Transform;

			AABB submeshAABB = submeshes[ i ].BoundingBox;
			AABB transformedAABB = TransformAABB( submeshAABB, submeshTransform );

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

		auto& id = mesh->ID;

		auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); i++ )
		{
			const glm::mat4 submeshTransform = transform * submeshes[ i ].Transform;

			const AABB submeshAABB = submeshes[ i ].BoundingBox;
			const AABB transformedAABB = TransformAABB( submeshAABB, submeshTransform );

			if( /*m_RendererData.CurrentCamera.pCamera->CameraFrustumIntersectsAABB( transformedAABB )*/ true )
			{
				StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i };

				// Submit for rendering
				auto& command = m_DynamicDrawList[ key ];
				command.Mesh = mesh;
				command.SubmeshIndex = ( uint32_t ) i;
				++command.Instances;

				SendBoneDataToMap( mesh, key, boneTransforms );

				/*
				size_t index = 0;
				for( const auto& transform : boneTransforms )
				{
					m_RendererData.BoneTransformMap[ key ].Data[ index ] = transform;
					index++;
				}
				*/

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

		auto& id = mesh->ID;
		auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); i++ )
		{
			StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i };

			auto& command = m_PhysicsColliderDrawList[ key ];
			command.Mesh = mesh;
			command.SubmeshIndex = ( uint32_t ) i;
			++command.Instances;
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

		InitSceneComposite();
		InitLateComposite();
		InitPhysicsOutline();
		
		InitTexturePass();

		const glm::uvec2 viewportSize = { m_RendererData.Width, m_RendererData.Height };

		glm::uvec2 bs = ( viewportSize + 1u ) / 2u;
		bs += m_RendererData.BloomWorkSize - bs % m_RendererData.BloomWorkSize;

		for( uint32_t i = 0; i < 3; i++ )
		{
			m_RendererData.BloomTextures[ i ] = Ref<Texture2D>::Create( ImageFormat::RGBA32F, bs.x, bs.y, nullptr, true );
			m_RendererData.BloomTextures[ i ]->SetDebugName( "Bloom Texture: " + std::to_string( i ) );
		}

		constexpr uint32_t TILE_SIZE = 16;
		glm::uvec2 Viewport = { m_RendererData.Width, m_RendererData.Height };
		glm::uvec2 Size = Viewport;
		Size += TILE_SIZE - Viewport % TILE_SIZE;

		m_RendererData.LightCullingWorkGroups = { Size / TILE_SIZE, 1 };

		float size = m_RendererData.LightCullingWorkGroups.x * m_RendererData.LightCullingWorkGroups.y * 4.0f * 1024.0f;
		m_RendererData.StorageBufferSet->Resize( 0, 14, ( size_t ) size );

//		m_RendererData.SceneCompositeShader->WriteDescriptor( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ]->GetDescriptorInfo(), m_RendererData.SC_DescriptorSet->GetVulkanSet() );

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
		Renderer::Get().SetSceneEnvironment( m_RendererData.ShadowCascades[ 0 ].Framebuffer->GetDepthAttachmentResource(), m_RendererData.SceneEnvironment, m_RendererData.BRDFLUT_Texture );

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
		const uint32_t frame = Renderer::Get().GetCurrentFrame();

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
			Renderer::Get().SubmitMesh(
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
		const uint32_t frame = Renderer::Get().GetCurrentFrame();
		m_RendererData.DynamicMeshMaterial->Update( {} );

		uint32_t index = 0;
		for( auto&& [key, Cmd] : m_DynamicDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			// Render Submesh
			Renderer::Get().SubmitDynamicMesh(
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

		const auto pAllocator = VulkanContext::Get().GetVulkanAllocator();
		const VkExtent2D Extent = { ( uint32_t ) SHADOW_MAP_SIZE, ( uint32_t ) SHADOW_MAP_SIZE };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;
		const uint32_t frame = Renderer::Get().GetCurrentFrame();

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

			for( auto&& [key, Cmd] : m_ShadowMapDrawList )
			{
				// Pass in the cascade index.
				Buffer AdditionalData( sizeof( uint32_t ), &i );

				const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

				Renderer::Get().RenderMeshWithoutMaterial( 
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

			vkCmdEndRenderPass( CommandBuffer );
			CmdEndDebugLabel( CommandBuffer );

			m_RendererData.ShadowMapTimers[ i ].Stop();
		}
	}

	void SceneRenderer::PreDepthPass()
	{
		SAT_PF_EVENT();

		m_RendererData.PreDepthTimer.Reset();

		const uint32_t frame = Renderer::Get().GetCurrentFrame();

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
			Renderer::Get().RenderMeshWithoutMaterial(
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
			Renderer::Get().RenderDynamicMeshWithoutMaterial(
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
		Renderer::Get().SubmitFullscreenQuad(
			CommandBuffer, m_RendererData.SceneCompositePipeline,
			m_RendererData.SceneCompositeMaterial, m_RendererData.UniformBufferSet,
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		// End scene composite pass.
		m_RendererData.SceneComposite->EndPass();

		m_RendererData.SceneCompPPTimer.Stop();
	}

	void SceneRenderer::LateCompPhysicsOutline()
	{
		if( !m_PhysicsColliderDrawList.size() )
			return;

		const uint32_t frame = Renderer::Get().GetCurrentFrame();
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

			Renderer::Get().RenderMeshWithoutMaterial(
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

	/*
	void SceneRenderer::SelectionPass()
	{
		const uint32_t frame = Renderer::Get().GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width,m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.SelectionRenderPass->BeginPass( CommandBuffer, m_RendererData.SelectionFramebuffer->GetVulkanFramebuffer(), Extent );

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
			glm::mat4 ViewProjection;
		} u_Matrices{};

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.pCamera->ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		m_RendererData.SelectionMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		for( auto& rCommand : m_TemporarySelectedMeshDrawList )
		{
			glm::mat4 transform = rCommand.entity->GetComponent<TransformComponent>();
			ENTT_ID_TYPE handle = ( ENTT_ID_TYPE ) rCommand.entity->GetHandle();

			glm::vec3 idColor{};

			uint32_t r, g, b;
			r = ( handle >> 16 ) & 0xFF;
			g = ( handle >> 8 ) & 0xFF;
			b = ( handle ) & 0xFF;

			idColor = glm::vec3( r / 255.0f, g / 255.0f, b / 255.0f );

			m_RendererData.SelectionMaterial->SetPC( "u_IDBuffer.IDColor", idColor );

			vkCmdPushConstants( CommandBuffer, m_RendererData.SelectionPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, ( uint32_t ) m_RendererData.SelectionMaterial->GetPushConstantData().Size, m_RendererData.SelectionMaterial->GetPushConstantData().Data );

			Buffer transformBufer( sizeof( glm::mat4 ), &transform );

			Renderer::Get().RenderMeshWithoutMaterial(
				CommandBuffer,
				m_RendererData.SelectionPipeline,
				rCommand.Mesh,
				m_RendererData.SelectionMaterial,
				nullptr,
				m_RendererData.StorageBufferSet,
				rCommand.SubmeshIndex, transformBufer );
		}

		m_RendererData.SelectionRenderPass->EndPass();

		// Now read the texture
		Ref<Image2D> selectionOut = m_RendererData.SelectionFramebuffer->GetColorAttachmentsResources()[ 0 ];

		glm::vec2 pos = Input::Get().MousePosition();

		int x = glm::clamp( ( int ) pos.x - (int)m_RendererData.ViewportPos.x, 0, ( int ) m_RendererData.Width - 1 );
		int y = glm::clamp( ( int ) pos.y - ( int ) m_RendererData.ViewportPos.y, 0, ( int ) m_RendererData.Height - 1 );
	
		Buffer pixel = selectionOut->CopyToBufferPixel( x, y );

		if( pixel.Data )
		{
			uint32_t handle = ( ( ( pixel[ 0 ] & 0x0FF ) << 16 ) | ( ( pixel[ 1 ] & 0x0FF ) << 8 ) | ( pixel[ 2 ] & 0x0FF ) );

			if( handle != 0 || handle != UINT32_MAX )
			{
				SharedPtr<Entity> e = m_pScene->FindEntityByHandle( entt::entity( handle ) );

				if( e )
				{
					m_pScene->AddSelectedEntity( e );
				}
			}
		}

		pixel.Free();
	}
	*/

	void SceneRenderer::TexturePass()
	{
		SAT_PF_EVENT();

		Ref<Pass> pass = VulkanContext::Get().GetDefaultPass();
		const uint32_t frame = Renderer::Get().GetCurrentFrame();

		const VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		const VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// Begin scene composite pass.
		pass->BeginPass( CommandBuffer, VulkanContext::Get().GetSwapchain().GetFramebuffers()[ frame ], Extent );

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
		Renderer::Get().SubmitFullscreenQuad(
			CommandBuffer, m_RendererData.TexturePassPipeline,
			m_RendererData.TexturePassMaterial, m_RendererData.UniformBufferSet,
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
		Ref<StorageBuffer> SB = m_RendererData.StorageBufferSet->Get( 0, 14, Renderer::Get().GetCurrentFrame() );

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
		m_RendererData.BloomTimer.Stop();

		// TEMP
		//return;
	}

	void SceneRenderer::AddScheduledFunction( ScheduledFunc&& rrFunc )
	{
		m_ScheduledFunctions.push_back( rrFunc );
	}

#if !defined(SAT_DIST)
	void SceneRenderer::OnShaderReloaded( const std::string& rName )
	{
		const Ref<Shader> shader = ShaderLibrary::Get().Find( rName );
		auto& rReference = Renderer::Get().FindShaderReference( shader->GetShaderHash() );

		for( auto& rPipeline : rReference.Pipelines )
		{
			rPipeline->Recreate();
		}
	}
#endif

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

		// Every submesh has to have 100 bone transforms
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
		const uint32_t frame = Renderer::Get().GetCurrentFrame();

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

		m_RendererData.SubmeshTransformData[ frame ].VertexBuffer->Reallocate( m_RendererData.SubmeshTransformData[ frame ].pData, off * sizeof( TransformBufferData ) );
	
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
			auto pAllocator = VulkanContext::Get().GetVulkanAllocator();
			auto bufferAloc = pAllocator->GetAllocationFromBuffer( m_RendererData.SBBoneTransforms->Get( 2, 15, frame )->GetBuffer() );

			void* pBufferData = pAllocator->MapMemory<void>( bufferAloc );
			std::memcpy( pBufferData, ( const uint8_t* ) m_RendererData.BoneTransformData, ( uint32_t ) off * sizeof( glm::mat4 ) );
			pAllocator->UnmapMemory( bufferAloc );
		}
	}

	void SceneRenderer::InitRenderer2D()
	{
		// TODO: We don't support multiple Renderer2Ds atm, so we only want the master scene renderer to set the Renderer2D passes.
		if( /*HasFlag( SceneRendererFlag_MasterInstance )*/ !HasFlag( SceneRendererFlag_NoRenderer2D ) )
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

		m_RendererData.CommandBuffer = Renderer::Get().ActiveCommandBuffer();

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
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Scene Composite/Post Processing" );
			SceneCompositePass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Late Composite/SceneRenderer" );
			LateCompPhysicsOutline();
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
		m_PhysicsColliderDrawList.clear();
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
		if( !Application::Get().HasFlag( ApplicationFlag_CreateSceneRenderer ) )
			return;

		// DescriptorSets
		BloomDS                   = nullptr;

		// Vertex and Index buffers
		QuadVertexBuffer->Destroy();
		QuadIndexBuffer->Destroy();

		// Framebuffers
		GeometryFramebuffer       = nullptr;
		SceneCompositeFramebuffer = nullptr;
		PreDepthFramebuffer       = nullptr;
		LateCompositeFramebuffer  = nullptr;

		for( int i = 0; i < 3; ++i )
			BloomTextures[ i ] = nullptr;

		for( int i = 0; i < SHADOW_CASCADE_COUNT; ++i )
			ShadowCascades[ i ].Framebuffer = nullptr;

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
