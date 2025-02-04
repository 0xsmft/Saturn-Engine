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

#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	class MaterialAsset;
	struct MaterialEvaluatorValue;

	class MaterialOutputNode : public Node
	{
	public:
		struct RuntimeData
		{
			Ref<MaterialAsset> MaterialAsset = nullptr;
		};
	public:
		MaterialOutputNode();
		MaterialOutputNode( const std::string& rName );

		virtual ~MaterialOutputNode();

		NodeEditorCompilationStatus EvaluateNode( NodeEditorRuntime* evaluator ) override;

	public:
		RuntimeData RuntimeData;

	private:
		void CreateNode();

		void HandleAlbedo( const MaterialEvaluatorValue& rTextureValue );
	};

	class MaterialSampler2DNode : public Node 
	{
	public:
		MaterialSampler2DNode();
		MaterialSampler2DNode( const std::string& rName );

		virtual ~MaterialSampler2DNode();

		NodeEditorCompilationStatus EvaluateNode( NodeEditorRuntime* evaluator ) override;

	public:
		size_t TextureSlot = 0;
	
	private:
		void CreateNode();
	};

	class MaterialColorPickerNode : public Node
	{
	public:
		MaterialColorPickerNode();
		MaterialColorPickerNode( const std::string& rName );

		virtual ~MaterialColorPickerNode();

		NodeEditorCompilationStatus EvaluateNode( NodeEditorRuntime* evaluator ) override;

		void SetColor( const glm::vec3& rColor );

	public:
		size_t TextureSlot = 0;

	private:
		void CreateNode();
	};

	class MaterialGetAssetNode : public Node
	{
	public:
		MaterialGetAssetNode();
		MaterialGetAssetNode( const std::string& rName );

		virtual ~MaterialGetAssetNode();

		NodeEditorCompilationStatus EvaluateNode( NodeEditorRuntime* evaluator ) override;

		AssetID GetAssetID() const;

	private:
		void CreateNode();
	};

	class MaterialSeparateColorRGB : public Node
	{
	public:
		MaterialSeparateColorRGB();
		MaterialSeparateColorRGB( const std::string& rName );

		virtual ~MaterialSeparateColorRGB();

		NodeEditorCompilationStatus EvaluateNode( NodeEditorRuntime* evaluator ) override;

	private:
		void CreateNode();
	};

	class MaterialNodeLibrary
	{
	public:
		static NodeEditorType GetStaticType() { return NodeEditorType::Material; }

		static Ref<MaterialOutputNode> SpawnOutputNode( Ref<NodeEditorBase> nodeEditor );

		static Ref<MaterialGetAssetNode> SpawnGetAsset( Ref<NodeEditorBase> nodeEditor );
		static Ref<MaterialColorPickerNode> SpawnColorPicker( Ref<NodeEditorBase> nodeEditor );
		static Ref<MaterialSampler2DNode> SpawnSampler2D( Ref<NodeEditorBase> nodeEditor );
		static Ref<MaterialSeparateColorRGB> SpawnSeparateColorRGB( Ref<NodeEditorBase> nodeEditor );

		static Ref<Node> SpawnMixColors( Ref<NodeEditorBase> nodeEditor );
	};
}
