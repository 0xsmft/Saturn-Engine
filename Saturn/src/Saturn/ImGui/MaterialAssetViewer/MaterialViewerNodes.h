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

#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	class MaterialAsset;
	struct MaterialEvaluatorValue;

	class MaterialOutputNode : public NodeEditorBlueprintNode
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::MaterialOutput );
	public:
		struct RuntimeData
		{
			Ref<MaterialAsset> MaterialAsset = nullptr;
		};
	public:
		MaterialOutputNode();
		MaterialOutputNode( const std::string& rName );

		virtual ~MaterialOutputNode();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	public:
		RuntimeData RuntimeData;

	private:
		void CreateNode();

		void HandleAlbedo( const MaterialEvaluatorValue& rTextureValue );
	};

	class MaterialSampler2DNode : public NodeEditorBlueprintNode 
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::Sampler2D );
	public:
		MaterialSampler2DNode();
		MaterialSampler2DNode( const std::string& rName );

		virtual ~MaterialSampler2DNode();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	public:
		size_t TextureSlot = 0;
	
	private:
		void CreateNode();
	};

	class MaterialColorPickerNode : public NodeEditorBlueprintNode
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::ColorPicker );
	public:
		MaterialColorPickerNode();
		MaterialColorPickerNode( const std::string& rName );

		virtual ~MaterialColorPickerNode();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

		void SetColor( const glm::vec3& rColor );

	public:
		size_t TextureSlot = 0;

	private:
		void CreateNode();
	};

	class MaterialGetAssetNode : public NodeEditorBlueprintNode
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::AssetID );
	public:
		MaterialGetAssetNode();
		MaterialGetAssetNode( const std::string& rName );

		virtual ~MaterialGetAssetNode();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

		AssetID GetAssetID() const;
		void SetAssetID( AssetID id );

	private:
		void CreateNode();
	};

	class MaterialSeparateColorRGBNode : public NodeEditorBlueprintNode
	{
//		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::MaterialSeparateColorRGB );
	public:
		MaterialSeparateColorRGBNode();
		MaterialSeparateColorRGBNode( const std::string& rName );

		virtual ~MaterialSeparateColorRGBNode();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};

	class MaterialColorMixerNode : public NodeEditorBlueprintNode
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::MaterialMixColors );
	public:
		MaterialColorMixerNode();
		MaterialColorMixerNode( const std::string& rName );

		virtual ~MaterialColorMixerNode();

		NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};

	class MaterialNodeLibrary
	{
	public:
		static void RegisterAllNodes();

	public:
		static NodeEditorType GetStaticType() { return NodeEditorType::Material; }

		static Ref<MaterialOutputNode> SpawnOutputNode( Ref<NodeEditorBase> nodeEditor );

		static Ref<MaterialGetAssetNode> SpawnGetAsset( Ref<NodeEditorBase> nodeEditor );
		static Ref<MaterialColorPickerNode> SpawnColorPicker( Ref<NodeEditorBase> nodeEditor );
		static Ref<MaterialSampler2DNode> SpawnSampler2D( Ref<NodeEditorBase> nodeEditor );
		static Ref<MaterialSeparateColorRGBNode> SpawnSeparateColorRGB( Ref<NodeEditorBase> nodeEditor );

		static Ref<MaterialColorMixerNode> SpawnMixColors( Ref<NodeEditorBase> nodeEditor );
	};
}
