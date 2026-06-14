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

#include "BehaviourTreeConditionTask.h"

namespace Saturn {

	enum class BTBlackboardConditionQueryType : uint8_t
	{
		Set,
		NotSet
	};

	inline std::string BTBlackboardConditionQueryTypeToString( BTBlackboardConditionQueryType type ) 
	{
		switch( type )
		{
			case BTBlackboardConditionQueryType::Set:
				return "Is Set";

			case BTBlackboardConditionQueryType::NotSet:
				return "Is Not Set";

			default: return "Unknown";
		}
	}

	// BehaviourTreeMemoryCondition
	//
	// Condition if a blackboard key has a valid value or not, determined by the QueryType (BTBlackboardConditionQueryType)
	//
	SCLASS()
	class BehaviourTreeBlackboardCondition : public BehaviourTreeConditionTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeBlackboardCondition, BehaviourTreeConditionTask )
	public:
		BehaviourTreeBlackboardCondition();
		virtual ~BehaviourTreeBlackboardCondition();

		//////////////////////////////////////////////////////////////////////////
		// BehaviourTreeBaseTask

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

	public:
		//////////////////////////////////////////////////////////////////////////
		// BehaviourTreeCondition

#if !defined( SAT_DIST )
		virtual void RenderDetails() override;
		virtual std::string GetTitleText() const override;
#endif
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		// On Dist, we don't need to store our key spec, only with editor to allow us to select a target variable
#if !defined( SAT_DIST )
		Ref<BlackboardVaraibleSpec> m_VariableSpec;
#endif
		BTBlackboardConditionQueryType m_QueryType = BTBlackboardConditionQueryType::Set;
	};

}
