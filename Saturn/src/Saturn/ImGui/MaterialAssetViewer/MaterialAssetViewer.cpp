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
#include "MaterialAssetViewer.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "MaterialNodeEditorEvaluator.h"
#include "MaterialGraphNodes.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/Vulkan/Renderer.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Serialisation/YAML/AssetSerialisers.h"

#include "Saturn/Project/Project.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <stack>

#if !defined(SAT_DIST)
#include "Saturn/ImGui/EditorEvents.h"
#endif

namespace ed = ax::NodeEditor;

namespace Saturn {

	MaterialAssetViewer::MaterialAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::Material;

		AddMaterialAsset();
	}

	MaterialAssetViewer::~MaterialAssetViewer()
	{
		// TEMP
		MaterialAssetSerialiser mas;
		mas.Serialise( m_HostMaterialAsset );

		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveAndMarkClean();
		}

		m_HostMaterialAsset = nullptr;
		m_EditingMaterial = nullptr;
	}

	void MaterialAssetViewer::OnImGuiRender()
	{
		DrawInternal();
	}

	void MaterialAssetViewer::HandleAssetDependencyReplace( AssetID oldID, AssetID newID )
	{
#if !defined(SAT_DIST)
		// Discover all nodes that are based from MaterialGetAssetNode and set new the ID if needed.
		for( auto& [Id, pNode] : m_NodeEditor->GetNodes() )
		{
			if( !pNode->GetClass()->IsChildOf( MaterialGetAssetNode::StaticClass() )
				|| pNode->GetClass() != MaterialGetAssetNode::StaticClass() )
			{
				continue;
			}

			// Get the GetAsset node and check if this ID is the one that needs replaced.
			SharedPtr<MaterialGetAssetNode> getAssetNode = pNode.As<MaterialGetAssetNode>();
			if( getAssetNode->GetAssetID() == oldID )
			{
				getAssetNode->SetAssetID( newID );
				SAT_CORE_INFO( "[MaterialAssetViewer] HandleAssetDependencyReplace - Changed DependencyID from {0} to  {1}", oldID, newID );
			}
		}

		/*
		if( m_NodeEditor->Evaluate() != NodeEditorCompilationStatus::Success ) 
		{
			Application::Get()->DispatchEvent<SendEditorNotificationEvent>( "The node editor failed to evaluate." );
		}
		*/

		m_NodeEditor->SaveAndMarkClean();
#endif
	}

	void MaterialAssetViewer::AddMaterialAsset()
	{
		Ref<MaterialAsset> materialAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( m_AssetID );

		m_HostMaterialAsset = materialAsset;
		m_EditingMaterial = Ref<Material>( m_HostMaterialAsset->GetMaterial() );

		m_NodeEditor = SharedPtr<MaterialGraph>::Create( m_AssetID );

		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID ) )
		{
			m_OutputNodeID = m_NodeEditor->FindNode( "Material Output" )->ID;
		}
		else
		{
			SetupNewNodeEditor();
		}

		m_Name = std::format( "{0}##{1}", m_HostMaterialAsset->Name, ( uint64_t ) m_AssetID );

		MaterialNodeEditorEvaluator::MaterialNodeEdInfo info;
		info.HostMaterial = m_HostMaterialAsset;
		info.OutputNodeID = m_OutputNodeID;

		auto rt = Ref<MaterialNodeEditorEvaluator>::Create( info );
		rt->SetTargetNodeEditor( m_NodeEditor );

		m_NodeEditor->SetWindowName( m_Name );
		m_NodeEditor->SetHostMaterialAsset( m_HostMaterialAsset );

		SetupNodeEditorCallbacks();

		// Maybe in the future we would want to do some stuff here.
		m_NodeEditor->OpenWindow( true );
		m_Open = true;
	}

	void MaterialAssetViewer::SetupNodeEditorCallbacks()
	{
		m_NodeEditor->SetCreateNewNodeFunction(
			[&]() -> SharedPtr<NodeEditorNodeBase>
			{
				SharedPtr<NodeEditorNodeBase> node = nullptr;

				ImGui::SeparatorText( "Material" );

				if( ImGui::MenuItem( "Texture Sampler2D" ) )
					node = MaterialNodeLibrary::SpawnSampler2D( m_NodeEditor );

				if( ImGui::MenuItem( "Get Texture Asset" ) )
					node = MaterialNodeLibrary::SpawnGetAsset( m_NodeEditor );

				if( ImGui::MenuItem( "Color RGB" ) )
					node = MaterialNodeLibrary::SpawnColorPicker( m_NodeEditor );

				if( ImGui::MenuItem( "Color Mixer" ) )
					node = MaterialNodeLibrary::SpawnMixColors( m_NodeEditor );

				if( ImGui::MenuItem( "Separate Color (RGB)" ) )
					node = MaterialNodeLibrary::SpawnSeparateColorRGB( m_NodeEditor );

				return node;
			} );
	}

	void MaterialAssetViewer::SetupNewNodeEditor()
	{
		// Add material output node.
		SharedPtr<MaterialOutputNode> OutputNode( NewObject<MaterialOutputNode>( m_NodeEditor.Get() ) );
		m_NodeEditor->AddNode( OutputNode );

		m_OutputNodeID = OutputNode->ID;

		// Read the material data, and create some nodes based of the info.
		SetupNodesFromMaterial();

		MarkDirty();
	}

	void MaterialAssetViewer::SetupNodesFromMaterial()
	{
		std::map<uint32_t, Ref<Texture2D>> IndexToTextureIndex =
		{
			{ 0u, m_HostMaterialAsset->GetAlbeoMap() },
			{ 1u, m_HostMaterialAsset->GetNormalMap() },
			{ 2u, m_HostMaterialAsset->GetMetallicMap() },
			{ 3u, m_HostMaterialAsset->GetRoughnessMap() }
		};

		for( size_t i = 0; i < IndexToTextureIndex.size(); ++i )
		{
			CreateNodesFromTexture( IndexToTextureIndex[ static_cast<uint32_t>( i ) ], static_cast<int>( i ) );
		}
	}

	void MaterialAssetViewer::CreateNodesFromTexture( Ref<Texture2D> texture, int slot )
	{
		const auto& rPath = texture->GetPath();
		const bool HasTexture = texture != Renderer::Get()->GetPinkTexture();

		if( HasTexture )
		{
			// Find the texture asset.
			const auto relativePath = std::filesystem::relative( rPath, Project::GetActiveProject()->GetRootDir() );

			Ref<Asset> TextureAsset = AssetManager::Get()->FindAsset( relativePath );
			AssetID TextureAssetID = TextureAsset->ID;

			SharedPtr<MaterialSampler2DNode> Sampler2DNode;
			SharedPtr<MaterialGetAssetNode> AssetNode;
			Sampler2DNode = MaterialNodeLibrary::SpawnSampler2D( m_NodeEditor );
			AssetNode = MaterialNodeLibrary::SpawnGetAsset( m_NodeEditor );

			AssetNode->SetAssetID( TextureAssetID );

			SharedPtr<NodeEditorNodeBase> OutputNode = m_NodeEditor->FindNode( m_OutputNodeID );

			m_NodeEditor->CreateLink( AssetNode->Outputs[ 0 ], Sampler2DNode->Inputs[ 0 ], AssetNode->Outputs[ 0 ]->GetPinColor() );
			m_NodeEditor->CreateLink( Sampler2DNode->Outputs[ 0 ], OutputNode->Inputs[ slot ], Sampler2DNode->Outputs[ 0 ]->GetPinColor() );
		}
		else if( slot == 0 )
		{
			SharedPtr<MaterialColorPickerNode> colorPickerNode = MaterialNodeLibrary::SpawnColorPicker( m_NodeEditor );

			auto& albedoColor = m_HostMaterialAsset->Get<glm::vec3>( "u_Materials.AlbedoColor" );
			colorPickerNode->SetColor( albedoColor );

			SharedPtr<NodeEditorNodeBase> outputNode = m_NodeEditor->FindNode( m_OutputNodeID );
			m_NodeEditor->CreateLink( colorPickerNode->Outputs[ slot ], outputNode->Inputs[ slot ], colorPickerNode->Outputs[ slot ]->GetPinColor() );
		}
	}

	void MaterialAssetViewer::DrawInternal()
	{
		if( m_NodeEditor->IsOpen() )
		{
			m_NodeEditor->OnImGuiRender();
		}
		else if( m_HostMaterialAsset )
		{
			m_NodeEditor->OpenWindow( false );
			m_Open = false;
		}
	}
}
