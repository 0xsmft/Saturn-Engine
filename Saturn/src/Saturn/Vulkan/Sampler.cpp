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
#include "Sampler.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

namespace Saturn {

	Sampler::Sampler( const SamplerSpecification& rSpec )
		: m_Specification( rSpec )
	{
		Create();
	}

	void Sampler::Create()
	{
		VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.minFilter    = ( VkFilter ) m_Specification.MinFilter;
		samplerInfo.magFilter    = ( VkFilter ) m_Specification.MagFilter;
		samplerInfo.mipmapMode   = ( VkSamplerMipmapMode ) m_Specification.MipFilter;
		samplerInfo.addressModeU = ( VkSamplerAddressMode ) m_Specification.AddressingMode;
		samplerInfo.addressModeV = samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = m_Specification.MipLodBias;
		samplerInfo.minLod = m_Specification.MinLod;
		samplerInfo.maxLod = m_Specification.MaxLod;
		samplerInfo.borderColor = ( VkBorderColor ) m_Specification.BorderColor;
		samplerInfo.anisotropyEnable = ( m_Specification.MaxAnisotropy >= 1.0 );
		samplerInfo.compareEnable = m_Specification.CompareEnable;
		samplerInfo.compareOp = ( VkCompareOp ) m_Specification.CompareOperation;
		samplerInfo.maxAnisotropy = m_Specification.MaxAnisotropy;

		VK_CHECK( vkCreateSampler( VulkanContext::Get()->GetDevice(), &samplerInfo, nullptr, &m_Sampler ) );

		SetDebugUtilsObjectName( m_Specification.DebugName, ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );

		m_DescriptorImageInfo.sampler = m_Sampler;
	}

	void Sampler::Destroy()
	{
		vkDestroySampler( VulkanContext::Get()->GetDevice(), m_Sampler, nullptr );
	}

	Sampler::~Sampler()
	{
		Destroy();
	}

}
