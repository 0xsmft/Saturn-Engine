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

namespace Saturn {

	struct SoundDecodedInformation
	{
		// Audio Format, see: ma_format
		int Format = 0;
		uint32_t Channels = 0;
		uint32_t SampleRate = 0;
		uint64_t BytesPerFrame = 0;
		uint64_t PCMFrameCount = 0;

		Buffer PCMFrames;
	};

	class SoundSpecification : public Asset
	{
	public:
		SoundSpecification() = default;
		SoundSpecification( const Ref<Asset>& rBase ) 
			: Asset( rBase )
		{
		}

		std::filesystem::path SoundSourcePath;
		std::filesystem::path OriginalImportPath;

#if defined(SAT_DIST)
		SoundDecodedInformation DecodedInformation;
#else
		// Stored as YYYY-MM-DD
		std::string LastWriteTime;
#endif
	};
}