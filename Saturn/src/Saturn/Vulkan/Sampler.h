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

#include "AddressingMode.h"

#include <string>
#include <vulkan.h>

namespace Saturn {

	enum class SamplerFilter : uint8_t
	{
		Nearest,
		Linear
	};

	enum class CompareOp : uint8_t
	{
		Never = 0,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always
	};

	enum class SamplerBorderColor
	{
		FloatTransparentBlack,
		IntTransparentBlack,
		FloatOpaqueBlack,
		IntOpaqueBlack,
		
		FloatOpaqueWhite,
		IntOpaqueWhite
	};

	struct SamplerSpecification
	{
		std::string DebugName = "Sampler-Obj";

		SamplerFilter MinFilter = SamplerFilter::Nearest;
		SamplerFilter MagFilter = SamplerFilter::Nearest;
		SamplerFilter MipFilter = SamplerFilter::Linear;

		AddressingMode AddressingMode = AddressingMode::Repeat;

		SamplerBorderColor BorderColor = SamplerBorderColor::FloatOpaqueWhite;

		float MipLodBias = 0.0f;
		float MinLod = 0.0f;
		float MaxLod = 0.0f;
		
		// NOTE: Anisotropy is auto enabled if this value is greater than zero.
		float MaxAnisotropy = 0.0f;

		CompareOp CompareOperation = CompareOp::Always;
		bool CompareEnable = false;
	};

	//
	// Sampler
	// 
	// Provides a wrapper for a single sampler object, useful if we ever need to have a single sampler without needing
	// to create an Image or Texture
	//
	class Sampler : public RefTarget
	{
		Sampler( const Sampler& ) = delete;
		Sampler& operator=( const Sampler& ) = delete;
	public:
		Sampler( const SamplerSpecification& rSpec );
		virtual ~Sampler();

	public:
		VkSampler GetSampler() const { return m_Sampler; }

		const VkDescriptorImageInfo& GetDescriptorImageInfo() const { return m_DescriptorImageInfo; }
		VkDescriptorImageInfo& GetDescriptorImageInfo() { return m_DescriptorImageInfo; }

	private:
		void Create();
		void Destroy();

	private:
		SamplerSpecification m_Specification;
		VkDescriptorImageInfo m_DescriptorImageInfo{};

		VkSampler m_Sampler = VK_NULL_HANDLE;
	};
	
}
