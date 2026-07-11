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

#include "Saturn/Core/Ref.h"
#include "Saturn/Runtime/RuntimeState.h"

#include <string>
#include <unordered_map>

namespace Saturn {
	
	class ImGuiWindow;
	class Event;

	class ImGuiWindowManager : public RefTarget
	{
	public:
		static inline ImGuiWindowManager* Get() { return SingletonStorage::GetSingleton<ImGuiWindowManager>(); }

	public:
		ImGuiWindowManager();
		~ImGuiWindowManager();

		void DrawAll();
		void ProcessEvent( Event& rEvent );
		void OnUpdate( Timestep ts );
		
		template<typename Ty, typename... VaArgs>
		Ref<Ty> AddWindow( VaArgs&&... rrArgs )
		{
			static_assert( std::is_base_of<ImGuiWindow, Ty>::value, "Ty must be a child class of Panel!" );

			auto panel = Ref<Ty>::Create( std::forward<VaArgs>( rrArgs )... );
			panel->SetReadOnly( m_IsReadOnly );
			m_Panels[ Ty::GetStaticName() ] = panel;
			return panel;
		}

		void AddWindow( Ref<ImGuiWindow> window, const std::string& rCustomName );

		template<typename Ty, typename... VaArgs>
		void OpenOrShowWindow( const std::string& rCustomName, VaArgs&&... rrArgs )
		{
			Ref<Ty> window = GetWindow<Ty>( rCustomName );

			if( window )
			{
				window->OpenWindow();
			}
			else
			{
				AddWindow( Ref<Ty>::Create( std::forward<VaArgs>( rrArgs )... ), rCustomName );
			}
		}

		template<typename Ty>
		[[nodiscard]] Ref<Ty> GetWindow( const std::string& rPanelName )
		{
			static_assert( std::is_base_of<ImGuiWindow, Ty>::value, "Ty must be a child class of Panel!" );

			auto Itr = m_Panels.find( rPanelName );

			if( Itr != m_Panels.end() )
				return Itr->second.As<Ty>();
			else
				return nullptr;
		}

		template<typename Ty>
		[[nodiscard]] Ref<Ty> GetPanel()
		{
			static_assert( std::is_base_of<ImGuiWindow, Ty>::value, "Ty must be a child class of Panel!" );

			auto Itr = m_Panels.find( Ty::GetStaticName() );

			if( Itr != m_Panels.end() )
				return Itr->second.As<Ty>();
			else
				return nullptr;
		}

		template<typename Ty>
		void DestroyPanel() 
		{
			auto Itr = m_Panels.find( Ty::GetStaticName() );

			if( Itr != m_Panels.end() ) 
			{
				m_Panels[ Itr ] = nullptr;
				m_Panels.erase( Itr );
			}
		}

		void OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState );

		void MarkAllWindowsAsReadOnly();
		void ResetReadOnlyState();

	private:
		void Terminate();

	private:
		//                        NAME -> Window
		std::unordered_map<std::string, Ref<ImGuiWindow>> m_Panels;

		// Global read only flag for all windows.
		bool m_IsReadOnly = false;
	};
}
