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
#include "Saturn/Vulkan/Mesh.h"
#include "MaterialViewerNodes.h"

#include "MaterialNodeEditorEvaluator.h"

#include "MaterialAssetViewer.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "builders.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL NODE LIBRARY

	Ref<MaterialGetAssetNode> MaterialNodeLibrary::SpawnGetAsset( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification pin;
		pin.Name = "Asset ID";
		pin.Type = PinType::AssetID;

		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 30, 117, 217 );
		nodeSpec.Name = "Get Asset";

		nodeSpec.Outputs.push_back( pin );

		Ref<MaterialGetAssetNode> node = Ref<MaterialGetAssetNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialColorPickerNode> MaterialNodeLibrary::SpawnColorPicker( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification pin;
		pin.Name = "RGBA";
		pin.Type = PinType::Material_Sampler2D;

		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 252, 186, 3 );
		nodeSpec.Name = "Color Picker";

		nodeSpec.Outputs.push_back( pin );

		Ref<MaterialColorPickerNode> node = Ref<MaterialColorPickerNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialSampler2DNode> MaterialNodeLibrary::SpawnSampler2D( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification pin;
		pin.Name = "Albedo";
		pin.Type = PinType::Material_Sampler2D;

		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 0, 255, 0 );
		nodeSpec.Name = "Sampler2D";

		pin.Name = "RGBA";
		nodeSpec.Outputs.push_back( pin );
		pin.Name = "R";
		nodeSpec.Outputs.push_back( pin );
		pin.Name = "G";
		nodeSpec.Outputs.push_back( pin );
		pin.Name = "B";
		nodeSpec.Outputs.push_back( pin );
		pin.Name = "A";
		nodeSpec.Outputs.push_back( pin );

		pin.Name = "Asset";
		pin.Type = PinType::AssetID;
		nodeSpec.Inputs.push_back( pin );

		Ref<MaterialSampler2DNode> node = Ref<MaterialSampler2DNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<Node> MaterialNodeLibrary::SpawnMixColors( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification output;
		output.Name = "Out";
		output.Type = PinType::Material_Sampler2D;

		PinSpecification input;
		input.Name = "Color 1";
		input.Type = PinType::Material_Sampler2D;

		NodeSpecification nodeSpec;
		nodeSpec.Name = "Mix Colors";
		nodeSpec.Outputs.push_back( output );
		nodeSpec.Inputs.push_back( input );

		input.Name = "Color 2";
		nodeSpec.Inputs.push_back( input );

		input.Name = "Power";
		input.Type = PinType::Float;
		nodeSpec.Inputs.push_back( input );

		Ref<Node> node = Ref<Node>::Create( nodeSpec );
		node->ExecutionType = NodeExecutionType::MaterialMixColors;
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialOutputNode> MaterialNodeLibrary::SpawnOutputNode( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification pin;
		pin.Name = "Albedo";
		pin.Type = PinType::Material_Sampler2D;

		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 255, 128, 128 );
		nodeSpec.Name = "Material Output";
		nodeSpec.Inputs.push_back( pin );

		pin.Name = "Normal";
		nodeSpec.Inputs.push_back( pin );
		pin.Name = "Metallic";
		nodeSpec.Inputs.push_back( pin );
		pin.Name = "Roughness Map";
		nodeSpec.Inputs.push_back( pin );
		pin.Name = "Emission";
		pin.Type = PinType::Float;
		nodeSpec.Inputs.push_back( pin );
		pin.Name = "Roughness";
		nodeSpec.Inputs.push_back( pin );
		pin.Name = "Metalness";
		nodeSpec.Inputs.push_back( pin );

		Ref<MaterialOutputNode> node = Ref<MaterialOutputNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL OUTPUT NODE

	MaterialOutputNode::MaterialOutputNode( const NodeSpecification& rSpec )
		: Node( rSpec )
	{
		ExecutionType = NodeExecutionType::MaterialOutput;
	}

	MaterialOutputNode::~MaterialOutputNode()
	{
		RuntimeData.MaterialAsset = nullptr;
	}

	NodeEditorCompilationStatus MaterialOutputNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEditorCompilationStatus::Failed;

		auto& TextureStack = materialEval->GetTextureStack();

		while( !TextureStack.empty() )
		{
			const auto& tv = TextureStack.top();
			TextureStack.pop();

			if( tv.Slot == 0 )
			{
				HandleAlbedo( tv );
			}
			else if( tv.Slot == 1 )
			{
				RuntimeData.MaterialAsset->SetNormalMap( tv.TextureAssetID );
			}
			else if( tv.Slot == 2 )
			{
				RuntimeData.MaterialAsset->SetMetallicMap( tv.TextureAssetID );
			}
			else if( tv.Slot == 3 )
			{
				RuntimeData.MaterialAsset->SetRoughnessMap( tv.TextureAssetID );
			}
		}

		return NodeEditorCompilationStatus::Success;
	}

	void MaterialOutputNode::HandleAlbedo( const MaterialEvaluatorValue& rTextureValue )
	{
		if( rTextureValue.Color.r != 0.0f || rTextureValue.Color.g != 0.0f || rTextureValue.Color.b != 0.0f )
		{
			RuntimeData.MaterialAsset->SetAlbeoColor( rTextureValue.Color );
		}
		else if( rTextureValue.TextureAssetID != 0 )
		{
			Ref<Asset> TextureAsset = nullptr;
			TextureAsset = AssetManager::Get().FindAsset( rTextureValue.TextureAssetID );

			RuntimeData.MaterialAsset->SetAlbeoColor( glm::vec3( 1.0f ) );

			if( TextureAsset )
			{
				RuntimeData.MaterialAsset->SetAlbeoMap( TextureAsset->Path );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL SAMPLER 2D NODE

	MaterialSampler2DNode::MaterialSampler2DNode( const NodeSpecification& rSpec )
		: Node( rSpec )
	{
		ExecutionType = NodeExecutionType::Sampler2D;
	}

	MaterialSampler2DNode::~MaterialSampler2DNode()
	{
	}

	NodeEditorCompilationStatus MaterialSampler2DNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEditorCompilationStatus::Failed;

		if( TextureSlot != UINT64_MAX )
		{
			auto neighbors = materialEval->GetTargetEditor()->FindNeighbors( this );

			Ref<MaterialGetAssetNode> assetNode = materialEval->GetTargetEditor()->FindNode( neighbors[ 0 ] );

			MaterialEvaluatorValue tv;
			tv.TextureAssetID = assetNode->AssetID;
			tv.Slot = static_cast<uint32_t>( TextureSlot );

			// Add to root node
			materialEval->AddToValueStack( tv );
		}
		
		return NodeEditorCompilationStatus::Success;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL COLOR PICKER NODE

	MaterialColorPickerNode::MaterialColorPickerNode( const NodeSpecification& rSpec )
		: Node( rSpec )
	{
		ExecutionType = NodeExecutionType::ColorPicker;
	}

	MaterialColorPickerNode::~MaterialColorPickerNode()
	{
	}

	NodeEditorCompilationStatus MaterialColorPickerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEditorCompilationStatus::Failed;

		if( TextureSlot != UINT64_MAX )
		{
			MaterialEvaluatorValue tv;
			tv.Slot = static_cast<uint32_t>( TextureSlot );
			tv.Color = PickedColor;

			// Add to root node
			materialEval->AddToValueStack( tv );
		}

		return NodeEditorCompilationStatus::Success;
	}

	void MaterialColorPickerNode::OnRenderOutput( Ref<Pin> pin )
	{
		bool OpenAssetColorPicker = false;

		if( Name == "Color Picker" && pin->Type == PinType::Material_Sampler2D )
		{
			ImGui::BeginHorizontal( "PickerH" );

			ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
			ImRect boundingBox = { ImGui::GetCursorPos(), ImGui::GetCursorPos() + buttonSize };

			bool hovered, held;

			ImGui::ButtonBehavior( boundingBox, ImGui::GetID( &ID ), &hovered, &held );

			// TODO: Ruby and ImGui do not match! so ImGuiButtonFlags_None = 0 and RubyMouse::Left = 0
			if( hovered && ImGui::IsMouseClicked( ImGuiButtonFlags_None ) )
			{
				OpenAssetColorPicker = true;
			}

			Auxiliary::DrawColoredRect( buttonSize, ImVec4( PickedColor.x, PickedColor.y, PickedColor.z, 255.0f ) );

			ImGui::EndHorizontal();
		}

		ed::Suspend();

		if( OpenAssetColorPicker )
		{
			ImGui::OpenPopup( "AssetColorPicker" );
		}

		ImGui::SetNextWindowSize( { 350.0f, 0.0f } );
		if( ImGui::BeginPopup( "AssetColorPicker", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
		{
			bool PopupModified = false;

			constexpr auto FALLBACK_COLOR = ImVec4( 114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f );

			ImVec4 color = ImVec4( PickedColor.x, PickedColor.y, PickedColor.z, 255.0f );

			if( color.x == 0 && color.y == 0 && color.z == 0 && color.w == 0 )
				color = FALLBACK_COLOR;

			if( ImGui::ColorPicker3( "Color Picker", ( float* ) &color ) )
			{
				PickedColor = glm::vec3( color.x, color.y, color.z );

				PopupModified = true;
			}
			else
			{
				if( PopupModified )
				{
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		ed::Resume();
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL GET ASSET NODE

	MaterialGetAssetNode::MaterialGetAssetNode( const NodeSpecification& rSpec )
		: Node( rSpec )
	{
		ExecutionType = NodeExecutionType::AssetID;
	}

	MaterialGetAssetNode::~MaterialGetAssetNode()
	{
	}

	void MaterialGetAssetNode::OnRenderOutput( Ref<Pin> pin )
	{
		bool openAssetIDPopup = false;
		
		if( pin->Type == PinType::AssetID )
		{
			std::string name = AssetID == 0 ? "Select Asset" : std::to_string( AssetID );

			if( ImGui::Button( name.c_str() ) )
			{
				openAssetIDPopup = true;
			}
		}

		ed::Suspend();
		Auxiliary::DrawAssetFinder( AssetType::Texture, &openAssetIDPopup, AssetID );
		ed::Resume();
	}
}