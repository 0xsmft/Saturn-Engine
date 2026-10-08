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

#include "Asset.h"

namespace Saturn {

	class Shader;

	//
	// ShaderAsset
	// 
	// In Saturn this class represents two Vulkan Shaders:
	//
	// One of them will be a static mesh shader and the
	// other will be compiled as if it was for a
	// dynamic mesh.
	// 
	// A ShaderAsset only exists in the game and should
	// NOT be confused with the Shader class (that lives
	// in /Vulkan/Shader.h)
	//
	class ShaderAsset : public Asset
	{
	public:
		ShaderAsset( const Ref<Asset>& rBase );
		virtual ~ShaderAsset() = default;

		virtual void OnDelete() override {}
		virtual void OnAssetDependencyReplace( AssetID oldID, AssetID newID ) override {}
		virtual bool CanPurge() const override { return false; }

#if !defined(SAT_DIST)
		void CopyTemplateFile();

		bool CompileAll();
#endif

	public:
		bool TryLoadShaders();

	public:
		const std::string& GetShaderSourceCode() const { return m_ShaderSourceCode; }

		Ref<Shader> GetShaderForMaterial() const;
		Ref<Shader> GetShaderForDynamicMeshes() const;

	private:
		std::string m_ShaderSourceCode;

		// Also known as the static shader.
		Ref<Shader> m_ShaderForMaterial;

		// Also known as the dynamic mesh shader.
		Ref<Shader> m_ShaderForDynamicMeshes;

	private:
		friend class ShaderViewerWindow;
        friend class ShaderPrototypeAssetSerialiser;
	};
}
