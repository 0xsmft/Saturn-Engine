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

#include "Saturn/NodeEditor/Runtime/NodeEditorRuntime.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"

#include <glm/glm.hpp>
#include <stack>

namespace Saturn {

	struct MaterialEvaluatorValue
	{
		uint32_t Slot = 0;
		glm::vec3 Color;
		UUID TextureAssetID = 0;
	};

	class MaterialAsset;
	class NodeEditorBase;
	class Node;

	class MaterialNodeEditorEvaluator : public NodeEditorRuntime
	{
	public:
		MaterialNodeEditorEvaluator( const MaterialNodeEditorEvaluator& ) = delete;

		struct MaterialNodeEdInfo
		{
			Ref<MaterialAsset> HostMaterial;
			UUID OutputNodeID;
		};

	public:
		MaterialNodeEditorEvaluator( const MaterialNodeEdInfo& rInfo );
		virtual ~MaterialNodeEditorEvaluator() = default;

		[[nodiscard]] virtual NodeEditorCompilationStatus EvaluateEditor() override;

		void AddToValueStack( const MaterialEvaluatorValue& rValue );
		std::stack<MaterialEvaluatorValue>& GetTextureStack() { return m_ValueStack; }

	private:
		size_t IsOutputsLinkedToOutNode( const Ref<Node>& rNode );

	private:
		MaterialNodeEdInfo m_Info;
		std::stack<MaterialEvaluatorValue> m_ValueStack;

	private:
		friend class MaterialOutputNode;
	};
}