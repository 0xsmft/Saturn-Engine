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

#include "Saturn/Asset/AssetRegistry.h"

#include "Saturn/ImGui/JobProgress.h"

namespace Saturn {

	class Asset;

	enum class AssetBundleResult
	{
		Success,
		FileNotFound,
		FailedToUncompress,
		InvalidFileHeader,
		FileVersionMismatch,
		InvalidPackFileHeader,
		PackFileVersionMismatch,
		AssetIDMismatch
	};

	inline std::string_view AssetBundleResultToString( AssetBundleResult result )
	{
		switch( result )
		{
			case AssetBundleResult::Success:
				return "Success";
			case AssetBundleResult::FileNotFound:
				return "Asset Bundle file not found";
			case AssetBundleResult::FailedToUncompress:
				return "Failed to uncompress";
			case AssetBundleResult::InvalidFileHeader:
				return "Invalid asset bundle file header";
			case AssetBundleResult::FileVersionMismatch:
				return "Asset bundle version mismatch";
			case AssetBundleResult::InvalidPackFileHeader:
				return "Invalid pack file header";
			case AssetBundleResult::PackFileVersionMismatch:
				return "Pack version mismatch";
			case AssetBundleResult::AssetIDMismatch:
				return "Asset ID mismatch";
		}

		return "Unknown Error";
	}

	class JobProgress;

	class AssetBundle
	{
	public:
		[[nodiscard]] static AssetBundleResult BundleAssets( Ref<JobProgress>& jobProgress );
		[[nodiscard]] static AssetBundleResult ReadBundle();

		[[nodiscard]] static AssetBundleResult BundleMinimal();
		[[nodiscard]] static AssetBundleResult ReadMinimal();

	private:
		static void RTDumpAsset( const Ref<Asset>& rAsset, Ref<AssetRegistry>& AssetBundleRegistry );
	};

}