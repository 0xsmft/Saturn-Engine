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
#include "AssetViewer.h"

#include "Saturn/Core/OptickProfiler.h"

namespace Saturn {

	static std::unordered_map<AssetID, AssetViewer*> s_AssetViewers;
	static std::vector<AssetID> s_PendingAssetViewers;

	AssetViewer::AssetViewer( AssetID ID )
		: m_AssetID( ID )
	{
		s_AssetViewers[ ID ] = this;
	}

	AssetViewer::~AssetViewer()
	{
		s_AssetViewers[ m_AssetID ] = nullptr;
		s_AssetViewers.erase( m_AssetID );
	}

	static void CheckForDeadViewers() 
	{
		for( size_t i = 0; i < s_PendingAssetViewers.size(); i++ )
		{
			auto&& id = s_PendingAssetViewers.at( i );

			delete s_AssetViewers[ id ];

			s_AssetViewers.erase( id );
			s_PendingAssetViewers.erase( std::remove( s_PendingAssetViewers.begin(), s_PendingAssetViewers.end(), id ), s_PendingAssetViewers.end() );
		}
	}

	void AssetViewer::Draw()
	{
		SAT_PF_EVENT();

		CheckForDeadViewers();

		for ( auto&& [id, viewer] : s_AssetViewers )
		{
			viewer->OnImGuiRender();
		}
	}

	void AssetViewer::Update( Timestep ts )
	{
		CheckForDeadViewers();

		for( auto&& [id, viewer] : s_AssetViewers )
		{
			viewer->OnUpdate( ts );
		}
	}

	void AssetViewer::ProcessEvent( RubyEvent& rEvent )
	{
		CheckForDeadViewers();

		for( auto&& [id, viewer] : s_AssetViewers )
		{
			viewer->OnEvent( rEvent );
		}
	}

	void AssetViewer::DestroyViewer( AssetID ID )
	{
		s_PendingAssetViewers.push_back( ID );
	}

	void AssetViewer::Terminate()
	{
		for( auto&& [id, viewer] : s_AssetViewers )
		{
			if( viewer )
				delete viewer;
		}
	}

}