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
#include "PipelineCache.h"

#include "Pass.h"

namespace Saturn {

	PipelineCache::PipelineCache()
	{
	}

	void PipelineCache::Clear()
	{
		m_Cache.clear();
	}

	PipelineCache::~PipelineCache()
	{
		Clear();
	}

	bool PipelineCache::DoesPipelineExist( UUID shaderHash ) const
	{
		return m_Cache.contains( shaderHash );
	}

	Ref<Pipeline> PipelineCache::EmplaceNewPipeline( UUID shaderHash, const PipelineSpecification& rSpecification )
	{
		const auto [k, v] = m_Cache.emplace( std::make_pair( shaderHash, Ref<Pipeline>::Create( rSpecification ) ) );
		return k->second;
	}

	Ref<Pipeline> PipelineCache::GetPipeline( UUID shaderHash )
	{
		const auto itr = m_Cache.find( shaderHash );
		if( itr != m_Cache.end() )
		{
			return itr->second;
		}

		return nullptr;
	}

	void PipelineCache::RemovePipeline( UUID shaderHash )
	{
		if( !DoesPipelineExist( shaderHash ) )
			return;

		m_Cache.erase( shaderHash );
	}

}
