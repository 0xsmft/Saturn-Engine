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

#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	class MaterialAsset;
	struct MaterialEvaluatorValue;

	class MaterialOutputNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MaterialOutputNode, NodeEditorBlueprintNode );
	public:
		struct RuntimeData
		{
			Ref<MaterialAsset> MaterialAsset = nullptr;
		};
	public:
		MaterialOutputNode();
		MaterialOutputNode( const std::string& rName );

		virtual ~MaterialOutputNode();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	public:
		RuntimeData RuntimeData;

	private:
		void CreateNode();

		void HandleAlbedo( const MaterialEvaluatorValue& rTextureValue );
	};

	class MaterialSampler2DNode : public NodeEditorBlueprintNode 
	{
		SAT_DECLARE_CLASS( MaterialSampler2DNode, NodeEditorBlueprintNode );
	public:
		MaterialSampler2DNode();
		MaterialSampler2DNode( const std::string& rName );

		virtual ~MaterialSampler2DNode();

	public:
		size_t TextureSlot = 0;
	
	private:
		void CreateNode();
	};

	class MaterialColorPickerNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MaterialColorPickerNode, NodeEditorBlueprintNode );
	public:
		MaterialColorPickerNode();
		MaterialColorPickerNode( const std::string& rName );

		virtual ~MaterialColorPickerNode();

		void SetColor( const glm::vec3& rColor );

		virtual NodeEditorTaskBase* ConvertToTask() override;

	public:
		size_t TextureSlot = 0;

	private:
		void CreateNode();
	};

	class MaterialGetAssetNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MaterialGetAssetNode, NodeEditorBlueprintNode );
	public:
		MaterialGetAssetNode();
		MaterialGetAssetNode( const std::string& rName );

		virtual ~MaterialGetAssetNode();

		AssetID GetAssetID() const;
		void SetAssetID( AssetID id );

	private:
		void CreateNode();
	};

	class MaterialSeparateColorRGBNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MaterialSeparateColorRGBNode, NodeEditorBlueprintNode );
	public:
		MaterialSeparateColorRGBNode();
		MaterialSeparateColorRGBNode( const std::string& rName );

		virtual ~MaterialSeparateColorRGBNode();

	private:
		void CreateNode();
	};

	class MaterialColorMixerNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MaterialColorMixerNode, NodeEditorBlueprintNode );
	public:
		MaterialColorMixerNode();
		MaterialColorMixerNode( const std::string& rName );

		virtual ~MaterialColorMixerNode();

	private:
		void CreateNode();
	};

	class MaterialNodeLibrary
	{
	public:
		static SharedPtr<MaterialOutputNode> SpawnOutputNode( SharedPtr<NodeEditor> nodeEditor );

		static SharedPtr<MaterialGetAssetNode> SpawnGetAsset( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<MaterialColorPickerNode> SpawnColorPicker( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<MaterialSampler2DNode> SpawnSampler2D( SharedPtr<NodeEditor> nodeEditor );
		static SharedPtr<MaterialSeparateColorRGBNode> SpawnSeparateColorRGB( SharedPtr<NodeEditor> nodeEditor );

		static SharedPtr<MaterialColorMixerNode> SpawnMixColors( SharedPtr<NodeEditor> nodeEditor );
	};
}
