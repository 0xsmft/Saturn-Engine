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
#include "Saturn/Core/App.h"

#include "CrashReporterLayer.h"

class CrashReporterApplication : public Saturn::Application
{
public:
	explicit CrashReporterApplication( 
		const Saturn::ApplicationSpecification& spec, 
		const std::filesystem::path errPath )
		: Application( spec ), m_ErrorPath( errPath )
	{
		LoadFonts();
	}

	~CrashReporterApplication() = default;

	virtual void OnInit() override
	{
		m_pMainLayer = new Saturn::CrashReporterLayer( m_ErrorPath );
		PushLayer( m_pMainLayer );
	}

	virtual void OnShutdown() override
	{
		PopLayer( m_pMainLayer );
		delete m_pMainLayer;
	}

private:
	Saturn::CrashReporterLayer* m_pMainLayer = nullptr;
	std::filesystem::path m_ErrorPath;
};

Saturn::Application* Saturn::CreateApplication( int argc, char** argv )
{
	std::filesystem::path errPath;
	if( argc >= 2 )
	{
		errPath = argv[ 1 ];
	}

	Saturn::ApplicationSpecification spec;
	spec.Flags = Saturn::ApplicationFlag_UIOnly;
	spec.WindowWidth = 450;
	spec.WindowHeight = 770;

	return new CrashReporterApplication( spec, errPath );
}
