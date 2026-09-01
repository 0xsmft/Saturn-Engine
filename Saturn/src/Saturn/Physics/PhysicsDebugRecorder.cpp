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
#include "PhysicsDebugRecorder.h"

#include "PhysicsFoundation.h"

#include "Saturn/Project/Project.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Core/Process.h"
#include "Saturn/Core/EnvironmentVariables.h"

// g_ActiveScene class type
#include "Saturn/Scene/Scene.h"

#include <Jolt/Physics/Body/BodyManager.h>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	void PhysicsRecorderOut::Open( const std::filesystem::path& rPath )
	{
		m_Stream.open( rPath, std::ios::binary | std::ios::trunc );
	}

	void PhysicsRecorderOut::Close()
	{
		m_Stream.close();
	}

	void PhysicsRecorderOut::WriteBytes( const void* pData, size_t numBytes )
	{
		m_Stream.write( ( const char* ) pData, numBytes );
	}

	bool PhysicsRecorderOut::IsFailed() const
	{
		return m_Stream.fail();
	}

	//////////////////////////////////////////////////////////////////////////

	PhysicsDebugRecorder::PhysicsDebugRecorder()
	{
	}

	PhysicsDebugRecorder::~PhysicsDebugRecorder()
	{
	}

	void PhysicsDebugRecorder::BeginRecord()
	{
		std::filesystem::path outPath = Project::GetActiveProject()->GetFullCachePath();
		outPath /= "PerUser";
		outPath /= std::format( "{0}.JoltCapture.jor", g_ActiveScene->Name );
		m_OutStream.Open( outPath );

#if !defined(SAT_DIST)
		m_Recorder = std::make_unique<JPH::DebugRendererRecorder>( m_OutStream );
#endif
	}

	void PhysicsDebugRecorder::NewFrame()
	{
#if !defined(SAT_DIST)
		JPH::BodyManager::DrawSettings drawSettings;
		PhysicsFoundation::Get()->GetPhysicsSystem()->DrawBodies( drawSettings, m_Recorder.get() );
		m_Recorder->EndFrame();
#endif
	}

	void PhysicsDebugRecorder::EndRecord()
	{
		m_OutStream.Close();

#if !defined(SAT_DIST)
		m_Recorder.reset();
#endif
	}

	void PhysicsDebugRecorder::OpenRecordedFile()
	{
		std::filesystem::path outPath = Project::GetActiveProject()->GetFullCachePath();
		outPath /= "PerUser";
		outPath /= std::format( "{0}.JoltCapture.jor", g_ActiveScene->Name );

		if( !std::filesystem::exists( outPath ) )
		{
			return;
		}

		const std::filesystem::path SaturnRootDir = Auxiliary::GetEnvironmentVariableWs( L"SATURN_DIR" );
		std::filesystem::path joltViewerPath = SaturnRootDir;
		joltViewerPath /= "Saturn";
		joltViewerPath /= "vendor";
		joltViewerPath /= "JoltPhysics";
		joltViewerPath /= "JoltViewer";
		joltViewerPath /= "PreBuilt";

#if defined(SAT_PLATFORM_WINDOWS)
		joltViewerPath /= "Windows-x64";
		joltViewerPath /= "JoltViewer.exe";
#elif defined(SAT_PLATFORM_LINUX)
		joltViewerPath /= "Linux-x64";
		joltViewerPath /= "JoltViewer";
#endif

		joltViewerPath += " ";
		joltViewerPath += outPath;

		DetachedProcess dp( joltViewerPath.wstring() );
	}

}
