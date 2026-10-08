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
#include "ShaderAsset.h"

#include "Saturn/Vulkan/Shader.h"

#if !defined(SAT_DIST)
#include "Saturn/Project/Project.h"
#endif

namespace Saturn {

	ShaderAsset::ShaderAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

#if !defined(SAT_DIST)
	void ShaderAsset::CopyTemplateFile()
	{
		std::filesystem::copy( "content/Templates/CustomShaderTemplate.glsl", Project::GetActiveProject()->FilepathAbs( Path ) );
	}

	bool ShaderAsset::CompileAll()
	{
		const auto absPathToShader = Project::GetActiveProject()->FilepathAbs( Path );

		m_ShaderForMaterial = Ref<Shader>::Create( absPathToShader );
		if( !m_ShaderForMaterial->DidShaderDidCompileSuccessfully() )
		{
			return false;
		}
		else
		{
			m_ShaderForDynamicMeshes = Ref<Shader>::Create( absPathToShader, true );
			if( m_ShaderForDynamicMeshes->DidShaderDidCompileSuccessfully() )
			{
				return true;
			}
		}

		return false;
	}
#endif

	bool ShaderAsset::TryLoadShaders()
	{
		return CompileAll();

		const auto cachePath = Project::GetActiveProject()->GetFullCachePath() / "PerUser";

		// Static shader
		const auto staticShaderPath = ( cachePath / Name ).replace_extension( ".scgs" );

		if( !std::filesystem::exists( staticShaderPath ) )
		{
			return false;
		}
		return true;
	}

	Ref<Shader> ShaderAsset::GetShaderForMaterial() const
	{
		return m_ShaderForMaterial;
	}

	Ref<Shader> ShaderAsset::GetShaderForDynamicMeshes() const
	{
		return m_ShaderForDynamicMeshes;
	}

}
