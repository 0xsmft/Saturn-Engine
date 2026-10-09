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

#include "Pipeline.h"

namespace Saturn {

	//
	// PipelineCache
	//
	// The pipeline cache is responsible for holding a collection of
	// pipelines.
	// 
	// A pipeline cache is key-value map to pipelines.
	// 
	// The PipelineCache will assume ownership of any pipelines
	// that are added/created into the cache.
	// 
	// For example, geometry passes uses a PipelineCache,
	// this is because every mesh that comes into a geometry pass
	// may have a different shader and such would need a different
	// pipeline.
	// 
	//
	class PipelineCache : public RefTarget
	{
	public:
		PipelineCache();
		virtual ~PipelineCache();

		[[nodiscard]] bool DoesPipelineExist( UUID shaderHash ) const;

		Ref<Pipeline> EmplaceNewPipeline( UUID shaderHash, const PipelineSpecification& rSpecification );

		Ref<Pipeline> GetPipeline( UUID shaderHash );

		void RemovePipeline( UUID shaderHash );

	private:
		void Clear();

	private:
		//				SHADER HASH -> PIPELINE
		std::unordered_map<UUID, Ref<Pipeline>> m_Cache;
	};
	
}
