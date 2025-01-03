/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "Texture.h"
#include "Mesh.h"
#include "Material.h"
#include "ComputePipeline.h"
#include "Renderer2D.h"
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/Core/Memory/Buffer.h"

#include "Saturn/Core/OptickProfiler.h"

#include <Saturn/Core/Ruby/RubyWindow.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <backends/imgui_impl_vulkan.h>

#include <random>

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

		m_RendererData.IsSwapchainTarget = HasFlag( SceneRendererFlag_SwapchainTarget );

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

		m_RendererData.SSAOPassTimer.Reset();
		m_RendererData.SSAOPassTimer.Stop();

		m_RendererData.AOCompositeTimer.Reset();
		m_RendererData.AOCompositeTimer.Stop();

		constexpr size_t TransformCount = static_cast<size_t>( 1024 ) * 10;
		m_RendererData.SubmeshTransformData.resize( MAX_FRAMES_IN_FLIGHT );
		
		for( uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_RendererData.SubmeshTransformData[ i ].VertexBuffer = Ref<VertexBuffer>::Create( sizeof( TransformBufferData ) * TransformCount );
			m_RendererData.SubmeshTransformData[ i ].pData = new TransformBufferData[ TransformCount ];
		}

		//////////////////////////////////////////////////////////////////////////

		// TODO: We don't support multiple Renderer2Ds atm, so we only want the master scene renderer to set the Renderer2D passes.
		if( HasFlag( SceneRendererFlag_MasterInstance ) )
		{
			Renderer2D::Get().SetInitialRenderPass( m_RendererData.LateCompositePass, m_RendererData.LateCompositeFramebuffer );
		}

		Renderer::Get().AddShaderReloadCB( SAT_BIND_EVENT_FN( OnShaderReloaded ) );
	}

	Ref<Image2D> SceneRenderer::CompositeImage()
	{
		return m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];
	}

	void SceneRenderer::Terminate()
	{
		Renderer::Get().ClearShaderReferences();

		m_pScene = nullptr;

		FlushDrawList();

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
		FBSpec.ExistingImages[ 3 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentsResource();
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
	}

	void SceneRenderer::InitDirShadowMap()
	{
		m_RendererData.ShadowCascades.resize( SHADOW_CASCADE_COUNT );
		m_RendererData.DirShadowMapPasses.resize( SHADOW_CASCADE_COUNT );
		m_RendererData.DirShadowMapPipelines.resize( SHADOW_CASCADE_COUNT );

		if( !m_RendererData.DirShadowMapShader )
		{
			m_RendererData.DirShadowMapShader = ShaderLibrary::Get().FindOrLoad( "ShadowMap", "content/shaders/ShadowMap.glsl" );
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
			{ ShaderDataType::Float3, "a_Position" },
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
		PipelineSpec.RequestDescriptorSets = { ShaderType::Vertex, 0 };

		// Layered image
		Ref<Image2D> shadowImage = Ref<Image2D>::Create( ImageFormat::DEPTH32F, ( uint32_t ) SHADOW_MAP_SIZE, ( uint32_t ) SHADOW_MAP_SIZE, 4 );
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
			m_RendererData.LightCullingShader = ShaderLibrary::Get().FindOrLoad( "LightCulling", "content/shaders/LightCulling.glsl" );
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
		PipelineSpec.RequestDescriptorSets = { ShaderType::Vertex, 0 };
		PipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS;
		PipelineSpec.HasColorAttachment = false;
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

		m_RendererData.PreDepthPipeline = Ref<Pipeline>::Create( PipelineSpec );

		//////////////////////////////////////////////////////////////////////////
		// Light culling
		//////////////////////////////////////////////////////////////////////////
		if( m_RendererData.LightCullingPipeline )
			m_RendererData.LightCullingPipeline = nullptr;

		m_RendererData.LightCullingPipeline = Ref<ComputePipeline>::Create( m_RendererData.LightCullingShader );

		if( m_RendererData.LightCullingDescriptorSet )
			m_RendererData.LightCullingDescriptorSet = nullptr;

		m_RendererData.LightCullingDescriptorSet = m_RendererData.LightCullingShader->CreateDescriptorSet( 0 );

		m_RendererData.LightCullingShader->WriteDescriptor( "u_PreDepth", m_RendererData.PreDepthFramebuffer->GetDepthAttachmentsResource()->GetDescriptorInfo(), m_RendererData.LightCullingDescriptorSet->GetVulkanSet() );
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
		}

		if( !m_RendererData.SC_DescriptorSet )
			m_RendererData.SC_DescriptorSet = m_RendererData.SceneCompositeShader->CreateDescriptorSet( 0 );

		m_RendererData.SceneCompositeShader->WriteDescriptor( "u_GeometryPassTexture", m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ]->GetDescriptorInfo(), m_RendererData.SC_DescriptorSet->GetVulkanSet() );
		m_RendererData.SceneCompositeShader->WriteDescriptor( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ]->GetDescriptorInfo(), m_RendererData.SC_DescriptorSet->GetVulkanSet() );
		m_RendererData.SceneCompositeShader->WriteDescriptor( "u_BloomDirtTexture", m_RendererData.BloomDirtTexture->GetDescriptorInfo(), m_RendererData.SC_DescriptorSet->GetVulkanSet() );
		m_RendererData.SceneCompositeShader->WriteDescriptor( "u_DepthTexture", m_RendererData.GeometryFramebuffer->GetDepthAttachmentsResource()->GetDescriptorInfo(), m_RendererData.SC_DescriptorSet->GetVulkanSet() );

		m_RendererData.SceneCompositeShader->WriteAllUBs( m_RendererData.SC_DescriptorSet );

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
			NewSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentsResource();

			m_RendererData.LateCompositeFramebuffer->Recreate( m_RendererData.Width, m_RendererData.Height, NewSpec );
		}
		else	
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.RenderPass = m_RendererData.LateCompositePass;
			FBSpec.Width = m_RendererData.Width;
			FBSpec.Height = m_RendererData.Height;

			FBSpec.ExistingImages[ 0 ] = m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ];
			FBSpec.ExistingImages[ 1 ] = m_RendererData.PreDepthFramebuffer->GetDepthAttachmentsResource();

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
		PipelineSpec.RequestDescriptorSets = { ShaderType::Vertex, 0 };
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		PipelineSpec.PolygonMode = VK_POLYGON_MODE_LINE;
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

		m_RendererData.BloomShader->WriteAllUBs( m_RendererData.BloomDS );

		m_RendererData.BloomDirtTexture = Ref<Texture2D>::Create( "content/textures/editor/BloomDirtTextureUE.png", AddressingMode::Repeat );
	}

	void SceneRenderer::InitTexturePass()
	{
		if( !m_RendererData.IsSwapchainTarget )
			return;

		if( !m_RendererData.TexturePassShader )
		{
			m_RendererData.TexturePassShader = ShaderLibrary::Get().FindOrLoad( "TexturePass", "content/shaders/TexturePass.glsl" );
		}

		if( !m_RendererData.TexturePassDescriptorSet )
			m_RendererData.TexturePassDescriptorSet = m_RendererData.TexturePassShader->CreateDescriptorSet( 0 );

		m_RendererData.TexturePassShader->WriteDescriptor( "u_InputTexture", m_RendererData.SceneCompositeFramebuffer->GetColorAttachmentsResources()[ 0 ]->GetDescriptorInfo(), m_RendererData.TexturePassDescriptorSet->GetVulkanSet() );

		m_RendererData.TexturePassShader->WriteAllUBs( m_RendererData.TexturePassDescriptorSet );

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

	void SceneRenderer::RenderGrid()
	{
		SAT_PF_EVENT();

		if( !HasFlag( SceneRendererFlag_RenderGrid ) )
			return;

		// Set UB Data.
		glm::mat4 trans = glm::rotate( glm::mat4( 1.0f ), glm::radians( 90.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( 16.0f ) );

		RendererData::GridMatricesObject GridMatricesObject = {};
		GridMatricesObject.Transform = trans;
		GridMatricesObject.ViewProjection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		GridMatricesObject.Res = 0.025f;
		GridMatricesObject.Scale = 16.025f;

		auto Data = m_RendererData.GridShader->MapUB( ShaderType::All, 0, 0 );

		memcpy( Data, &GridMatricesObject, sizeof( GridMatricesObject ) );

		m_RendererData.GridShader->UnmapUB( ShaderType::All, 0, 0 );

		m_RendererData.GridShader->WriteAllUBs( m_RendererData.GridDescriptorSet );

		Renderer::Get().SubmitFullscreenQuad(
			m_RendererData.CommandBuffer, m_RendererData.GridPipeline, m_RendererData.GridDescriptorSet, m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );
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

		auto pAllocator = VulkanContext::Get().GetVulkanAllocator();
		auto& UBs = m_RendererData.SkyboxDescriptorSet;

		m_RendererData.SkyboxShader->WriteDescriptor( "u_CubeTexture", m_RendererData.SceneEnvironment->IrradianceMap->GetDescriptorInfo(), m_RendererData.SkyboxDescriptorSet->GetVulkanSet() );

		RendererData::SkyboxMatricesObject SkyboxMatricesObject = {};
		SkyboxMatricesObject.InverseVP = glm::inverse( m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix );

		m_RendererData.SkyboxShader->UploadUB( ShaderType::Vertex, 0, 0, &SkyboxMatricesObject, sizeof( SkyboxMatricesObject ) );

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

		m_RendererData.SkyboxShader->UploadUB( ShaderType::Fragment, 0, 2, &u_Data, sizeof( u_Data ) );

		m_RendererData.SkyboxShader->WriteAllUBs( m_RendererData.SkyboxDescriptorSet );

		Renderer::Get().SubmitFullscreenQuad(
			CommandBuffer, m_RendererData.SkyboxPipeline, m_RendererData.SkyboxDescriptorSet, m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );
	}

	void SceneRenderer::CheckInvalidSkybox()
	{
		if( !m_pScene )
			return;

		// Invalid skybox, maybe null from loading a new scene? This only happens on the first frames so this is a hack.
		if( m_RendererData.SceneEnvironment->IrradianceMap == nullptr && m_RendererData.SceneEnvironment->RadianceMap == nullptr )
		{
			Ref<Entity> SkylightEntity = nullptr;

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

				m_RendererData.SkyboxShader->WriteAllUBs( m_RendererData.SkyboxDescriptorSet );
			}
		}
	}

	void SceneRenderer::UpdateCascades( const glm::vec3& Direction )
	{
		const auto& viewProjection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

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
			float p = ( i + 1 ) / static_cast< float >( SHADOW_CASCADE_COUNT );
			float log = minZ * std::pow( RATIO, p );
			float uniform = minZ + RANGE * p;
			float d = m_RendererData.CascadeSplitLambda * ( log - uniform ) + uniform;
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
			for( uint32_t i = 0; i < 8; i++ )
			{
				glm::vec4 invCorner = invCam * glm::vec4( frustumCorners[ i ], 1.0f );
				frustumCorners[ i ] = invCorner / invCorner.w;
			}

			for( uint32_t i = 0; i < 4; i++ )
			{
				glm::vec3 dist = frustumCorners[ i + 4 ] - frustumCorners[ i ];
				frustumCorners[ i + 4 ] = frustumCorners[ i ] + ( dist * splitDist );
				frustumCorners[ i ] = frustumCorners[ i ] + ( dist * lastSplitDist );
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3( 0.0f );
			for( uint32_t i = 0; i < 8; i++ )
				frustumCenter += frustumCorners[ i ];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for( uint32_t i = 0; i < 8; i++ )
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
		}

		if( !m_RendererData.GridDescriptorSet )
			m_RendererData.GridDescriptorSet = m_RendererData.GridShader->CreateDescriptorSet( 0 );

		m_RendererData.GridShader->WriteAllUBs( m_RendererData.GridDescriptorSet );

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

		m_RendererData.GridPipeline = Ref< Pipeline >::Create( PipelineSpec );
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
		}

		if( !m_RendererData.SkyboxDescriptorSet )
			m_RendererData.SkyboxDescriptorSet = m_RendererData.SkyboxShader->CreateDescriptorSet( 0 );

		if( !m_RendererData.PreethamDescriptorSet )
			m_RendererData.PreethamDescriptorSet = m_RendererData.PreethamShader->CreateDescriptorSet( 0 );

		m_RendererData.SkyboxShader->WriteAllUBs( m_RendererData.SkyboxDescriptorSet );
		m_RendererData.PreethamShader->WriteAllUBs( m_RendererData.PreethamDescriptorSet );

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
			auto FrameTimings = Renderer::Get().GetFrameTimings();

			float shadowPassTime = 0.0f;

			if( m_RendererData.EnableShadows )
			{
				for( int i = 0; i < SHADOW_CASCADE_COUNT; i++ )
				{
					shadowPassTime += m_RendererData.ShadowMapTimers[ i ].ElapsedMilliseconds();
				}
			}

			ImGui::Text( "Renderer::BeginFrame: %.2f ms", FrameTimings.first );

			ImGui::Text( "SceneRenderer::PreDepthPass: %.2f ms", m_RendererData.PreDepthTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::ShadowMapPass: %.2f ms", shadowPassTime );

			ImGui::Text( "SceneRenderer::LightCulling: %.4f ms", m_RendererData.LightCullingTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::GeometryPass: %.2f ms", m_RendererData.GeometryPassTimer.ElapsedMilliseconds() );

			ImGui::Text( "SceneRenderer::BlomPass: %.3f ms", m_RendererData.BloomTimer.ElapsedMilliseconds() );

			ImGui::Text( "Renderer::EndFrame - Queue Present: %.2f ms", Renderer::Get().GetQueuePresentTime() );

			ImGui::Text( "Renderer::EndFrame: %.2f ms", FrameTimings.second );

			ImGui::Text( "Total (RenderThread::Execute): %.2f ms", RenderThread::Get().GetWaitTime() );
			ImGui::Text( "Total : %.2f ms", Application::Get().Time().Milliseconds() );

			if( ImGui::Button( "Screenshot" ) )
			{
				m_RendererData.SceneCompositeFramebuffer->Screenshot( 0, "SceneComp.png" );
			}

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
				auto framebuffer = m_RendererData.ShadowCascades[ index ].Framebuffer->GetDepthAttachmentsResource();

				float size = ImGui::GetContentRegionAvail().x;

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

				float size = ImGui::GetContentRegionAvail().x;

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

	void SceneRenderer::SubmitStaticMesh( Ref<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform )
	{
		SAT_PF_EVENT();

		auto& id = mesh->ID;

		auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); i++ )
		{
			StaticMeshKey key = { mesh->ID, materialRegistry, (uint32_t)i };

			glm::mat4 submeshTransform = transform * submeshes[ i ].Transform;

			auto& command = m_DrawList[ key ];
			command.entity = entity;
			command.Mesh = mesh;
			command.SubmeshIndex = ( uint32_t ) i;
			command.Instances++;

			auto& shadow = m_ShadowMapDrawList[ key ];
			shadow.entity = entity;
			shadow.Mesh = mesh;
			shadow.SubmeshIndex = ( uint32_t ) i;
			shadow.Instances++;

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

	void SceneRenderer::SubmitPhysicsCollider( Ref<Entity> entity, Ref< StaticMesh > mesh, Ref<MaterialRegistry> materialRegistry, const glm::mat4& transform )
	{
		SAT_PF_EVENT();

		auto& id = mesh->ID;
		auto& submeshes = mesh->Submeshes();
		for( size_t i = 0; i < submeshes.size(); i++ )
		{
			StaticMeshKey key = { mesh->ID, materialRegistry, ( uint32_t ) i };

			auto& command = m_PhysicsColliderDrawList[ key ];
			command.entity = entity;
			command.Mesh = mesh;
			command.SubmeshIndex = ( uint32_t ) i;
			command.Instances++;
		}
	}

	void SceneRenderer::SetViewportSize( uint32_t w, uint32_t h )
	{
		if( m_RendererData.Width != w && m_RendererData.Height != h )
		{
			m_RendererData.Width = w;
			m_RendererData.Height = h;
			m_RendererData.Resized = true;
		}
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
			m_RendererData.BloomTextures[ i ]->Terminate();

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

		m_RendererData.SceneCompositeShader->WriteDescriptor( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ]->GetDescriptorInfo(), m_RendererData.SC_DescriptorSet->GetVulkanSet() );

		CreateSkyboxComponents();
		CreateGridComponents();

		if( HasFlag( SceneRendererFlag_MasterInstance ) )
		{
			Renderer2D::Get().SetInitialRenderPass( m_RendererData.LateCompositePass, m_RendererData.LateCompositeFramebuffer );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// RENDERER SHADER 
	//////////////////////////////////////////////////////////////////////////

	// Vertex, Binding 1
	struct LightData
	{
		glm::mat4 LightMatrix[ 4 ];
	};

	// Fragment, Binding 2
	struct SceneData
	{
		DirLight Lights;
		glm::vec3 CameraPosition;
	};

	// Fragment, Binding 3
	struct ShadowData
	{
		glm::vec4 CascadeSplits;
	};

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
		Renderer::Get().SetSceneEnvironment( m_RendererData.ShadowCascades[ 0 ].Framebuffer->GetDepthAttachmentsResource(), m_RendererData.SceneEnvironment, m_RendererData.BRDFLUT_Texture );

		RenderStaticMeshes();

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		//////////////////////////////////////////////////////////////////////////

		// End geometry pass.
		m_RendererData.GeometryPass->EndPass();

		m_RendererData.GeometryPassTimer.Stop();
	}

	void SceneRenderer::RenderStaticMeshes()
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		Ref< Shader > StaticMeshShader = m_RendererData.StaticMeshShader;

		// u_Matrices
		RendererData::StaticMeshMatrices u_Matrices = {};
		u_Matrices.View = m_RendererData.CurrentCamera.ViewMatrix;
		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		LightData u_LightData = {};
		RendererData::PointLights u_Lights;

		u_Lights.nbLights = int( m_pScene->m_Lights.PointLights.size() );

		memcpy( u_Lights.Lights, m_pScene->m_Lights.PointLights.data(), m_pScene->m_Lights.GetPointLightSize() );

		SceneData u_SceneData = {};
		ShadowData u_ShadowData = {};

		struct DebugData
		{
			int TilesCountX;
		} u_DebugData = {};

		u_DebugData.TilesCountX = ( int ) m_RendererData.LightCullingWorkGroups.x;

		auto dirLight = m_pScene->m_Lights.DirectionalLights[ 0 ];

		auto invView = glm::inverse( u_Matrices.View );

		u_SceneData.CameraPosition = invView[ 3 ];
		u_SceneData.Lights = { .Direction = dirLight.Direction, .Radiance = dirLight.Radiance, .Multiplier = dirLight.Intensity };

		if( m_RendererData.EnableShadows )
		{
			for( int i = 0; i < SHADOW_CASCADE_COUNT; i++ )
			{
				u_ShadowData.CascadeSplits[ i ] = m_RendererData.ShadowCascades[ i ].SplitDepth;
				u_LightData.LightMatrix[ i ] = m_RendererData.ShadowCascades[ i ].ViewProjection;
			}
		}

		StaticMeshShader->UploadUB( ShaderType::Vertex, 0, 0, &u_Matrices, sizeof( u_Matrices ) );
		StaticMeshShader->UploadUB( ShaderType::Vertex, 0, 1, &u_LightData, sizeof( u_LightData ) );

		StaticMeshShader->UploadUB( ShaderType::Fragment, 0, 2, &u_SceneData, sizeof( u_SceneData ) );
		StaticMeshShader->UploadUB( ShaderType::Fragment, 0, 3, &u_ShadowData, sizeof( u_ShadowData ) );
		StaticMeshShader->UploadUB( ShaderType::Fragment, 0, 12, &u_DebugData, sizeof( u_DebugData ) );

		//StaticMeshShader->UploadUB( ShaderType::Fragment, 0, 13, &u_Lights, sizeof( u_Lights ) );
		StaticMeshShader->UploadUB( ShaderType::Fragment, 0, 13, &u_Lights, 16ull + sizeof( PointLight ) * u_Lights.nbLights );

		for( auto&& [key, Cmd] : m_DrawList )
		{
			// Entity may of been deleted.
			if( !Cmd.entity )
				continue;

			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			// Render Submesh
			Renderer::Get().SubmitMesh( m_RendererData.CommandBuffer,
				m_RendererData.StaticMeshPipeline,
				Cmd.Mesh, m_RendererData.StorageBufferSet, key.Registry, Cmd.SubmeshIndex, Cmd.Instances, m_RendererData.SubmeshTransformData[ frame ].VertexBuffer, rTransformData.Offset );
		}
	}

	void SceneRenderer::DirShadowMapPass()
	{
		SAT_PF_EVENT();

		if( !m_RendererData.EnableShadows )
			return;

		auto pAllocator = VulkanContext::Get().GetVulkanAllocator();
		VkExtent2D Extent = { ( uint32_t ) SHADOW_MAP_SIZE, ( uint32_t ) SHADOW_MAP_SIZE };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;
		uint32_t frame = Renderer::Get().GetCurrentFrame();

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

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		//////////////////////////////////////////////////////////////////////////

		Ref< Shader > ShadowShader = m_RendererData.DirShadowMapShader;

		UpdateCascades( m_pScene->m_Lights.DirectionalLights[ 0 ].Direction );

		// u_Matrices
		struct UB_Matrices
		{
			glm::mat4 ViewProjection[ 4 ];
		} u_Matrices;

		u_Matrices = {};

		for( size_t i = 0; i < SHADOW_CASCADE_COUNT; i++ )
		{
			u_Matrices.ViewProjection[ i ] = m_RendererData.ShadowCascades[ i ].ViewProjection;
		}

		auto pData = m_RendererData.DirShadowMapShader->MapUB( ShaderType::Vertex, 0, 0 );

		memcpy( pData, &u_Matrices, sizeof( u_Matrices ) );

		m_RendererData.DirShadowMapShader->UnmapUB( ShaderType::Vertex, 0, 0 );

		for( int i = 0; i < SHADOW_CASCADE_COUNT; i++ )
		{
			m_RendererData.ShadowMapTimers[ i ].Reset();

			RenderPassBeginInfo.framebuffer = m_RendererData.ShadowCascades[ i ].Framebuffer->GetVulkanFramebuffer();
			RenderPassBeginInfo.renderPass = m_RendererData.DirShadowMapPasses[ i ]->GetVulkanPass();
			m_RendererData.DirShadowMapPipelines[ i ]->GetShader()->WriteAllUBs( m_RendererData.DirShadowMapPipelines[ i ]->GetDescriptorSet( ShaderType::Vertex, 0 ) );

			// Begin directional shadow map pass.
			CmdBeginDebugLabel( CommandBuffer, "ShadowMap" );
			vkCmdBeginRenderPass( CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

			vkCmdSetViewport( m_RendererData.CommandBuffer, 0, 1, &Viewport );
			vkCmdSetScissor( m_RendererData.CommandBuffer, 0, 1, &Scissor );

			for( auto&& [key, Cmd] : m_ShadowMapDrawList )
			{
				// Entity may of been deleted.
				if( !Cmd.entity )
					continue;

				// Pass in the cascade index.
				Buffer AdditionalData( sizeof( uint32_t ), &i );

				const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

				Renderer::Get().RenderMeshWithoutMaterial( CommandBuffer, m_RendererData.DirShadowMapPipelines[ i ], Cmd.Mesh, Cmd.Instances, m_RendererData.SubmeshTransformData[ frame ].VertexBuffer, rTransformData.Offset, Cmd.SubmeshIndex, AdditionalData );
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

		uint32_t frame = Renderer::Get().GetCurrentFrame();

		VkExtent2D Extent = { m_RendererData.Width,m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.PreDepthPass->BeginPass( CommandBuffer, m_RendererData.PreDepthFramebuffer->GetVulkanFramebuffer(), Extent );

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

		// u_Matrices
		struct UB_Matrices
		{
			glm::mat4 ViewProjection;
		} u_Matrices;

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		m_RendererData.PreDepthShader->UploadUB( ShaderType::Vertex, 0, 0, &u_Matrices, sizeof( u_Matrices ) );

		m_RendererData.PreDepthShader->WriteAllUBs( m_RendererData.PreDepthPipeline->GetDescriptorSet( ShaderType::Vertex, 0 ) );

		for( auto&& [key, Cmd]: m_DrawList )
		{
			// Entity may of been deleted.
			if( !Cmd.entity )
				continue;

			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			Renderer::Get().RenderMeshWithoutMaterial( CommandBuffer, m_RendererData.PreDepthPipeline, Cmd.Mesh, Cmd.Instances, m_RendererData.SubmeshTransformData[ frame ].VertexBuffer, rTransformData.Offset, Cmd.SubmeshIndex );
		}

		m_RendererData.PreDepthPass->EndPass();
		m_RendererData.PreDepthTimer.Stop();
	}

	void SceneRenderer::SceneCompositePass()
	{
		SAT_PF_EVENT();

		VkExtent2D Extent = { m_RendererData.Width,m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// Begin scene composite pass.
		m_RendererData.SceneComposite->BeginPass( CommandBuffer, m_RendererData.SceneCompositeFramebuffer->GetVulkanFramebuffer(), Extent );

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

		// Actual scene composite pass.

		m_RendererData.SceneCompositeShader->WriteAllUBs( m_RendererData.SC_DescriptorSet );

		Renderer::Get().SubmitFullscreenQuad(
			CommandBuffer, m_RendererData.SceneCompositePipeline,
			m_RendererData.SC_DescriptorSet,
			m_RendererData.QuadIndexBuffer, m_RendererData.QuadVertexBuffer );

		// End scene composite pass.
		m_RendererData.SceneComposite->EndPass();
	}

	void SceneRenderer::LateCompPhysicsOutline()
	{
		if( !m_PhysicsColliderDrawList.size() )
			return;

		uint32_t frame = Renderer::Get().GetCurrentFrame();
		VkExtent2D Extent = { m_RendererData.Width,m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		m_RendererData.LateCompositePass->BeginPass( CommandBuffer, m_RendererData.LateCompositeFramebuffer->GetVulkanFramebuffer(), Extent );

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
		} u_Matrices;

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;

		m_RendererData.PhysicsOutlineShader->UploadUB( ShaderType::Vertex, 0, 0, &u_Matrices, sizeof( u_Matrices ) );

		m_RendererData.PhysicsOutlineShader->WriteAllUBs( m_RendererData.PhysicsOutlinePipeline->GetDescriptorSet( ShaderType::Vertex, 0 ) );

		for( auto& [key, Cmd] : m_PhysicsColliderDrawList )
		{
			const auto& rTransformData = m_RendererData.MeshTransforms[ key ];

			Renderer::Get().RenderMeshWithoutMaterial( CommandBuffer, m_RendererData.PhysicsOutlinePipeline, Cmd.Mesh, Cmd.Instances, m_RendererData.SubmeshTransformData[ frame ].VertexBuffer, rTransformData.Offset, Cmd.SubmeshIndex );
		}

		m_RendererData.LateCompositePass->EndPass();
	}

	void SceneRenderer::TexturePass()
	{
		SAT_PF_EVENT();

		Ref<Pass> pass = VulkanContext::Get().GetDefaultPass();
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		VkExtent2D Extent = { m_RendererData.Width, m_RendererData.Height };
		VkCommandBuffer CommandBuffer = m_RendererData.CommandBuffer;

		// Begin scene composite pass.
		pass->BeginPass( CommandBuffer, VulkanContext::Get().GetSwapchain().GetFramebuffers()[ frame ], Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = ( float ) m_RendererData.Height;
		Viewport.width = ( float ) m_RendererData.Width;
		Viewport.height = -( float ) m_RendererData.Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0, 0 }, .extent = Extent };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		// Actual scene composite pass.

		m_RendererData.TexturePassShader->WriteAllUBs( m_RendererData.TexturePassDescriptorSet );

		Renderer::Get().SubmitFullscreenQuad(
			CommandBuffer, m_RendererData.TexturePassPipeline,
			m_RendererData.TexturePassDescriptorSet,
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

			float size = m_RendererData.LightCullingWorkGroups.x * m_RendererData.LightCullingWorkGroups.y * 4.0f * 1024.0f;
			m_RendererData.StorageBufferSet->Resize( 0, 14, ( size_t ) size );
		}

		// UBs
		RendererData::PointLights u_Lights;

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

		u_Matrices.ViewProjection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix() * m_RendererData.CurrentCamera.ViewMatrix;
		u_Matrices.Projection = m_RendererData.CurrentCamera.Camera.ProjectionMatrix();
		u_Matrices.View = glm::inverse( m_RendererData.CurrentCamera.ViewMatrix );
		u_Matrices.InvP = glm::inverse( u_Matrices.Projection );

		u_ScreenData.FullResolution = { m_RendererData.Width, m_RendererData.Height };

		u_Lights.nbLights = ( uint32_t ) m_pScene->m_Lights.PointLights.size();

		std::memcpy( u_Lights.Lights, m_pScene->m_Lights.PointLights.data(), m_pScene->m_Lights.GetPointLightSize() );

		m_RendererData.LightCullingShader->UploadUB( ShaderType::Compute, 0, 0, &u_Lights, 16ull + sizeof PointLight * u_Lights.nbLights );
		m_RendererData.LightCullingShader->UploadUB( ShaderType::Compute, 0, 3, &u_ScreenData, sizeof( u_ScreenData ) );
		m_RendererData.LightCullingShader->UploadUB( ShaderType::Compute, 0, 4, &u_Matrices, sizeof( u_Matrices ) );

		m_RendererData.LightCullingShader->WriteAllUBs( m_RendererData.LightCullingDescriptorSet );

		// Write storage buffer
		Ref<StorageBuffer> SB = m_RendererData.StorageBufferSet->Get( 0, 14, Renderer::Get().GetCurrentFrame() );

		m_RendererData.LightCullingShader->WriteSB( 0, 14, SB->GetBufferInfo(), m_RendererData.LightCullingDescriptorSet );

		// Light culling here
		auto& CullingPipeline = m_RendererData.LightCullingPipeline;

		CullingPipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		CullingPipeline->Execute(
			m_RendererData.LightCullingDescriptorSet->GetVulkanSet(),
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
		//m_RendererData.BloomTimer.Stop();

		// TEMP
		//return;

		struct u_Settings
		{
			float Threshold;
			float Knee;
			float TK; // Threshold - Knee
			float DK; // Knee * 2.0f
			float QK; // Knee / 0.25f
			float Stage; // -1 = Prefilter, 0 = Downsample, 1 = Upsample
			float LOD;
		} pc_Settings{};

		pc_Settings.LOD = 0.0f;
		pc_Settings.Threshold = 1.5f;
		pc_Settings.Knee = 0.1f;

		pc_Settings.TK = pc_Settings.Knee - pc_Settings.Threshold;
		pc_Settings.DK = pc_Settings.Knee * 2.0f;
		pc_Settings.QK = pc_Settings.Knee / 0.25f;

		auto& shader = m_RendererData.BloomShader;
		auto& pipeline = m_RendererData.BloomComputePipeline;

		VkDescriptorSet descriptorSet;
		VkDescriptorPool descriptorPool = Renderer::Get().GetDescriptorPool()->GetVulkanPool();

		VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		info.descriptorPool = descriptorPool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = shader->GetSetLayouts().data();

		auto& InputImg = m_RendererData.GeometryFramebuffer->GetColorAttachmentsResources()[ 0 ];

		glm::vec2 workgrps{};

		// Step 0: Bind compute pipeline in graphics queue.
		// Make sure to do it on the graphics queue.
		pipeline->BindWithCommandBuffer( m_RendererData.CommandBuffer );

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Prefilter" );

		// Step 0.5: Prefilter.
		// Here we are just getting anything that is brighter than the bloom threshold.
		pc_Settings.Stage = ( float ) BloomStage::Prefilter;
		
		{
			VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get().GetDevice(), &info, &descriptorSet ) );

			shader->WriteDescriptor( "u_InputTexture", InputImg->GetDescriptorInfo(), descriptorSet );
			shader->WriteDescriptor( "u_BloomTexture", InputImg->GetDescriptorInfo(), descriptorSet );

			auto descriptorInfo = m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo();
			descriptorInfo.imageView = m_RendererData.BloomTextures[ 0 ]->GetOrCreateMipImageView( 0 );

			shader->WriteDescriptor( "o_Image", descriptorInfo, descriptorSet );

			workgrps.x = ( float ) m_RendererData.BloomTextures[ 0 ]->Width() / m_RendererData.BloomWorkSize;
			workgrps.y = ( float ) m_RendererData.BloomTextures[ 0 ]->Height() / m_RendererData.BloomWorkSize;

			pipeline->AddPushConstant( &pc_Settings, 0, sizeof( pc_Settings ) );
			pipeline->Execute( descriptorSet, ( uint32_t ) workgrps.x, ( uint32_t ) workgrps.y, 1 );
		}

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Downsample" );

		if( m_RendererData.BloomTextures[ 0 ]->GetMipMapLevels() <= 1 )
			return;

		uint32_t mips = m_RendererData.BloomTextures[ 0 ]->GetMipMapLevels() - 2;

		// Step 1: Downsample.
		pc_Settings.Stage = ( float ) BloomStage::Downsample;
		for( uint32_t i = 1; i < mips; i++ )
		{
			auto [w, h] = m_RendererData.BloomTextures[ 0 ]->GetMipSize( i );

			workgrps.x = glm::ceil( ( float ) w / m_RendererData.BloomWorkSize );
			workgrps.y = glm::ceil( ( float ) h / m_RendererData.BloomWorkSize );

			{
				auto descriptorInfo = m_RendererData.BloomTextures[ 1 ]->GetDescriptorInfo();
				descriptorInfo.imageView = m_RendererData.BloomTextures[ 1 ]->GetOrCreateMipImageView( i );

				VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get().GetDevice(), &info, &descriptorSet ) );

				shader->WriteDescriptor( "u_InputTexture", m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo(), descriptorSet );
				shader->WriteDescriptor( "u_BloomTexture", InputImg->GetDescriptorInfo(), descriptorSet );

				shader->WriteDescriptor( "o_Image", descriptorInfo, descriptorSet );

				pc_Settings.LOD = i - 1.0f;

				// Step 1.5: Set push constants & execute the compute shader.
				pipeline->AddPushConstant( &pc_Settings, 0, sizeof( pc_Settings ) );
				pipeline->Execute( descriptorSet,
					( uint32_t ) workgrps.x,
					( uint32_t ) workgrps.y,
					( uint32_t ) 1 );
			}

			// Now we do the same again.
			// However this time we swap the textures.
			{
				VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get().GetDevice(), &info, &descriptorSet ) );

				shader->WriteDescriptor( "u_InputTexture", m_RendererData.BloomTextures[ 1 ]->GetDescriptorInfo(), descriptorSet );
				shader->WriteDescriptor( "u_BloomTexture", InputImg->GetDescriptorInfo(), descriptorSet );

				auto descriptorInfo = m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo();
				descriptorInfo.imageView = m_RendererData.BloomTextures[ 0 ]->GetOrCreateMipImageView( i );

				shader->WriteDescriptor( "o_Image", descriptorInfo, descriptorSet );

				pc_Settings.LOD = ( float ) i;

				pipeline->AddPushConstant( &pc_Settings, 0, sizeof( pc_Settings ) );
				pipeline->Execute( descriptorSet,
					( uint32_t ) workgrps.x,
					( uint32_t ) workgrps.y,
					( uint32_t ) 1 );
			}
		}

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "First Upsample" );

		// Step 2: First upsample.
		pc_Settings.Stage = ( float ) BloomStage::FirstUpsample;
		workgrps.x *= 2;
		workgrps.y *= 2;

		{
			VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get().GetDevice(), &info, &descriptorSet ) );

			shader->WriteDescriptor( "u_InputTexture", m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo(), descriptorSet );
			shader->WriteDescriptor( "u_BloomTexture", InputImg->GetDescriptorInfo(), descriptorSet );

			auto info = m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo();
			info.imageView = m_RendererData.BloomTextures[ 2 ]->GetOrCreateMipImageView( mips - 2 );

			shader->WriteDescriptor( "o_Image", info, descriptorSet );

			pc_Settings.LOD--;

			auto [w, h] = m_RendererData.BloomTextures[ 2 ]->GetMipSize( mips - 2 );

			workgrps.x = glm::ceil( ( float ) w / m_RendererData.BloomWorkSize );
			workgrps.y = glm::ceil( ( float ) h / m_RendererData.BloomWorkSize );

			pipeline->AddPushConstant( &pc_Settings, 0, sizeof( pc_Settings ) );
			pipeline->Execute( descriptorSet,
				( uint32_t ) workgrps.x,
				( uint32_t ) workgrps.y,
				( uint32_t ) 1 );
		}

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		CmdBeginDebugLabel( m_RendererData.CommandBuffer, "Upsample" );

		// Step 3: Upsample.
		pc_Settings.Stage = ( float ) BloomStage::Upsample;
		for( int32_t mip = mips - 3; mip >= 0; mip-- )
		{
			auto [w, h] = m_RendererData.BloomTextures[ 2 ]->GetMipSize( mip );

			workgrps.x = glm::ceil( ( float ) w / m_RendererData.BloomWorkSize );
			workgrps.y = glm::ceil( ( float ) h / m_RendererData.BloomWorkSize );

			VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get().GetDevice(), &info, &descriptorSet ) );

			{
				shader->WriteDescriptor( "u_InputTexture", m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo(), descriptorSet );
				shader->WriteDescriptor( "u_BloomTexture", m_RendererData.BloomTextures[ 2 ]->GetDescriptorInfo(), descriptorSet );

				auto info = m_RendererData.BloomTextures[ 0 ]->GetDescriptorInfo();
				info.imageView = m_RendererData.BloomTextures[ 2 ]->GetOrCreateMipImageView( mip );

				shader->WriteDescriptor( "o_Image", info, descriptorSet );

				pc_Settings.LOD = ( float ) mip;

				// Step 2.5: Set push constants & Execute the compute shader.
				pipeline->AddPushConstant( &pc_Settings, 0, sizeof( pc_Settings ) );
				pipeline->Execute( descriptorSet,
					( uint32_t ) workgrps.x,
					( uint32_t ) workgrps.y,
					( uint32_t ) 1 );
			}
		}

		CmdEndDebugLabel( m_RendererData.CommandBuffer );

		pipeline->Unbind();

		m_RendererData.BloomTimer.Stop();
	}

	void SceneRenderer::AddScheduledFunction( ScheduledFunc&& rrFunc )
	{
		m_ScheduledFunctions.push_back( rrFunc );
	}

	void SceneRenderer::OnShaderReloaded( const std::string& rName )
	{
		Ref<Shader> shader = ShaderLibrary::Get().Find( rName );

		auto& rReference = Renderer::Get().FindShaderReference( shader->GetShaderHash() );

		for( auto& rPipeline : rReference.Pipelines )
		{
			rPipeline->Recreate();
		}

		for( auto& rMaterial : rReference.Materials )
		{
			//rMaterial;
		}
	}

	Ref<TextureCube> SceneRenderer::CreateDymanicSky()
	{
		constexpr uint32_t cubemapSize = 512;
		constexpr uint32_t irradianceMap = 32;

		Ref<TextureCube> Environment = Ref<TextureCube>::Create( ImageFormat::RGBA32F, cubemapSize, cubemapSize );

		Ref<Shader> skyShader = ShaderLibrary::Get().Find( "Skybox_Compute" );
		Ref<ComputePipeline> pipeline = Ref<ComputePipeline>::Create( skyShader );

		glm::vec3 params = { m_RendererData.SceneEnvironment->Turbidity, m_RendererData.SceneEnvironment->Azimuth, m_RendererData.SceneEnvironment->Inclination };

		skyShader->WriteDescriptor( "o_CubeMap", Environment->GetDescriptorInfo(), m_RendererData.PreethamDescriptorSet->GetVulkanSet() );

		auto CommandBuffer = m_RendererData.CommandBuffer;

		pipeline->Bind();
		pipeline->AddPushConstant( &params, 0, sizeof( glm::vec3 ) );
		pipeline->Execute( m_RendererData.PreethamDescriptorSet->GetVulkanSet(), cubemapSize / irradianceMap, cubemapSize / irradianceMap, 6 );
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
		m_RendererData.SceneCompositeFramebuffer->Screenshot( 0, rPath, rSize );
	}

	void SceneRenderer::InitBuffers()
	{
		SAT_PF_EVENT();

		// Create our buffers for instance data.
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		uint32_t off = 0;
		for( auto& [id, buffer] : m_RendererData.MeshTransforms )
		{
			buffer.Offset = off * sizeof( TransformBufferData );
			for (const auto& transform : buffer.Data )
			{
				m_RendererData.SubmeshTransformData[ frame ].pData[ off ] = transform;
				off++;
			}
		}

		m_RendererData.SubmeshTransformData[ frame ].VertexBuffer->Reallocate( m_RendererData.SubmeshTransformData[ frame ].pData, off * sizeof( TransformBufferData ) );
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
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Scene Composite - Post Processing" );
			SceneCompositePass();
		}

		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Late Composite (SceneRenderer)" );
			LateCompPhysicsOutline();
		}

		if( m_RendererData.IsSwapchainTarget )
		{
			ScopedDebugLabel label( m_RendererData.CommandBuffer, "Scene Composite - Texture Pass" );
			TexturePass();
		}

		FlushDrawList();
	}

	void SceneRenderer::FlushDrawList()
	{
		m_DrawList.clear();
		m_ShadowMapDrawList.clear();
		m_PhysicsColliderDrawList.clear();
		m_ScheduledFunctions.clear();
		m_RendererData.MeshTransforms.clear();
	}

	void SceneRenderer::SetCamera( const RendererCamera& Camera )
	{
		m_RendererData.CurrentCamera = Camera;
	}

	//////////////////////////////////////////////////////////////////////////
	// RendererData
	//////////////////////////////////////////////////////////////////////////

	void RendererData::Terminate()
	{
		if( !Application::Get().HasFlag( ApplicationFlag_CreateSceneRenderer ) )
			return;

		// DescriptorSets
		GridDescriptorSet         = nullptr;
		SkyboxDescriptorSet       = nullptr;
		SC_DescriptorSet          = nullptr;
		PreethamDescriptorSet     = nullptr;
		LightCullingDescriptorSet = nullptr;
		BloomDS                   = nullptr;
		TexturePassDescriptorSet  = nullptr;

		// Vertex and Index buffers
		QuadVertexBuffer->Destroy();
		QuadIndexBuffer->Destroy();

		// Framebuffers
		GeometryFramebuffer       = nullptr;
		SceneCompositeFramebuffer = nullptr;
		PreDepthFramebuffer       = nullptr;
		LateCompositeFramebuffer  = nullptr;

		for( int i = 0; i < 3; i++ )
			BloomTextures[ i ] = nullptr;

		for( int i = 0; i < SHADOW_CASCADE_COUNT; i++ )
			ShadowCascades[ i ].Framebuffer = nullptr;

		ShadowCascades.clear();

		// Render Passes
		for( int i = 0; i < SHADOW_CASCADE_COUNT; i++ )
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

		for( int i = 0; i < SHADOW_CASCADE_COUNT; i++ )
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

		// Storage buffer set
		StorageBufferSet = nullptr;

		SubmeshTransformData.clear();

		MeshTransforms.clear();
	}

}