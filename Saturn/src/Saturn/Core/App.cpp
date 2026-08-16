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
#include "App.h"

#include "Process.h"
#include "JobSystem.h"
#include "AuxiliaryEvents.h"

#include "Ruby/RubyWindow.h"
#include "Ruby/RubyMonitor.h"
#include "Ruby/RubyLibrary.h"

#include "Saturn/Vulkan/Renderer.h"
#include "Saturn/Vulkan/VulkanContext.h"

#include "Renderer/RenderThread.h"

#include "Profiler.h"

#include <nativefiledialog/nfd.hpp>

#if defined( SAT_PLATFORM_WINDOWS )
// Needed for FOLDERID_RoamingAppData et al.
#include <ShObjIdl.h>
#include <ShlObj.h>
#endif

#if defined(SAT_DIST) && !defined(SAT_WITH_CRASHCATCH)
#define SAT_WITH_CRASHCATCH 1
#endif

#if SAT_WITH_CRASHCATCH
#include <CrashCatch/CrashCatch.hpp>
#include "EnvironmentVariables.h"
#endif

#define APP_BIND_EVENT_FN(_) std::bind(&Application::_, this, std::placeholders::_1)

namespace Saturn {

	Application::Application( const ApplicationSpecification& spec )
		: m_Specification( spec )
	{
		SingletonStorage::AddSingleton( this );

#if	SAT_WITH_CRASHCATCH
		InitCrashReporter();
#endif

		InitWindow();
		InitGraphics();

		// Now, resize to specification width and height
		if( m_Specification.WindowWidth != 0 && m_Specification.WindowHeight != 0 )
			m_Window->Resize( m_Specification.WindowWidth, m_Specification.WindowHeight );

		// Right before we create any threads, lets get the main thread id and handle.
		m_MainThreadID = std::this_thread::get_id();

		// Lazy load.
		RenderThread::Get().EnableIf( HasFlag( ApplicationFlag_UseGameThread_DEPRECATED ) );
		RenderThread::Get().Start();

#if defined( SAT_DIST )
		m_Window->Show();
#else
		m_ImGuiLayer = new ImGuiLayer();
		m_ImGuiLayer->OnAttach();
		m_Window->Show();
#endif

#if defined( SAT_PROFILER_ENABLE )
		tracy::StartupProfiler();
#endif
	}

	void Application::InitWindow()
	{
		// Setup default width and height
		const RubyMonitor& rPrimaryMonitor = RubyLibrary::Get().GetPrimaryMonitor();
		const uint32_t width = 3 * rPrimaryMonitor.MonitorSize.x / 4;
		const uint32_t height = 3 * rPrimaryMonitor.MonitorSize.y / 4;

		const RubyStyle windowStyle = HasFlag( ApplicationFlag_Titlebar ) ? RubyStyle::Default : m_Specification.WindowStyle;

		const RubyWindowSpecification windowSpec{ .Name = L"Saturn", .Width = width, .Height = height, .GraphicsAPI = RubyGraphicsAPI::Vulkan, .Style = windowStyle, .ShowNow = false };

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
				Renderer::Get()->BeginFrame();
				{
					BuildRenderCommands();
				}
				// End this frame on render thread.
				RenderThread::Get().Queue( [=] { Renderer::Get()->EndFrame(); } );
			}
			else
				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

			// Execute render thread (last frame).
			RenderThread::Get().WaitAll();

			const float time = ( float ) m_Window->GetTime();
			const float frametime = time - m_LastFrameTime;

			m_Timestep = std::min<float>( frametime, 0.0333f );
			m_Framerate = 1.0f / m_Timestep;

			m_LastFrameTime = time;
		}

		JobSystem::Get().WaitForUnfinshedJobs();

#if defined( SAT_PROFILER_ENABLE )
		tracy::ShutdownProfiler();
#endif

		OnShutdown();
		
		// So the difference between "Terminate" and delete is delete will completely destroy the class and remove it from the singleton list. 
		// However "Terminate" is used to destroy any data in the class but will not remove it from the singleton list, it is also used because we don't own the class so we can just implicitly destroy them.
		RenderThread::Get().RequestJoin();

#if !defined( SAT_DIST )
		m_VulkanContext->SubmitTerminateResource( [&]()
		{
			SAT_CORE_ASSERT( m_Layers.empty(), "Not all layers have been removed prior to the Application shutdown. The applicaition is not responsible for cleaning up layers it doesn't own." );

			m_ImGuiLayer->OnDetach();
			delete m_ImGuiLayer;
			m_ImGuiLayer = nullptr;
		} );
