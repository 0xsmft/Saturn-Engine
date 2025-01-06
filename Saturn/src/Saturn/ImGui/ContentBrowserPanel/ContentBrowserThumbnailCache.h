/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "Saturn/Vulkan/Texture.h"

namespace Saturn {

	constexpr int CB_DIRECTORY_ICON = 0;
	constexpr int CB_FILE_ICON = 1;

	class Asset;

	class ContentBrowserThumbnailCache
	{
	public:
		static void Init();
		static void Terminate();

		static void InsertNew( const std::filesystem::path& rPath, int64_t time, Ref<Texture2D> texture );
		static void ModifyTexture( const std::filesystem::path& rPath, Ref<Texture2D> texture );

		static void OnUpdate();

		[[nodiscard]] static Ref<Texture2D> GetDefault( int Identifier );
		[[nodiscard]] static Ref<Texture2D> GetFor( const Ref<Asset>& rAsset );
		
	public:
		static void Serialise();
		static void Deserialise();
	};

}