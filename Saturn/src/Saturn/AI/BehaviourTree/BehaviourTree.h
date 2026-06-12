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

#include "Blackboard.h"

#include "Saturn/AI/AIAgentEntity.h"

#include "Saturn/Asset/Asset.h"

namespace Saturn {

	class BehaviourTreeTaskHandler;

	//
	// BehaviourTree 
	// 
	// This class is local and is tied to an asset but can be loaded many time. 
	// For every entity that uses a BehaviourTree this class will be created.
	//
	class BehaviourTree : public RefTarget
	{
	public:
		BehaviourTree() = default;
		BehaviourTree( AssetID id );
		virtual ~BehaviourTree();

		void Initialise( SharedPtr<AIAgentEntity> entity );
		void Tick( Timestep ts );

		Ref<Asset> GetUnderlyingAsset() const { return m_BehaviourTreeAsset; }
		
		Ref<BehaviourTreeTaskHandler> GetTaskHandler();
		const Ref<BehaviourTreeTaskHandler> GetTaskHandler() const;

#if !defined(SAT_DIST)
	public:
		const std::string& GetDebugName() const { return m_DebugName; }

	private:
		std::string m_DebugName;
#endif
		// The "BehaviourTree" class is not an asset however BehaviourTree are an asset
		Ref<Asset> m_BehaviourTreeAsset;
		Ref<BehaviourTreeTaskHandler> m_TaskHandler;
	};

}
