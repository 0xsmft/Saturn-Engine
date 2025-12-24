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

#include "Saturn/Asset/Asset.h"
#include "Saturn/Vulkan/Texture.h"

#include <filesystem>

namespace Saturn {

	struct AluraMSDFData;

	class AluraFont : public Asset
	{
	public:
		AluraFont( const std::filesystem::path& rFontPath, const Ref<Asset>& rBase );
		AluraFont( const Ref<Asset>& rBase );
		virtual ~AluraFont();

		void Serialise( int width, int height, const std::filesystem::path& rPath );
		void LoadFromCache();
		void Deserialise();

		Ref<Texture2D> GetTexture() const { return m_TextureAtlas; }
		AluraMSDFData* GetMSDFData() const { return m_pMSDFData; }

		glm::vec2 CalcTextSize( float fontSize, const std::string& rText );

		std::filesystem::path GetFontFilepath() const { return m_Filepath; }
		std::string GetFontName() const { return m_Name; }

	private:
		void CreateAtlas( bool overrideCache = false );

	private:
		std::string m_Name;
		std::filesystem::path m_Filepath;
		AluraMSDFData* m_pMSDFData = nullptr;
		Ref<Texture2D> m_TextureAtlas;
	};	
	
}
