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

#include "Saturn/Core/Ref.h"

#include <string>
#include <unordered_map>

namespace Saturn {
	
	class Panel;

	class PanelManager : public RefTarget
	{
	public:
		PanelManager();
		~PanelManager();

		void DrawAllPanels();
		
		template<typename Ty>
		void AddPanel( const Ref<Ty>& rPanel )
		{
			static_assert( std::is_base_of<Panel, Ty>::value, "Ty must be a child class of Panel!" );

			m_Panels[ Ty::GetStaticName() ] = rPanel;
		}

		void AddPanel( const Ref<Panel>& rPanel, const std::string& rCustomName );

		template<typename Ty>
		[[nodiscard]] Ref<Ty> GetPanel( const std::string& rPanelName )
		{
			static_assert( std::is_base_of<Panel, Ty>::value, "Ty must be a child class of Panel!" );

			auto Itr = m_Panels.find( rPanelName );

			if( Itr != m_Panels.end() )
				return Itr->second.As<Ty>();
			else
				return nullptr;
		}

		template<typename Ty>
		[[nodiscard]] Ref<Ty> GetPanel()
		{
			static_assert( std::is_base_of<Panel, Ty>::value, "Ty must be a child class of Panel!" );

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

	private:
		void Terminate();

	private:
		std::unordered_map<std::string, Ref<Panel>> m_Panels;
	};
}