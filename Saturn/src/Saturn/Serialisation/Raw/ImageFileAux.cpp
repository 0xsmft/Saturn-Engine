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

#include "sppch.h"
#include "ImageFileAux.h"

#include <stb_image_write.h>

namespace Saturn::Auxiliary {

	void WriteImageFile( const std::filesystem::path& rPath, ImageFileType Type, uint32_t Width, uint32_t Height, uint32_t Components, const void* pData, int Stride /*= 0*/, bool FlipVectically /*=true*/ )
	{
		stbi_flip_vertically_on_write( FlipVectically );
		std::string Filepath = rPath.string();

		switch( Type )
		{
			case ImageFileType::PNG:
				stbi_write_png( Filepath.c_str(), Width, Height, Components, pData, Stride );
				break;
			case ImageFileType::JPG:
				stbi_write_jpg( Filepath.c_str(), Width, Height, Components, pData, Stride );
				break;
			case ImageFileType::BMP:
				stbi_write_bmp( Filepath.c_str(), Width, Height, Components, pData );
				break;
			case ImageFileType::TGA:
				stbi_write_tga( Filepath.c_str(), Width, Height, Components, pData );
				break;
			case ImageFileType::HDR:
				stbi_write_hdr( Filepath.c_str(), Width, Height, Components, (const float*)pData );
				break;
			
			default:
				std::unreachable();
				break;
		}
	}

}