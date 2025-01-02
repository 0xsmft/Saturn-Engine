/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

// This Job System is based from Geno IDE's system:
// Thank You: https://github.com/Geno-IDE/Geno/blob/master/src/Common/Async/JobSystem.cpp

#include "sppch.h"
#include "JobSystem.h"

namespace Saturn {

	JobSystem::JobSystem()
	{
		SetMaxThreads( 2 );
		CreateThreads();
	}

	JobSystem::~JobSystem()
	{
		Stop();
	}

	void JobSystem::Stop()
	{
		m_Running = false;
		TerminateThreads();
	}

	void JobSystem::SetMaxThreads( size_t maxThreads )
	{
		m_MaxThreads = glm::clamp( ( size_t ) maxThreads, ( size_t ) 1, ( size_t )std::thread::hardware_concurrency() );
	}

	void JobSystem::CreateThreads()
	{
		// Clear last threads if any.
		TerminateThreads();

		m_Running = true;

		SetMaxThreads( m_MaxThreads );
		m_Threads.resize( m_MaxThreads );

		for( size_t i = 0; i < m_MaxThreads; i++ )
		{
			m_Threads[ i ] = std::thread( &JobSystem::ThreadRun, this );
		}
	}

	void JobSystem::TerminateThreads()
	{
		size_t index = 0;
		for( auto& rThread : m_Threads )
		{
			if( rThread.joinable() )
			{
				rThread.join();
			}

			m_Threads.erase( m_Threads.begin() + index );
			index++;
		}
	}

	void JobSystem::ThreadRun()
	{
		SetThreadDescription( GetCurrentThread(), L"JobSystemThread" );

		while( m_Running )
		{
			while( !m_Jobs.empty() )
			{
				Ref<Job> currentJob;

				m_Mutex.lock();

				// Find any jobs that have not yet been done.
				for( auto Itr = m_Jobs.begin(); Itr != m_Jobs.end(); ++Itr )
				{
					auto& rJob = *Itr;

					if( rJob && rJob->CanRun() )
					{
						currentJob = rJob;
						
						// Make sure we remove it so that it cannot be executed again.
						m_Jobs.erase( Itr );
						break;
					}
				}

				m_Mutex.unlock();

				if( currentJob )
				{
					currentJob->ExecuteJob();
				}
			}

			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
	}

}