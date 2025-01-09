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

#include "sppch.h"
#include "Saturn/Core/App.h"

#include "ProjectBrowserLayer.h"

class ProjectBrowserApplication : public Saturn::Application
{
public:
	explicit ProjectBrowserApplication( const Saturn::ApplicationSpecification& spec )
		: Application( spec )
	{
	}

	~ProjectBrowserApplication() {}

	virtual void OnInit() override
	{
		m_ProjectBrowserLayer = new Saturn::ProjectBrowserLayer();

		PushLayer( m_ProjectBrowserLayer );
	}

	virtual void OnShutdown() override
	{
		PopLayer( m_ProjectBrowserLayer );
		delete m_ProjectBrowserLayer;
	}

private:
	Saturn::ProjectBrowserLayer* m_ProjectBrowserLayer = nullptr;
};

Saturn::Application* Saturn::CreateApplication( int argc, char** argv ) 
{
	Saturn::ApplicationSpecification spec;
	spec.Flags = Saturn::ApplicationFlag_UIOnly;
	spec.WindowWidth = 1200;
	spec.WindowHeight = 780;

	return new ProjectBrowserApplication( spec );
}