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

namespace Saturn {

	enum class ImageFormat
	{
		// VK_FORMAT_UNDEFINED
		None = 0,

		// Color

		// VK_FORMAT_R8G8B8A8_UNORM
		RGBA8 = 1,

		// VK_FORMAT_R16G16B16A16_UNORM
		RGBA16 = 2,

		// VK_FORMAT_R32G32B32A32_SFLOAT
		RGBA32F = 3,

		// VK_FORMAT_R32G32B32_SFLOAT
		RGB32F = 4,

		// VK_FORMAT_B8G8R8A8_UNORM
		BGRA8 = 5,

		// VK_FORMAT_R8_UNORM
		RED8 = 6,

		// VK_FORMAT_R8G8B8_UNORM
		RGB8 = 7,

		// VK_FORMAT_R8G8_UNORM
		RG8 = 8,

		// VK_FORMAT_R32_SFLOAT
		RED32F = 9,

		// VK_FORMAT_R8_UINT
		RED8UI = 10,

		// Depth

		// VK_FORMAT_D32_SFLOAT
		DEPTH32F = 11,

		// VK_FORMAT_D32_SFLOAT_S8_UINT
		DEPTH24STENCIL8 = 12,

		// VK_FORMAT_D32_SFLOAT
		Depth = DEPTH32F
	};

	// Linear tiled images:
	// These are stored as is and can be copied directly to. But due to the linear nature they're not a good match for GPUs and format and feature support is very limited.
	// It's not advised to use linear tiled images for anything else than copying from host to GPU if buffer copies are not an option.
	//
	// Optimal tiled images:
	// These are stored in an implementation specific layout matching the capability of the hardware. They usually support more formats and features and are much faster.
	// Optimal tiled images are stored on the device and not accessible by the host. So they can't be written directly to (like liner tiled images) and always require some sort of data copy, either from a buffer or a linear tiled image.
	//
	// In Short: Always use optimal tiled images for rendering.
	enum class ImageTiling
	{
		Optimal = 0,
		Linear = 1,
		MaxEnum = 0x7FFFFFFF
	};
	
}
