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
#include "Saturn/Vulkan/Mesh.h"
#include "MaterialViewerNodes.h"

#include "MaterialNodeEditorEvaluator.h"
#include "MaterialViewerColorPin.h"

#include "Saturn/NodeEditor/AssetIDPin.h"
#include "Saturn/NodeEditor/GlobalNodesList.h"

#include "MaterialAssetViewer.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "builders.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL NODE LIBRARY

	void MaterialNodeLibrary::RegisterAllNodes()
	{
		GlobalNodesList::RegisterLibrary( {
			{ NodeExecutionType::AssetID,                  MaterialNodeLibrary::SpawnGetAsset    },
			{ NodeExecutionType::Sampler2D,                MaterialNodeLibrary::SpawnSampler2D   },
			{ NodeExecutionType::MaterialOutput,           MaterialNodeLibrary::SpawnOutputNode  },
			{ NodeExecutionType::ColorPicker,              MaterialNodeLibrary::SpawnColorPicker },
//			{ NodeExecutionType8192::MaterialSeparateColorRGB, MaterialNodeLibrary::SpawnSeparateColorRGB },
			{ NodeExecutionType::MaterialMixColors,        MaterialNodeLibrary::SpawnMixColors }
		} );
	}

	Ref<MaterialOutputNode> MaterialNodeLibrary::SpawnOutputNode( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<MaterialOutputNode> node = Ref<MaterialOutputNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialGetAssetNode> MaterialNodeLibrary::SpawnGetAsset( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<MaterialGetAssetNode> node = Ref<MaterialGetAssetNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialColorPickerNode> MaterialNodeLibrary::SpawnColorPicker( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<MaterialColorPickerNode> node = Ref<MaterialColorPickerNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialSampler2DNode> MaterialNodeLibrary::SpawnSampler2D( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<MaterialSampler2DNode> node = Ref<MaterialSampler2DNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialSeparateColorRGBNode> MaterialNodeLibrary::SpawnSeparateColorRGB( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<MaterialSeparateColorRGBNode> node = Ref<MaterialSeparateColorRGBNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<MaterialColorMixerNode> MaterialNodeLibrary::SpawnMixColors( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<MaterialColorMixerNode> node = Ref<MaterialColorMixerNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL OUTPUT NODE

	MaterialOutputNode::MaterialOutputNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Material Output";

		CreateNode();
	}

	MaterialOutputNode::MaterialOutputNode( const std::string& rName )
		: NodeEditorBlueprintNode()
	{
		Name = rName;

		CreateNode();
	}

	MaterialOutputNode::~MaterialOutputNode()
	{
		RuntimeData.MaterialAsset = nullptr;
	}

	Saturn::NodeEvaluationState MaterialOutputNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEvaluationState::Failed;

		auto& TextureStack = materialEval->GetTextureStack();

		RuntimeData.MaterialAsset->SetAlbeoColor( Inputs[ 0 ].As<MaterialViewerColorPin>()->Data );

		AssetManager::Get().UnregisterAllAssetDependencies( RuntimeData.MaterialAsset->ID );

		// Textures
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

			if( tv.TextureAssetID != 0 )
				AssetManager::Get().RegisterAssetDependency( RuntimeData.MaterialAsset->ID, tv.TextureAssetID );
		}

		// Emission
		if( materialEval->GetTargetEditor()->IsLinked( Inputs[ 4 ]->ID ) )
		{
			Ref<FloatPin> fpin = Inputs[ 4 ].As<FloatPin>();
			RuntimeData.MaterialAsset->SetEmissive( fpin->Data );
		}

		// Roughness
		if( materialEval->GetTargetEditor()->IsLinked( Inputs[ 5 ]->ID ) )
		{
			Ref<FloatPin> fpin = Inputs[ 5 ].As<FloatPin>();
			RuntimeData.MaterialAsset->SetRoughness( fpin->Data );
		}

		// Metalness
		if( materialEval->GetTargetEditor()->IsLinked( Inputs[ 6 ]->ID ) )
		{
			Ref<FloatPin> fpin = Inputs[ 6 ].As<FloatPin>();
			RuntimeData.MaterialAsset->SetMetalness( fpin->Data );
		}

		return NodeEvaluationState::Evaluated;
	}

	void MaterialOutputNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::MaterialOutput;
		Color = ImColor( 255, 128, 128 );
		CanBeDeleted = false;

		// Inputs
		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Albedo Map",    PinKind::Input ) );
		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Normal Map",    PinKind::Input, false, true ) );
		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Metallic Map",  PinKind::Input, false, true ) );
		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Roughness Map", PinKind::Input, false, true ) );

		// Float inputs
		Inputs.push_back( Ref<FloatPin>::Create( "Emission", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Roughness", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Metalness", PinKind::Input ) );
	}

	void MaterialOutputNode::HandleAlbedo( const MaterialEvaluatorValue& rTextureValue )
	{
		if( rTextureValue.TextureAssetID != 0 )
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

	MaterialSampler2DNode::MaterialSampler2DNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Sampler2D";
		CreateNode();
	}

	MaterialSampler2DNode::MaterialSampler2DNode( const std::string& rName )
		: NodeEditorBlueprintNode()
	{
		Name = rName;
		CreateNode();
	}

	void MaterialSampler2DNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::Sampler2D;
		Color = ImColor( 0, 255, 0 );

		Inputs.push_back( Ref<AssetIDPin>::Create( "Asset",             PinKind::Input, AssetType::Texture ) );
		Outputs.push_back( Ref<MaterialViewerColorPin>::Create( "RGBA", PinKind::Output, true ) );
	}

	MaterialSampler2DNode::~MaterialSampler2DNode()
	{
	}

	Saturn::NodeEvaluationState MaterialSampler2DNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEvaluationState::Failed;

		if( TextureSlot != UINT64_MAX )
		{
			AssetID textureID = Inputs[ 0 ].As<AssetIDPin>()->GetAssetID();

			auto neighbors = materialEval->GetTargetEditor()->FindNeighborsRight( this );
			if( neighbors.size() )
			{
				textureID = materialEval->GetTargetEditor()->FindNode( neighbors[ 0 ] ).As<MaterialGetAssetNode>()->GetAssetID();
			}

			MaterialEvaluatorValue tv;
			tv.TextureAssetID = textureID;
			tv.Slot = static_cast<uint32_t>( TextureSlot );

			// Add to root node
			materialEval->AddToValueStack( tv );
		}
		
		return NodeEvaluationState::Evaluated;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL COLOR PICKER NODE

	MaterialColorPickerNode::MaterialColorPickerNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Color Picker";
		CreateNode();
	}

	MaterialColorPickerNode::MaterialColorPickerNode( const std::string& rName )
		: NodeEditorBlueprintNode()
	{
		Name = rName;
		CreateNode();
	}

	void MaterialColorPickerNode::CreateNode()
	{
		Color = ImColor( 252, 186, 3 );
		ExecutionType = NodeExecutionType::ColorPicker;

		Outputs.push_back( Ref<MaterialViewerColorPin>::Create( "RGBA", PinKind::Output ) );
	}

	MaterialColorPickerNode::~MaterialColorPickerNode()
	{
	}

	Saturn::NodeEvaluationState MaterialColorPickerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEvaluationState::Failed;

		Ref<MaterialViewerColorPin> colorPin = Outputs[ 0 ].As<MaterialViewerColorPin>();

		// Write our data to the input pin of the link.
		auto links = materialEval->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );
		for( const auto& rLink : links )
		{
			Ref<Pin> pin = materialEval->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin )
			{
				Ref<MaterialViewerColorPin> inputColPin = pin.As<MaterialViewerColorPin>();
				inputColPin->Data = colorPin->Data;
			}
		}

		return NodeEvaluationState::Evaluated;
	}

	void MaterialColorPickerNode::SetColor( const glm::vec3& rColor )
	{
		Outputs[ 0 ].As<MaterialViewerColorPin>()->Data = rColor;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL GET ASSET NODE

	MaterialGetAssetNode::MaterialGetAssetNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Get Asset";
		CreateNode();
	}

	MaterialGetAssetNode::MaterialGetAssetNode( const std::string& rName )
		: NodeEditorBlueprintNode()
	{
		Name = rName;
		CreateNode();
	}

	void MaterialGetAssetNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AssetID;
		Color = ImColor( 30, 117, 217 );

		Outputs.push_back( Ref<AssetIDPin>::Create( "Asset ID", PinKind::Output, AssetType::Texture ) );
	}

	MaterialGetAssetNode::~MaterialGetAssetNode()
	{
	}

	Saturn::NodeEvaluationState MaterialGetAssetNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEvaluationState::Failed;

		// Write ID from pin into input pin of link.
		auto links = materialEval->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );

		for( const auto& rLink : links )
		{
			Ref<Pin> pin = materialEval->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin && pin->Type == PinType::AssetID )
			{
				Ref<AssetIDPin> fpin = pin.As<AssetIDPin>();
				fpin->SetAssetID( Outputs[ 0 ].As<AssetIDPin>()->GetAssetID() );
			}
		}

		return NodeEvaluationState::Evaluated;
	}

	AssetID MaterialGetAssetNode::GetAssetID() const
	{
		return Outputs[ 0 ].As<AssetIDPin>()->GetAssetID();
	}

	void MaterialGetAssetNode::SetAssetID( AssetID id )
	{
		Outputs[ 0 ].As<AssetIDPin>()->SetAssetID( id );
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL SEPARATE COLOR RGB

	MaterialSeparateColorRGBNode::MaterialSeparateColorRGBNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Separate Color RGB";
		CreateNode();
	}

	MaterialSeparateColorRGBNode::MaterialSeparateColorRGBNode( const std::string& rName )
		: NodeEditorBlueprintNode()
	{
		Name = rName;
		CreateNode();
	}

	void MaterialSeparateColorRGBNode::CreateNode()
	{
		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Color", PinKind::Input ) );

		Outputs.push_back( Ref<MaterialViewerColorPin>::Create( "R", PinKind::Output, true ) );
		Outputs.push_back( Ref<MaterialViewerColorPin>::Create( "G", PinKind::Output, true ) );
		Outputs.push_back( Ref<MaterialViewerColorPin>::Create( "B", PinKind::Output, true ) );
	}

	MaterialSeparateColorRGBNode::~MaterialSeparateColorRGBNode()
	{
	}

	Saturn::NodeEvaluationState MaterialSeparateColorRGBNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEvaluationState::Failed;

		// Write separate color to the outs
		if( materialEval->GetTargetEditor()->IsLinked( Inputs[ 0 ]->ID ) )
		{
			Ref<MaterialViewerColorPin> inputColorPin = Inputs[ 0 ].As<MaterialViewerColorPin>();

			Outputs[ 0 ].As<MaterialViewerColorPin>()->Data.x = inputColorPin->Data.x;
			Outputs[ 1 ].As<MaterialViewerColorPin>()->Data.y = inputColorPin->Data.y;
			Outputs[ 2 ].As<MaterialViewerColorPin>()->Data.z = inputColorPin->Data.z;
		}

		return NodeEvaluationState::Evaluated;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL COLOR MIXER NODE

	MaterialColorMixerNode::MaterialColorMixerNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Color Mixer";
		CreateNode();
	}

	MaterialColorMixerNode::MaterialColorMixerNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void MaterialColorMixerNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::MaterialMixColors;
		Color = ImColor( 252, 186, 3 );

		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Color 1", PinKind::Input ) );
		Inputs.push_back( Ref<MaterialViewerColorPin>::Create( "Color 2", PinKind::Input ) );

		Inputs.push_back( Ref<FloatPin>::Create( "Power", PinKind::Input ) );

		Outputs.push_back( Ref<MaterialViewerColorPin>::Create( "Result", PinKind::Output, true ) );
	}

	MaterialColorMixerNode::~MaterialColorMixerNode()
	{
	}

	Saturn::NodeEvaluationState MaterialColorMixerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		MaterialNodeEditorEvaluator* materialEval = dynamic_cast< MaterialNodeEditorEvaluator* >( evaluator );

		if( !materialEval )
			return NodeEvaluationState::Failed;

		Ref<MaterialViewerColorPin> col1 = Inputs[ 0 ].As<MaterialViewerColorPin>();
		Ref<MaterialViewerColorPin> col2 = Inputs[ 1 ].As<MaterialViewerColorPin>();

		Ref<FloatPin> power = Inputs[ 2 ].As<FloatPin>();

		glm::vec3 result = glm::mix( col1->Data, col2->Data, power->Data );

		Outputs[ 0 ].As<MaterialViewerColorPin>()->Data = result;

		//////////////////////////////////////////////////////////////////////////
		// Write ID from pin into input pin of link.

		auto links = materialEval->GetTargetEditor()->FindLinksByPin( Outputs[ 0 ]->ID );
		
		for( const auto& rLink : links )
		{
			Ref<Pin> pin = materialEval->GetTargetEditor()->FindPin( rLink->EndPinID );
			if( pin && pin->Type == PinType::Material_Color )
			{
				Ref<MaterialViewerColorPin> colPin = pin.As<MaterialViewerColorPin>();
				colPin->Data = result;
			}
		}

		return NodeEvaluationState::Evaluated;
	}

}
