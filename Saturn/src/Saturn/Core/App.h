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

#include "Base.h"

#include "Ruby/RubyEvent.h"

#include "Layer.h"
#include "Events.h"
#include "Input.h"
#include "EngineSettings.h"

#include "SingletonStorage.h"

#include <vector>
#include <functional>
#include <thread>
#include <mutex>

namespace Saturn {

	enum ApplicationFlags_
	{
		ApplicationFlag_UIOnly = BIT( 0 ),
		ApplicationFlag_CreateSceneRenderer = BIT( 1 ),
		ApplicationFlag_UseGameThread = BIT( 2 ),
		ApplicationFlag_Titlebar = BIT( 3 ),
		ApplicationFlag_UseVFS = BIT( 4 )
	};

	// enum ApplicationFlags_
	typedef int ApplicationFlags;

	struct ApplicationSpecification
	{
		ApplicationFlags Flags = 0x7FFFFFFF;

		bool Titlebar = false;
		
		uint32_t WindowWidth = 0;
		uint32_t WindowHeight = 0;
		RubyStyle WindowStyle = RubyStyle::Borderless;
	};

	class SceneRenderer;
	class VulkanContext;
	class Log;
	class RubyWindow;

	class Application : public RubyEventTarget
	{
	public:
		static inline Application& Get() { return *SingletonStorage::GetSingleton<Application>(); }
	public:
		Application( const ApplicationSpecification& spec );
		virtual ~Application();

		void Run();
		void Close();
	
	public:
		bool Running() const { return m_Running; }

		const Timestep& Time() { return m_Timestep; }

		std::filesystem::path OpenFile( const char* pFilter ) const;
		std::filesystem::path SaveFile( const char* pFilter ) const;
		std::filesystem::path OpenFolder() const;

		ApplicationSpecification& GetSpecification() { return m_Specification; }

		void PushLayer( Layer* pLayer );
		void PopLayer( Layer* pLayer );

		virtual void OnInit() {}
		virtual void OnShutdown() {}
		
		SceneRenderer& PrimarySceneRenderer() { return *m_SceneRenderer; }
		RubyWindow* GetWindow() { return m_Window; }

		void SubmitOnMainThread( std::function<void()>&& rrFunction )
		{
			m_MainThreadQueue.push_back( std::move( rrFunction ) );
		}

		std::filesystem::path& GetRootContentDir() { return RootContentPath; }
		const std::filesystem::path& GetRootContentDir() const { return RootContentPath; }

		constexpr bool HasFlag( ApplicationFlags flag ) const
		{
			return ( m_Specification.Flags & ( uint32_t ) flag ) != 0;
		}

		std::filesystem::path GetAppDataFolder() const;

		void SuspendMainThreadCV();
		void ResumeMainThreadCV();

	public:
		static const char* GetCurrentPlatformName();
		static const char* GetCurrentConfigName();
		static const char* GetCurrentPlatformBinaryName();

	protected:

		bool OnEvent( RubyEvent& rEvent ) override;
		bool OnWindowResize( RubyWindowResizeEvent& e );

		void RenderImGui();

		// The path where the default content is. Path is absolute.
		std::filesystem::path RootContentPath;

	protected:
		SceneRenderer* m_SceneRenderer = nullptr;
		RubyWindow* m_Window = nullptr;

	private:
		bool m_Running = true;
		
		ImGuiLayer* m_ImGuiLayer = nullptr;

		Timestep m_Timestep;
		float m_LastFrameTime = 0.0f;
		
		ApplicationSpecification m_Specification;

		std::vector<Layer*> m_Layers;

		// TODO: Change all of these to refs, I really don't like this.
		VulkanContext* m_VulkanContext = nullptr;

		// Concurrency (threading)
		std::thread::id m_MainThreadID;
		std::condition_variable m_BlockCV;
		std::mutex m_Mutex;

		std::vector<std::function<void()>> m_MainThreadQueue;
	};

	Application* CreateApplication( int argc, char** argv );
}