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
#include "App.h"

#include "Ruby/RubyWindow.h"
#include "Ruby/RubyMonitor.h"
#include "Ruby/RubyLibrary.h"

#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/VulkanContext.h"

#include "Renderer/RenderThread.h"

#include "Saturn/Audio/AudioSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "OptickProfiler.h"

#if defined( SAT_PLATFORM_WINDOWS )
#include <ShObjIdl.h>
#include <ShlObj.h>
#endif

#define APP_BIND_EVENT_FN(_) std::bind(&Application::_, this, std::placeholders::_1)

namespace Saturn {

	Application::Application( const ApplicationSpecification& spec )
		: m_Specification( spec )
	{
		SingletonStorage::AddSingleton( this );

		InitWindow();
		InitGraphics();

		// Now, resize to specification width and height
		if( m_Specification.WindowWidth != 0 && m_Specification.WindowHeight != 0 )
			m_Window->Resize( m_Specification.WindowWidth, m_Specification.WindowHeight );

		// Right before we create any threads, lets get the main thread id and handle.
		m_MainThreadID = std::this_thread::get_id();

		// Lazy load.
		AudioSystem::Get();
		RenderThread::Get().EnableIf( HasFlag( ApplicationFlag_UseGameThread ) );
		RenderThread::Get().Start();

		// ImGui is only used if we have the editor, and ImGui should not be used when building the game.
		m_ImGuiLayer = new ImGuiLayer();

#if defined( SAT_DIST )
		m_Window->Show( RubyWindowShowCmd::Fullscreen );
#else
		m_ImGuiLayer->OnAttach();
		m_Window->Show();
#endif
	}

	void Application::InitWindow()
	{
		// Setup default width and height
		const RubyMonitor& rPrimaryMonitor = RubyLibrary::Get().GetPrimaryMonitor();
		const uint32_t width = 3 * rPrimaryMonitor.MonitorSize.x / 4;
		const uint32_t height = 3 * rPrimaryMonitor.MonitorSize.y / 4;

		RubyStyle windowStyle = HasFlag( ApplicationFlag_Titlebar ) ? RubyStyle::Default : m_Specification.WindowStyle;

		RubyWindowSpecification windowSpec{ .Name = L"Saturn", .Width = width, .Height = height, .GraphicsAPI = RubyGraphicsAPI::Vulkan, .Style = windowStyle, .ShowNow = false };

		m_Window = new RubyWindow( windowSpec );
		m_Window->SetEventTarget( this );
	}

	void Application::InitGraphics()
	{
		// This may not be the best way... but it's better than lazy loading.
		m_VulkanContext = new VulkanContext();
		m_VulkanContext->Init();
	}

	Application::~Application()
	{
		SingletonStorage::RemoveSingleton( this );
	}

