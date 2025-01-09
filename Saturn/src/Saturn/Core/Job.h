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

#include "Ref.h"
#include <functional>

// This Job System is based from Geno IDE's system:
// Thank You: https://github.com/Geno-IDE/Geno/blob/master/src/Common/Async/Job.cpp

namespace Saturn {

	class Job : public RefTarget
	{
	public:
		template<typename Func>
		explicit Job( Func&& rrFunc )
			: m_Function( std::forward<Func>( rrFunc ) )
		{
		}

		bool CanRun() const;
		void DependsOn( std::weak_ptr< Job > job );

	private:
		void ExecuteJob()
		{
			m_Function();
			m_Completed = true;
		}

	private:
		bool m_Completed = false;

		std::function<void()> m_Function;
		std::vector< std::weak_ptr< Job > > m_Dependencies{};
	private:
		friend class JobSystem;
	};
}