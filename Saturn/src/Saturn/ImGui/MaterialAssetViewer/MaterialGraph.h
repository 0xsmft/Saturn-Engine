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

#include "Saturn/NodeEditor/UI/NodeEditor.h"

namespace Saturn {

	class Material;
	class MaterialAsset;
	class MaterialGraphTaskHandler;

	class MaterialGraph : public NodeEditor
	{
	public:
		MaterialGraph();
		MaterialGraph( AssetID id );
		virtual ~MaterialGraph();

		SharedPtr<NodeEditorNodeBase> SetupNewNodeEditor( Ref<Material> material );

		void SetHostMaterialAsset( Ref<MaterialAsset> asset );

#if !defined(SAT_DIST)
	public:
		void BuildTaskCache();
		void OnNodeEditorEvent( NodeEditorAction action );
#endif
	
	private:
		void SimulateChanges();
		void ApplyMaterialChanges();

	private:
		// Local task handler needed for simulation.
		Ref<MaterialGraphTaskHandler> m_TaskHandler;
		Ref<MaterialAsset> m_HostMaterialAsset;
	
		// The temporary material that will have all of its changes applied in real time.
		// Owned by this class.
		Ref<Material> m_EditingMaterial;
	};
	
}