	void Application::Run()
	{
		// Tell children to create what ever they need.
		OnInit();

		while( m_Running )
		{
			ProcessAllEvents();

			for( auto&& rrFn : m_MainThreadQueue )
				rrFn();

			m_MainThreadQueue.clear();

			if( !m_Window->Minimized() )
			{
				Renderer::Get().BeginFrame();
				{
					BuildRenderCommands();
				}
				// End this frame on render thread.
				RenderThread::Get().Queue( [=] { Renderer::Get().EndFrame(); } );
			}
			else
				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

			// Execute render thread (last frame).
			RenderThread::Get().WaitAll();

			const float time = ( float ) m_Window->GetTime();
			const float frametime = time - m_LastFrameTime;

			m_Timestep = std::min<float>( frametime, 0.0333f );

			m_LastFrameTime = time;
		}

		OnShutdown();
		
		// So the difference between "Terminate" and delete is delete will completely destroy the class and remove it from the singleton list. 
		// However "Terminate" is used to destroy any data in the class but will not remove it from the singleton list, it is also used because we don't own the class so we can just implicitly destroy them.
		RenderThread::Get().RequestJoin();

		m_VulkanContext->SubmitTerminateResource( [&]()
		{
			for ( auto& rLayer : m_Layers )
			{
				delete rLayer;
			}

#if !defined( SAT_DIST )
			m_ImGuiLayer->OnDetach();
#endif

			delete m_ImGuiLayer;

			m_ImGuiLayer = nullptr;
		} );

		AudioSystem::Get().Terminate();
		
		delete m_VulkanContext;

		delete m_Window;
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::BuildRenderCommands()
	{
		SAT_PF_EVENT();

		// Update on the main thread.
		// Scene Rendering may happen here depending on if a layer as a SceneRenderer or not.
		for( auto& rLayer : m_Layers )
		{
			rLayer->OnUpdate( m_Timestep );
		}
			
		// Always submit Renderer2D AFTER a potential SceneRenderer has finished because we now may have new 
		// render commands to draw after OnUpdate was called.
		RenderThread::Get().Queue( [ = ] { Renderer2D::Get().Render(); } );

		//////////////////////////////////////////////////////////////////////////
		// Render ImGui.

#if !defined(SAT_DIST)
		// Begin on main thread.
		m_ImGuiLayer->Begin();

		RenderThread::Get().Queue( [=]
			{
				for( auto& rLayer : m_Layers )
				{
					rLayer->OnImGuiRender();
				}
			} );

		RenderThread::Get().Queue( [=]
			{
				m_ImGuiLayer->End( Renderer::Get().ActiveCommandBuffer() );
			} );
#endif
	}

	void Application::PushLayer( Layer* pLayer )
	{
		m_Layers.push_back( pLayer );
		pLayer->OnAttach();
	}

	void Application::PopLayer( Layer* pLayer )
	{
		// Find the layer in the layer stack.
		const auto itr = std::find( m_Layers.begin(), m_Layers.end(), pLayer );
		if( itr != m_Layers.end() )
		{
			m_Layers.erase( itr );
			pLayer->OnDetach();
		}
	}

	void Application::ProcessAllEvents()
	{
		// Poll all windows owned by the main thread
		RubyLibrary::PollEvents();

		std::scoped_lock<std::mutex> lock( m_Mutex );

		// After that, process any events that queued
		while( m_DeferredEventQueue.size() )
		{
			auto& rEvent = m_DeferredEventQueue.front();
			OnCustomEvent( *rEvent.get() );

			m_DeferredEventQueue.pop();
		}
	}

	bool Application::OnEvent( Event& rEvent )
	{
		switch( rEvent.Type )
		{
			case EventType::Resize:
			{
				OnWindowResize( ( RubyWindowResizeEvent& ) rEvent );
			} break;

			case EventType::Close:
			{
				Close();
			} break;

			default: break;
		}

		// Pass events to layers, this is the only place in the engine where we actually care if an event is handled or not.
		// Process Events backwards. 
		// This is because if we are in a game and we click a button if the first layer gets that event it might shoot in the game however we wanted to click a button not shoot.
		for( auto itr = m_Layers.end(); itr != m_Layers.begin(); )
		{
			( *--itr )->OnEvent( rEvent );

			if( rEvent.Handled )
				break;
		}

		return true;
	}

	void Application::OnWindowResize( RubyWindowResizeEvent& e )
	{
		const int width = e.GetWidth(), height = e.GetHeight();

		if( width == 0 && height == 0 )
			return;

		VulkanContext::Get().ResizeEvent();
	}

	void Application::OnCustomEvent( Event& rEvent )
	{
		// Pass events to layers, this is the only place in the engine where we actually care if an event is handled or not.
		// Process Events backwards. 
		// This is because if we are in a game and we click a button if the first layer gets that event it might shoot in the game however we wanted to click a button not shoot.
		for( auto itr = m_Layers.end(); itr != m_Layers.begin(); )
		{
			( *--itr )->OnEvent( rEvent );

			if( rEvent.Handled )
				break;
		}
	}

	std::filesystem::path Application::GetAppDataFolder() const
	{
		std::filesystem::path path = L"";

#if defined(SAT_PLATFORM_WINDOWS)
		PWSTR nativePath = 0;
		::SHGetKnownFolderPath( FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &nativePath );

		path = std::filesystem::path( nativePath ) / L"Saturn";

		::CoTaskMemFree( nativePath );
#elif defined(SAT_PLATFORM_LINUX)

#endif

		if( !std::filesystem::exists( path ) )
			std::filesystem::create_directories( path );

		return path;
	}

	void Application::SuspendMainThreadCV()
	{
		if( std::this_thread::get_id() == m_MainThreadID )
		{
			SAT_CORE_WARN( "Cannot suspend main thread if the current thread is the main thread, this will result in a deadlock!" );
			return;
		}

		bool complete = false;
		SubmitOnMainThread( [&]() 
			{
				std::unique_lock<std::mutex> lock( m_Mutex );
				complete = true;
				m_BlockCV.wait( lock );
			} );

		while( !complete ) std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}

	void Application::ResumeMainThreadCV()
	{
		m_BlockCV.notify_all();
	}

	std::filesystem::path Application::OpenFile( const char* pFilter ) const
	{
#ifdef  SAT_PLATFORM_WINDOWS
		::OPENFILENAMEA ofn;
		char szFile[ 260 ] = { 0 };

		::ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
		ofn.lStructSize = sizeof( OPENFILENAME );
		ofn.hwndOwner = ( HWND ) m_Window->GetNativeHandle();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof( szFile );
		ofn.lpstrFilter = pFilter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if( ::GetOpenFileNameA( &ofn ) == TRUE )
		{
			return std::filesystem::path( ofn.lpstrFile );
		}

		return "";
#endif

#ifdef  SAT_PLATFORM_LINUX
		return "";
#endif
	}

	std::filesystem::path Application::SaveFile( const char* pFilter ) const
	{
#ifdef  SAT_PLATFORM_WINDOWS
		::OPENFILENAMEA ofn;
		char szFile[ 260 ] = { 0 };

		::ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
		ofn.lStructSize = sizeof( OPENFILENAME );
		ofn.hwndOwner = ( HWND ) m_Window->GetNativeHandle();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof( szFile );
		ofn.lpstrFilter = pFilter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if( ::GetSaveFileNameA( &ofn ) == TRUE )
		{
			return std::filesystem::path( ofn.lpstrFile );
		}

		return "";
#endif

#ifdef  SAT_PLATFORM_LINUX
		return "";
#endif
	}

	std::filesystem::path Application::OpenFolder() const
	{
		std::filesystem::path path;

#if defined (SAT_PLATFORM_WINDOWS)
		::IFileOpenDialog* pFileOpen = nullptr;
		::PWSTR pszFilePath = 0;

		if( FAILED( ::CoInitialize( nullptr ) ) )
		{
			SAT_CORE_ASSERT( "Unable to initialise Windows COM (COM/0/0).", false );
		}

		// Create the object.
		::HRESULT hr = ::CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, ( void** ) &pFileOpen );

		if( FAILED( hr ) )
		{
			SAT_CORE_ASSERT( "Unable to create Windows COM/IID_IFileOpenDialog object", false );
		}

		::DWORD dwOptions;
		pFileOpen->GetOptions( &dwOptions );
		pFileOpen->SetOptions( dwOptions | FOS_PICKFOLDERS );

		// Show the dialog.
		hr = pFileOpen->Show( NULL );

		// Get the file name from the dialog.
		if( FAILED( hr ) )
		{
			SAT_CORE_ASSERT( "Unable to get the file name from the dialog using COM/IID_IFileOpenDialog", false );
		}

		::IShellItem* pItem;
		hr = pFileOpen->GetResult( &pItem );

		if( SUCCEEDED( hr ) )
		{
			hr = pItem->GetDisplayName( SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath );

			path = std::filesystem::path( pszFilePath );

			::CoTaskMemFree( pszFilePath );
		}

		pItem->Release();

		pFileOpen->Release();
		pFileOpen = NULL;

		return path;
#endif

		return "";
	}

	const char* Application::GetCurrentPlatformName()
	{
#if defined(SAT_PLATFORM_WINDOWS) || _WIN32 || _WIN64
		return "Windows";
#elif defined(SAT_LINUX) || __linux__
		return "Linux";
#elif defined(SAT_MAC) || __APPLE__
		return "Mac";
#else
		return "Unknown";
#endif
	}

	const char* Application::GetCurrentPlatformBinaryName()
	{
#if defined(SAT_PLATFORM_WINDOWS) || _WIN32 || _WIN64
		return "windows";
#elif defined(SAT_LINUX) || __linux__
		return "linux";
#elif defined(SAT_MAC) || __APPLE__
		return "mac";
#else
		return "Unknown";
#endif
	}

	const char* Application::GetCurrentConfigName()
	{
#if defined(SAT_DEBUG)
		return "Debug";
#elif defined(SAT_RELEASE)
		return "Release";
#elif defined(SAT_DIST)
		return "Dist";
#else
		return "Unknown";
#endif
	}

}
