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

#include "Job.h"
#include "Base.h"

#include <mutex>
#include <thread>
#include <span>

namespace Saturn {

	class JobSystem
	{
	public:
		SAT_SINGLETON_LAZY( JobSystem )

	public:
		JobSystem();
		~JobSystem();

		void Stop();
		void SetMaxThreads( size_t maxThreads );

		template<typename Func>
		Ref<Job> QueueJob( Func&& rrFunc )
		{
			std::unique_lock<std::mutex>( m_Mutex );

			Ref<Job> newJob = Ref<Job>::Create( rrFunc );

			// TODO: ADD DEPENDENCIES!
			m_Jobs.push_back( newJob );
		
			return newJob;
		}

	private:
		void ThreadRun();
		void CreateThreads();
		void TerminateThreads();

	private:
		bool m_Running = false;
		size_t m_MaxThreads = 0;

		std::mutex m_Mutex;
		std::vector<std::thread> m_Threads;
		std::vector<Ref<Job>> m_Jobs;
	};
}