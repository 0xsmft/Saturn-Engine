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

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeBaseTask.h"
#include "BehaviourTreeConditionInfo.h"

namespace Saturn {

	class BehaviourTreeCondition : public BehaviourTreeBaseTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeCondition, BehaviourTreeBaseTask )
	public:
		BehaviourTreeCondition() = default;
		BehaviourTreeCondition( const std::string& rTitle, BehaviourTreeConditionType type ) 
			: m_Title( rTitle ), m_ConditionType( type )
		{
		}

		~BehaviourTreeCondition() = default;

#if !defined( SAT_DIST )
		virtual void RenderDetails() {}
		virtual std::string GetTitleText() const { return m_Title; }

		[[nodiscard]] virtual bool IsSpawnableNode() const override final { return false; }
		virtual const char* GetTaskName() const override final { return ""; }
		virtual void OnRenderExtra() override final {}
#endif

		BehaviourTreeConditionType GetConditionType() const { return m_ConditionType; }

	public:
		void SetupMemVariable( AssetID memSpecID );

	public:
		virtual void Serialise( std::ofstream& rStream ) const;
		virtual void Deserialise( std::ifstream& rStream );

	protected:
#if !defined( SAT_DIST )
		std::string m_Title;

		Ref<BehaviourTreeMemorySpecification> m_BlackboardSpec;
#endif

		BehaviourTreeConditionType m_ConditionType = BehaviourTreeConditionType::None;
	};

}