#endif
		
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
		// Scene Rendering may happen here depending on if a layer has a SceneRenderer or not.
		for( auto& rLayer : m_Layers )
		{
			rLayer->OnUpdate( m_Timestep );
		}

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
				m_ImGuiLayer->End( Renderer::Get()->ActiveCommandBuffer() );
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
			pLayer->OnDetach();
			m_Layers.erase( itr );
		}
	}

	void Application::ProcessAllEvents()
	{
		// Poll all windows owned by the main thread.
		RubyLibrary::PollEvents();

		std::scoped_lock<std::mutex> lock( m_Mutex );

		// After that, process any events that queued.
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

		VulkanContext::Get()->ResizeEvent();
	}

	void Application::OnCustomEvent( Event& rEvent )
	{
		switch( rEvent.Type )
		{
			// This event is only handled by the application so
			// we'll not pass this on.
			case EventType::RequestRemoveLayer:
			{
				const auto& rRemoveLayerEvent = ( OnRequestRemoveApplicationLayer& ) rEvent;
				PopLayer( rRemoveLayerEvent.GetLayer() );

				DispatchEvent<OnRequestRemoveApplicationLayerReply, true>( rRemoveLayerEvent.GetLayer() );
			} return;

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
		Core::BreakDebug();
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

#if !defined(SAT_DIST)
	void Application::LoadFonts()
	{
		if( m_ImGuiLayer )
			m_ImGuiLayer->LoadFonts();
	}
#endif

	std::filesystem::path Application::OpenFile( const std::string& rFilter ) const
	{
		NFD::Init();

		std::filesystem::path path;

		std::vector<std::string> tokens;
		std::stringstream ss( rFilter );
		std::string item;

		// Split by |.
		while( std::getline( ss, item, '|' ) )
		{
			if( !item.empty() )
				tokens.push_back( item );
		}

		std::vector<nfdu8filteritem_t> filters;

		// Expecting pairs... (description | exts)
		for( size_t i = 0; i + 1 < tokens.size(); i += 2 )
		{
			filters.emplace_back( tokens[ i ].c_str(), tokens[ i + 1 ].c_str() );
		}

		// UniquePathN == std::unique_ptr so it's kinda like a span of chars
		NFD::UniquePathU8 nfdPath;

		nfdwindowhandle_t parentWindow
		{ 
#if defined(SAT_PLATFORM_WINDOWS)
			.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS, 
#elif defined(SAT_PLATFORM_LINUX)
			.type = NFD_WINDOW_HANDLE_TYPE_X11, 
#endif
			.handle = ( void* ) m_Window->GetNativeHandle() 
		};

		if( NFD::OpenDialog( nfdPath, filters.data(), ( nfdfiltersize_t ) filters.size(), nullptr, parentWindow ) == NFD_OKAY ) 
		{
			path = std::filesystem::path( nfdPath.get() );
		}

		NFD::Quit();

		return path;
	}

	std::vector<std::filesystem::path> Application::OpenMultipleFiles( const std::string& rFilter ) const
	{
		std::vector<std::filesystem::path> paths;

		NFD::Init();

		std::vector<std::string> tokens;
		std::stringstream ss( rFilter );
		std::string item;

		// Split by |.
		while( std::getline( ss, item, '|' ) )
		{
			if( !item.empty() )
				tokens.push_back( item );
		}

		std::vector<nfdu8filteritem_t> filters;

		// Expecting pairs... (description | exts)
		for( size_t i = 0; i + 1 < tokens.size(); i += 2 )
		{
			filters.emplace_back( tokens[ i ].c_str(), tokens[ i + 1 ].c_str() );
		}

		nfdwindowhandle_t parentWindow
		{
#if defined(SAT_PLATFORM_WINDOWS)
			.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS,
#elif defined(SAT_PLATFORM_LINUX)
			.type = NFD_WINDOW_HANDLE_TYPE_X11,
#endif
			.handle = ( void* ) m_Window->GetNativeHandle()
		};

		// This is a unique ptr, so we do not have to worry about freeing it.
		NFD::UniquePathSet nfdPaths;
		if( NFD::OpenDialogMultiple( nfdPaths, filters.data(), ( nfdfiltersize_t ) filters.size(), nullptr, parentWindow ) == NFD_OKAY )
		{
			nfdpathsetsize_t count;
			if( NFD::PathSet::Count( nfdPaths, count ) == NFD_OKAY )
			{
				paths.reserve( count );
				
				for( nfdpathsetsize_t i = 0; i < count; ++i )
				{
					NFD::UniquePathSetPathU8 nfdPath;
					if( NFD::PathSet::GetPath( nfdPaths, i, nfdPath ) == NFD_OKAY )
					{
						paths.push_back( std::filesystem::path( nfdPath.get() ) );
					}
				}
			}
		}

		NFD::Quit();

		return paths;
	}

	std::filesystem::path Application::SaveFile( const std::string& rFilter ) const
	{
		NFD::Init();

		std::filesystem::path path;

		std::vector<std::string> tokens;
		std::stringstream ss( rFilter );
		std::string item;

		// Split by |.
		while( std::getline( ss, item, '|' ) )
		{
			if( !item.empty() )
				tokens.push_back( item );
		}

		std::vector<nfdu8filteritem_t> filters;

		// Expecting pairs... (description | exts)
		for( size_t i = 0; i + 1 < tokens.size(); i += 2 )
		{
			filters.emplace_back( tokens[ i ].c_str(), tokens[ i + 1 ].c_str() );
		}

		// UniquePathN == std::unique_ptr so it's kinda like a span of chars
		NFD::UniquePathU8 nfdPath;

		nfdwindowhandle_t parentWindow
		{
#if defined(SAT_PLATFORM_WINDOWS)
			.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS,
#elif defined(SAT_PLATFORM_LINUX)
			.type = NFD_WINDOW_HANDLE_TYPE_X11,
#endif
			.handle = ( void* ) m_Window->GetNativeHandle()
		};

		if( NFD::SaveDialog( nfdPath, filters.data(), ( nfdfiltersize_t ) filters.size(), nullptr, nullptr, parentWindow ) == NFD_OKAY )
		{
			path = std::filesystem::path( nfdPath.get() );
		}

		NFD::Quit();

		return path;
	}

	void Application::OpenNativeFileExplorer( const std::filesystem::path& rPath, bool select /*= false */ )
	{
#if defined(SAT_PLATFORM_WINDOWS)
		std::wstring CommandLine = L"";
		
		if( select )
			CommandLine = std::format( L"explorer.exe /select,\"{0}\"", rPath.wstring() );
		else
			CommandLine = std::format( L"explorer.exe \"{0}\"", rPath.wstring() );

		DetachedProcess dp( CommandLine );
#elif defined(SAT_PLATFORM_LINUX) || defined(SAT_PLATFORM_MACOS)
		SAT_CORE_ASSERT( false, "Application::OpenNativeFileExplorer not implemented on Linux!" );
#endif
	}

	std::filesystem::path Application::OpenFolder() const
	{
		std::filesystem::path path;

		NFD::Init();

		nfdwindowhandle_t parentWindow
		{
#if defined(SAT_PLATFORM_WINDOWS)
			.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS,
#elif defined(SAT_PLATFORM_LINUX)
			.type = NFD_WINDOW_HANDLE_TYPE_X11,
#endif
			.handle = ( void* ) m_Window->GetNativeHandle()
		};

		// UniquePathN == std::unique_ptr so it's kinda like a span of wchars
		NFD::UniquePathN nfdPath;
		if( NFD::PickFolder( nfdPath, nullptr, parentWindow ) == NFD_OKAY ) 
		{
			path = std::filesystem::path( nfdPath.get() );
		}

		NFD::Quit();

		return path;
	}

	const char* Application::GetCurrentPlatformName()
	{
		return SAT_PLATFORM_FRIENDLY_NAME;
	}

	const char* Application::GetCurrentPlatformBinaryName()
	{
#if defined(SAT_PLATFORM_WINDOWS) || _WIN32 || _WIN64
		return "windows";
#elif defined(SAT_PLATFORM_LINUX) || __linux__
		return "linux";
#elif defined(SAT_PLATFORM_MACOS) || __APPLE__
		return "macosx";
#else
		return "Unknown";
#endif
	}

	ApplicationConfigKind Application::GetCurrentConfigKind()
	{
#if defined(SAT_DEBUG)
		return ApplicationConfigKind::Debug;
#elif defined(SAT_RELEASE)
		return ApplicationConfigKind::Release;
#elif defined(SAT_DIST)
		return ApplicationConfigKind::Dist;
#else
#error "Application::GetCurrentConfigKind Unknown config type"
		return ApplicationConfigKind::Debug;
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

	void Application::InitCrashReporter()
	{
#if SAT_WITH_CRASHCATCH
		CrashCatch::Config config;
		config.appVersion = std::format( "{} {} " SAT_CURRENT_VERSION_BUILD_TAG, SAT_CURRENT_VERSION, SAT_CURRENT_VERSION_STRING );
		config.buildConfig = GetCurrentConfigName();
		config.dumpFileName = "Saturn_Engine";
		config.dumpFolder = std::filesystem::current_path() / "Dumps";
		config.showCrashDialog = false;
		config.onCrash = []( const CrashCatch::CrashContext& rContext )
		{
			std::filesystem::path SaturnDir = Auxiliary::GetEnvironmentVariableWs( L"SATURN_DIR" );
			std::filesystem::path WorkingDir = SaturnDir / "Saturn-CrashReporter";

			// This check is important because if the client has never installed a source build of Saturn then this
			// environment variable is never set.
			// So, if it's empty we just use the current working directory, as when a distribution game is shipped
			// the crash reporter is placed in the same directory.
			if( SaturnDir.empty() )
			{
				WorkingDir = SaturnDir = std::filesystem::current_path();
			}
			else
			{
				const std::string binaryFolderName = std::format( "{0}-{1}-x86_64", Application::GetCurrentConfigName(), Application::GetCurrentPlatformBinaryName() );

				SaturnDir /= L"bin";
				SaturnDir /= binaryFolderName;
				SaturnDir /= L"Saturn-CrashReporter";
			}

#if defined( SAT_PLATFORM_WINDOWS )
			SaturnDir /= L"Saturn-CrashReporter.exe";
#else
			SaturnDir /= L"Saturn-CrashReporter";
#endif
			SaturnDir += L" ";
			SaturnDir += rContext.logFilePath;

			DetachedProcess dp( SaturnDir.wstring(), WorkingDir );
		};

		CrashCatch::initialize( config );
#endif
	}

}
