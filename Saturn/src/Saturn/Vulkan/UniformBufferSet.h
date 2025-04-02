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

#include "UniformBuffer.h"

#include "Saturn/Core/Ref.h"

#include <unordered_map>

namespace Saturn {

	class UniformBufferSet : public RefTarget
	{
	public:
		UniformBufferSet();
		~UniformBufferSet();

		void CreateBuffer( size_t size, uint32_t binding );
		Ref<UniformBuffer> Get( uint32_t set, uint32_t binding, uint32_t frame );

	private:
		void Set( Ref<UniformBuffer>& rBuffer, uint32_t set, uint32_t binding );

	private:
		// Set, Binding, Frame, Buffer
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::unordered_map<uint32_t, Ref<UniformBuffer>>>> m_Buffers;
	};
}