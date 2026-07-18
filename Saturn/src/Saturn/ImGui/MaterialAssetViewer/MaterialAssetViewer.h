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

#include "MaterialGraph.h"

#include "Saturn/ImGui/AssetViewer.h"

#include "Saturn/Asset/MaterialAsset.h"

namespace Saturn {

	class MaterialAssetViewer : public AssetViewer
	{
	public:
		MaterialAssetViewer( AssetID id );
		virtual ~MaterialAssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override {}
		virtual void OnEvent( Event& rEvent ) override {}

	public:
		void HandleAssetDependencyReplace( AssetID oldID, AssetID newID );

	private:
		void AddMaterialAsset();
		void DrawInternal();

		void SetupNodeEditorCallbacks();
		void SetupNewNodeEditor();
		void SetupNodesFromMaterial();
		void CreateNodesFromTexture( Ref<Texture2D> texture, int slot );
		void DrawSimpleEditor();

	private:
		SharedPtr<MaterialGraph> m_NodeEditor = nullptr;
		Ref<MaterialAsset> m_HostMaterialAsset = nullptr;
		Ref<Material> m_EditingMaterial = nullptr;

		UUID m_OutputNodeID = 0;
		AssetID m_SimpleEditorFinderID = 0;
		bool m_OpenSimpleEditor = false;
	};
}
